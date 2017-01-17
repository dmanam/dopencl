/******************************************************************************
 * This file is part of dOpenCL.
 * 
 * dOpenCL is an implementation of the OpenCL application programming
 * interface for distributed systems. See <http://dopencl.uni-muenster.de/>
 * for more information.
 * 
 * Developed by: Research Group Parallel and Distributed Systems
 *               Department of Mathematics and Computer Science
 *               University of Muenster, Germany
 *               <http://pvs.uni-muenster.de/>
 * 
 * Copyright (C) 2013  Philipp Kegel <philipp.kegel@uni-muenster.de>
 *
 * dOpenCL is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * dOpenCL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with dOpenCL. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Permission to use dOpenCL for scientific, non-commercial work is
 * granted under the terms of the dOpenCL Academic License provided
 * appropriate credit is given. See the dOpenCL Academic License for
 * more details.
 * 
 * You should have received a copy of the dOpenCL Academic License
 * along with dOpenCL. If not, see <http://dopencl.uni-muenster.de/>.
 ******************************************************************************/

/**
 * @file Program.cpp
 *
 * @date 2012-06-07
 * @author Karsten Jeschkies
 * @author Philipp Kegel
 */

#include "Program.h"

#include "Context.h"
#include "Device.h"
#include "Retainable.h"

#include "dclicd/Error.h"
#include "dclicd/utility.h"

#include "dclicd/detail/ProgramBuild.h"
#include "dclicd/detail/ProgramBuildInfo.h"

#include <dclasio/message/CreateProgramWithSource.h>
#include <dclasio/message/DeleteProgram.h>
#include <dclasio/message/ErrorResponse.h>
#include <dclasio/message/Response.h>

#include <dcl/CLError.h>
#include <dcl/ComputeNode.h>
#include <dcl/DataTransfer.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>
#include <dcl/Remote.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>


_cl_program::_cl_program(cl_context context, const Sources& sources) :
	_context(context), _isBuilt(false), _numKernels(0)
{
	if (!context) throw dclicd::Error(CL_INVALID_CONTEXT);
	if (sources.empty()) throw dclicd::Error(CL_INVALID_VALUE);

	/*
	 * compute total lengths
	 */
	unsigned int totalLength = 0;
	for (auto source : sources) {
		totalLength += source.second;
	}

    /* The compute nodes hosting this program are the compute nodes that host
     * the program's context. */
    _computeNodes = _context->computeNodes();

	/*
	 * concatenate strings
	 */
	_source.reserve(totalLength);
	for (auto source : sources) {
		_source.append(source.first, source.second);
	}

    /* The devices associated with a program are the devices that are
     * associated with a context, if a program is build from source. */
    _devices = _context->devices();
    _binaries.resize(_devices.size()); // _binaries must hold an entry for each device

	/*
	 * TODO Do not create program remotely before program build
	 * Creating a program currently is one of the most time-intensive operations
	 * in dOpenCL, as the program code is transferred to all compute nodes of a
	 * context. Deferring that data transfer to the time of program build reduces
	 * restricts data transfers to the compute nodes that the program is actually
	 * built for.
	 * Moreover, a deferred program build enables dOpenCL to correctly resolve
	 * include directives, as the list of include paths is not specified before
	 * compilation. Only if this list is known, dOpenCL can replace directives of
	 * the form '#include "..."' by the content of the found file, or by
	 * '#include<...>' statements to fallback to the default search on the compute
	 * node.
	 */

	try {
		dclasio::message::CreateProgramWithSource request(
				_id, _context->remoteId(), totalLength);

		/* Send request and data */
		for (auto computeNode : _computeNodes) {
		    assert(computeNode != nullptr);

			computeNode->sendRequest(request);
			/* Program code is sent using the data stream to avoid copying large
			 * program codes into a message before sending it. */
			computeNode->sendData(_source.size(), _source.data());
		}

		/* Await responses from all compute nodes */
		for (auto computeNode : _computeNodes) {
		    computeNode->awaitResponse(request);
			/* TODO Receive responses from *all* compute nodes, i.e., do not stop receipt on first failure */
		}

		dcl::util::Logger << dcl::util::Info
				<< "Program created from source (ID=" << _id << ')'
				<< std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}

	_context->retain();
}

_cl_program::_cl_program(cl_context context,
		const std::vector<cl_device_id>& devices, const Binaries& binaries,
		std::vector<cl_int> *binaryStatus) :
	_context(context), _isBuilt(false), _numKernels(0)
{
	if (!context) throw dclicd::Error(CL_INVALID_CONTEXT);
	if (devices.empty()) throw dclicd::Error(CL_INVALID_VALUE);
	if (binaries.empty()) throw dclicd::Error(CL_INVALID_VALUE);

	/*
	 * Derive list of compute nodes from list of devices
	 *
	 * The compute nodes hosting this program are the compute nodes that host
	 * the program's devices. These compute nodes may be a subset of the compute
	 * nodes that host the program's context.
	 */
	{
        std::set<dcl::ComputeNode *> deviceNodes;

        for (auto device : _devices) {
            deviceNodes.insert(&device->remote().getComputeNode());
        }
        _computeNodes.assign(std::begin(deviceNodes), std::end(deviceNodes));
	}

	/* The devices associated with a program are the devices that a binary has
	 * been provided for, if a program is build from binaries. */
    _devices = devices;
	_binaries.resize(_devices.size()); // _binaries must hold an entry for each device

    /* TODO Implement _cl_program constructor for binaries */
    assert(!"clCreateProgramFromBinaries not implemented");

	_context->retain();
}

_cl_program::~_cl_program() {
    dclicd::release(_context);
}

void _cl_program::destroy() {
	assert(_ref_count == 0);

	try {
		dclasio::message::DeleteProgram request(_id);
		dcl::executeCommand(_context->computeNodes(), request);
		dcl::util::Logger << dcl::util::Info
				<< "Program deleted (ID=" << _id << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

const std::vector<dcl::ComputeNode *>& _cl_program::computeNodes() const {
	return _computeNodes;
}

void _cl_program::build(
		const std::vector<cl_device_id> *deviceList,
		const char *options,
		void (*pfn_notify)(cl_program, void *),
		void *user_data) {
    const std::vector<cl_device_id>& devices = deviceList ? *deviceList : _devices;
    std::unique_ptr<dclicd::detail::ProgramBuild> programBuild;

    if (deviceList &&
            !std::includes(std::begin(_devices), std::end(_devices),
                    std::begin(devices), std::end(devices))) {
        /* At least one device in device list is not associated with program or NULL */
        throw dclicd::Error(CL_INVALID_DEVICE);
    }

    /* TODO Check if a kernel is attached to this program
     * Note that kernels may not be created on a daemon yet but have already been
     * provided to the application. Hence, only the host can check this property. */

    {
        std::lock_guard<std::mutex> lock(_buildStatusMutex);

        /*
         * Check if a program build is pending for any device in deviceList
         */
        for (auto& programBuild : _programBuilds) {
            /* no program build must be pending for any device in deviceList */
            if (programBuild->includesAnyDeviceOf(std::begin(devices), std::end(devices))) {
                if (!programBuild->isComplete()) throw dclicd::Error(CL_INVALID_OPERATION);
            }
        }

        /* Discard completed program builds */
        _programBuilds.erase(
                std::remove_if(std::begin(_programBuilds), std::end(_programBuilds),
                        std::bind(&dclicd::detail::ProgramBuild::isComplete, std::placeholders::_1)),
                        std::end(_programBuilds));
    }

    /* TODO Pre-process program to replace includes using the '-I' compile options
     * Locations for searching for header files are only valid on the host.
     * Therefore, process include directives as follows:
     * 1) '#include <file>' searches file in locations specified by '-I' and in
     * the DCL_INCLUDE_PATHS environment variable. No other (standard) locations
     * are searched, as standard locations are considered compiler-specific.
     * 2) '#include "file"' searches file in the application's execution
     * directory on the host. If file is not found, the directive is reprocessed
     * as directive of the form '#include <file>'. If file is still not found,
     * the directive's quotes are replaced by angle bracket to transform it into
     * a directive of the form '#include <file>'.
     *
     * Correct pre-processing also has to take into account conditional includes
     * and directives of the form '#include pp-token new-line', where pp-token
     * is any valid pre-processor expression. */

    /* TODO Remove '-I' compile options from the compile options string
     * Location specified by '-I' are only valid on the host and thus must not
     * be used by the daemons. */

    try {
        /* If not specified otherwise a program is build for all devices associated
         * with the program */
        programBuild.reset(new dclicd::detail::ProgramBuild(
                this, devices, options, pfn_notify, user_data));

        /* TODO Send program code to the devices' compute nodes */
    } catch (const dcl::CLError& err) {
        throw dclicd::Error(err);
    } catch (const dcl::IOException& err) {
        throw dclicd::Error(err);
    } catch (const dcl::ProtocolException& err) {
        throw dclicd::Error(err);
    }

    if (pfn_notify == nullptr) { // build program synchronously
        programBuild->wait(); // await completion of build operation
        if (programBuild->hasFailed()) {
            throw dclicd::Error(CL_BUILD_PROGRAM_FAILURE);
        }
    } else {
        std::lock_guard<std::mutex> lock(_buildStatusMutex);
        _programBuilds.push_back(std::move(programBuild)); // add programBuild to list
    }
}

void _cl_program::compile(
        const std::vector<cl_device_id> *deviceList,
        const char *options,
        const Headers *inputHeaders,
        void (*pfn_notify)(
                cl_program, void *),
        void *user_data) {
    assert(!"clCompileProgram not implemented");
}

cl_context _cl_program::context() const {
	return _context;
}

const std::vector<cl_device_id>& _cl_program::devices() const {
    return _devices;
}

void _cl_program::getInfo(
        cl_program_info param_name,
        size_t param_value_size,
        void *param_value,
        size_t *param_value_size_ret) const {
	switch (param_name) {
	case CL_PROGRAM_REFERENCE_COUNT:
		dclicd::copy_info(_ref_count, param_value_size,
				param_value, param_value_size_ret);
		break;
	case CL_PROGRAM_CONTEXT:
		dclicd::copy_info(_context, param_value_size,
				param_value, param_value_size_ret);
		break;
	case CL_PROGRAM_NUM_DEVICES:
		dclicd::copy_info(static_cast<cl_uint>(_devices.size()),
				param_value_size, param_value, param_value_size_ret);
		break;
	case CL_PROGRAM_DEVICES:
		/* deep copy device list */
		dclicd::copy_info(_devices, param_value_size, param_value,
		        param_value_size_ret);
		break;
	case CL_PROGRAM_SOURCE:
		dclicd::copy_info(_source.size() + 1, _source.c_str(),
				param_value_size, param_value, param_value_size_ret);
		break;
	case CL_PROGRAM_BINARY_SIZES:
	    if (!_source.empty()) {
	        // program has been created with source
            /* TODO Query binaries from devices */
	    }

		if (param_value) {
			/* The spec is not clear whether param_value points to an array or
			 * to the array's first element. We assume the latter as this
			 * assumption holds for other OpenCL implementations */
			auto binary_sizes = reinterpret_cast<size_t *>(param_value);

			if ((_devices.size() * sizeof(size_t)) > param_value_size) {
				throw dclicd::Error(CL_INVALID_VALUE);
			}

			/* copy binary sizes */
			for (unsigned int i = 0; i < _devices.size(); ++i) {
				binary_sizes[i] = (_binaries[i].first ? _binaries[i].second : 0);
			}
		}
		if (param_value_size_ret) {
			*param_value_size_ret = (_devices.size() * sizeof(size_t));
		}
		break;
	case CL_PROGRAM_BINARIES:
        if (!_source.empty()) {
            // program has been created with source
            /* TODO Query binaries from devices */
        }

		if (param_value) {
			/* The spec is not clear whether param_value points to an array or
			 * to the array's first element. We assume the latter as this
			 * assumption holds for other OpenCL implementations */
			unsigned char **binaries = reinterpret_cast<unsigned char **>(param_value);

			if ((_devices.size() * sizeof(unsigned char *)) > param_value_size) {
				throw dclicd::Error(CL_INVALID_VALUE);
			}

			/* copy binaries */
			for (unsigned int i = 0; i < _devices.size(); ++i) {
				/* Skip copying the program binary for a device identified by
				 * the array index, if the specific entry value in the array is
				 * NULL */
				if (binaries[i] == nullptr) continue;
				/* Do not attempt to copy unavailable binaries */
				if (_binaries[i].first == nullptr) continue;

				memcpy(binaries[i], _binaries[i].first, _binaries[i].second);
			}
		}
		if (param_value_size_ret) {
			*param_value_size_ret = (_devices.size() * sizeof(unsigned char *));
		}
		break;
	/* CL_PROGRAM_NUM_KERNELS also is available in pre-OpenCL 1.1 environments
	 * by means of a definition in cl_wwu_dcl.h. */
	case CL_PROGRAM_NUM_KERNELS:
	{
	    std::lock_guard<std::mutex> lock(_buildStatusMutex);

		/* The number of kernels is also required if only OpenCL 1.1 should be
		 * supported. This number has to be known in advance when all kernels of
		 * a program should be created with clCreateKernelsInProgram.
		 * In OpenCL 1.1 it can be obtained by calling
		 *    clCreateKernelsInProgram(program, 0, nullptr, &num_kernels)
		 * after building a program and returning this value to the host
		 * immediately. */
	    if (_isBuilt) {
            dclicd::copy_info(_numKernels, param_value_size, param_value,
                    param_value_size_ret);
	    } else {
            throw dclicd::Error(CL_INVALID_PROGRAM_EXECUTABLE);
	    }
	    break;
	}
#ifdef CL_VERSION_1_2
	case CL_PROGRAM_KERNEL_NAMES:
	{
	    std::lock_guard<std::mutex> lock(_buildStatusMutex);

	    if (_isBuilt) {
            dclicd::copy_info(_kernelNames, param_value_size, param_value,
                    param_value_size_ret);
	    } else {
            throw dclicd::Error(CL_INVALID_PROGRAM_EXECUTABLE);
	    }
		break;
	}
#endif
	default:
		throw dclicd::Error(CL_INVALID_VALUE);
	}
}

void _cl_program::getBuildInfo(
		cl_device_id device,
        cl_program_build_info param_name,
        size_t param_value_size,
        void *param_value,
        size_t *param_value_size_ret) const {
	if (std::find(std::begin(_devices), std::end(_devices), device) == std::end(_devices)) {
		throw dclicd::Error(CL_INVALID_DEVICE);
	}

    std::lock_guard<std::mutex> lock(_buildStatusMutex);
	/* (initialize and) obtain build info of program for device */
	auto& buildInfo = _buildInfo[device];

	switch (param_name) {
	case CL_PROGRAM_BUILD_STATUS:
		dclicd::copy_info(buildInfo.status, param_value_size, param_value,
				param_value_size_ret);
		break;
	case CL_PROGRAM_BUILD_OPTIONS:
		dclicd::copy_info(buildInfo.options, param_value_size, param_value,
				param_value_size_ret);
		break;
	case CL_PROGRAM_BUILD_LOG:
		if (buildInfo.status == CL_BUILD_NONE) {
			dclicd::copy_info("", param_value_size, param_value,
					param_value_size_ret);
		} else {
            /* TODO Fully implement _cl_program::getBuildInfo */
            assert(!"_cl_program::getBuildInfo not fully implemented");
#if 0
            /*
             * Query build log from compute node
             */
			try {
				dclasio::message::GetProgramBuildLog request(_id, device->remote().id(), param_value_size);
				std::unique_ptr<dclasio::message::ProgramBuildLog> response(
						static_cast<dclasio::message::ProgramBuildLog *>(
								device->getComputeNode().executeCommand(request, dclasio::message::ProgramBuildLog::TYPE)));

				if (param_value) {
					assert(size <= param_value_size);
					/* Receive program build log (blocking operation) */
					device->getComputeNode().receiveData(response->size(), param_value)->wait();
				}
				if (param_value_size_ret) {
					*param_value_size_ret = response->size();
				}
			} catch (const dcl::IOException& err) {
				throw dclicd::Error(err);
			} catch (const dcl::ProtocolException& err) {
				throw dclicd::Error(err);
			}
#endif
		}
		break;
	default:
		throw dclicd::Error(CL_INVALID_VALUE);
	}
}

bool _cl_program::hasDevice(cl_device_id device) const {
    return (std::find(std::begin(_devices), std::end(_devices), device) != std::end(_devices));
}

void _cl_program::onBuildStatusChanged(
        cl_device_id device,
        cl_build_status status,
        const std::string& options) {
	std::lock_guard<std::mutex> lock(_buildStatusMutex);

	/* Set \c _isBuilt to true if all compute nodes reported completion of program
	 * build, otherwise return */

    /* TODO Update build status and save build info (status and options) for
     * devices of completed (!) build operation
    for (auto device : deviceList) {
        _buildInfo[device] = dclicd::detail::ProgramBuildInfo(status, options);
    }
     */
}
