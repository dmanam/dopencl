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

/*!
 * \file    Kernel.cpp
 *
 * \date    2011-06-08
 * \author  Karsten Jeschkies
 * \author  Philipp Kegel
 */

#include "Kernel.h"

#include "Context.h"
#include "Device.h"
#include "Memory.h"
#include "Program.h"
#include "Retainable.h"

#include "dclicd/Error.h"
#include "dclicd/utility.h"

#include <dclasio/message/CreateKernel.h>
#include <dclasio/message/CreateKernelsInProgram.h>
#include <dclasio/message/DeleteKernel.h>
#include <dclasio/message/GetKernelInfo.h>
#include <dclasio/message/InfoResponse.h>
#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>
#include <dclasio/message/SetKernelArg.h>

#include <dcl/Binary.h>
#include <dcl/ComputeNode.h>
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

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>


_cl_kernel::_cl_kernel(
		cl_program program,
		const char *kernelName) :
	_program(program) {

	if (!program) throw dclicd::Error(CL_INVALID_PROGRAM);
	if (!kernelName) throw dclicd::Error(CL_INVALID_VALUE);

	try {
		dclasio::message::CreateKernel request(_id, program->remoteId(), kernelName);
		/* TODO Only create kernels on compute nodes where the associated program has been build.
		 * If no such compute nodes exists, throw CL_INVALID_PROGRAM_EXECUTABLE */
		dcl::executeCommand(program->computeNodes(), request);
		dcl::util::Logger << dcl::util::Info
				<< "Kernel created (ID=" << _id
				<< ", name=" << kernelName
				<< ')' << std::endl;

        /* TODO Obtain function name and number of kernel arguments from response
         * Use #args to create an appropriately sized _modifiedMemoryObjects */
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}

	_program->retain();
}

_cl_kernel::_cl_kernel(dcl::object_id id, cl_program program) :
	dcl::Remote(id), _program(program) {
	assert(program != nullptr); // program must not be NULL
	_program->retain();
}

_cl_kernel::~_cl_kernel() {
    dclicd::release(_program);
}

void _cl_kernel::destroy() {
	assert(_ref_count == 0);

	try {
		dclasio::message::DeleteKernel request(_id);
		dcl::executeCommand(_program->computeNodes(), request);
		dcl::util::Logger << dcl::util::Info
				<< "Kernel deleted (ID=" << _id << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

void _cl_kernel::setArgument(cl_uint index, size_t size, const void *value) {
//    cl_uint numArgs;
	std::unique_ptr<dclasio::message::Request> request;

	/* Validate argument index using number of kernel arguments from kernel info */
//	getInfo(CL_KERNEL_NUM_ARGS, sizeof(numArgs), &numArgs, nullptr);
//	if (index >= numArgs) throw dclicd::Error(CL_INVALID_ARG_INDEX);
	/* TODO Validate argument size using type name from kernel argument info (available as of OpenCL 1.2) */

    /* TODO Determine argument type using type name and address qualifier from kernel argument info (available as of OpenCL 1.2) */

	if (value == nullptr) {
		/* argument could be buffer object which should initialized with NULL
		 * or could be declared with the __local qualifier */
		request.reset(new dclasio::message::SetKernelArgMemObject(
		        _id, index, size));
	} else {
		if (size == sizeof(cl_mem)) {
			/* value could be a pointer to buffer or image
			 * check if value points to valid memory object */
			cl_mem mem = _cl_mem::findMemObject(*((cl_mem *) value));
			if (mem) {
				/* value points to memory object */
				request.reset(new dclasio::message::SetKernelArgMemObject(
				        _id, index, mem->remoteId()));

				if (mem->isOutput()) {
				    /* If a writable (CL_MEM_WRITE_ONLY, CL_MEM_READ_WRITE)
				     * memory object is set as kernel argument, it is assumed
				     * that the kernel will modify this memory object. */
				    /* TODO Set appropriate size of memory object list during kernel creation */
//				    assert(index < _writeMemoryObjects.size());
				    if (_writeMemoryObjects.size() <= index) {
				        _writeMemoryObjects.resize(index + 1);
				    }
				    _writeMemoryObjects[index] = mem;
				}
			}
		}
	}

	if (!request) {
		/* value points to a regular variable */
		request.reset(new dclasio::message::SetKernelArgBinary(
		        _id, index, size, value));
	}

	try {
		dcl::executeCommand(_program->computeNodes(), *request);
		dcl::util::Logger << dcl::util::Info
				<< "Kernel argument set (ID=" << _id << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

std::vector<cl_mem> _cl_kernel::writeMemoryObjects() const {
    std::set<cl_mem> writeMemoryObjects;

    /* copy memory objects from argument list to set to remove duplicates */
    for (auto mem : _writeMemoryObjects) {
        /* ignore empty (NULL) entries */
        if (mem) writeMemoryObjects.insert(mem);
    }

    return std::vector<cl_mem>(std::begin(writeMemoryObjects),
            std::end(writeMemoryObjects));
}

/*
 * Sends a 'create kernels in program' request to each compute node associated with program.
 *
 * All compute nodes receive a list of unique kernel IDs, which will be assigned
 * to the created kernels on the compute node.
 * If the number of kernel IDs is less than the number of kernels, an error
 * (CL_INVALID_VALUE) is thrown.
 */
void _cl_kernel::createKernelsInProgram(
		cl_program program,
		std::vector<cl_kernel>& kernels) {
	std::vector<dcl::object_id> kernelIds;
	cl_uint numKernels;

	if (!program) throw dclicd::Error(CL_INVALID_PROGRAM);

	/* _cl_program::getInfo will throw CL_INVALID_PROGRAM_EXECUTABLE if there
	 * is no successfully built executable for any device in program. */
	program->getInfo(CL_PROGRAM_NUM_KERNELS, sizeof(numKernels), &numKernels, nullptr);

	/* create kernel IDs */
	kernelIds.resize(numKernels);
	for (unsigned int i = 0; i < numKernels; ++i) {
		kernelIds[i] = dcl::Remote::generateId();
	}

	/* send command to compute nodes */
	try {
		dclasio::message::CreateKernelsInProgram request(program->remoteId(), kernelIds);
        /* TODO Only create kernels on compute nodes where the associated program has been build.
         * If no such compute nodes exists, throw CL_INVALID_PROGRAM_EXECUTABLE */
        dcl::executeCommand(program->computeNodes(), request);
		dcl::util::Logger << dcl::util::Info
				<< "Kernels in program created (program ID=" << program->remoteId()
				<< ", #kernels=" << numKernels
				<< ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}

	/* create kernels */
	kernels.resize(numKernels);
	for (unsigned int i = 0; i < numKernels; ++i) {
		kernels[i] = new _cl_kernel(kernelIds[i], program);
	}
}

cl_program _cl_kernel::program() const {
	return _program;
}

void _cl_kernel::getInfo(
		cl_kernel_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) const {
	switch (param_name) {
	case CL_KERNEL_REFERENCE_COUNT:
		dclicd::copy_info(_ref_count, param_value_size, param_value,
				param_value_size_ret);
		break;
	case CL_KERNEL_CONTEXT:
		dclicd::copy_info(_program->context(), param_value_size, param_value,
				param_value_size_ret);
		break;
	case CL_KERNEL_PROGRAM:
		dclicd::copy_info(_program, param_value_size, param_value,
				param_value_size_ret);
		break;
	default:
	{
        std::lock_guard<std::mutex> lock(_infoCacheMutex);

		auto i = _infoCache.find(param_name); // search kernel info in cache
		if (i == std::end(_infoCache)) {
	        /*
	         * Cache miss: query info from compute node
			 * The kernel is on many compute nodes. Pick first one to query
			 * kernel info from.
			 */
			auto computeNode = _program->computeNodes().front();
			dclasio::message::GetKernelInfo request(_id, param_name);

			try {
	            std::unique_ptr<dclasio::message::InfoResponse> response(
	                    static_cast<dclasio::message::InfoResponse *>(
	                            computeNode->executeCommand(request, dclasio::message::InfoResponse::TYPE).release()));
                /* add kernel info to cache */
                i = _infoCache.insert(std::make_pair(param_name, response->param())).first;

		        dcl::util::Logger << dcl::util::Info
		                << "Got kernel info (ID=" << _id
		                << ')' << std::endl;
			} catch (const dcl::CLError& err) {
				throw dclicd::Error(err);
			} catch (const dcl::IOException& err) {
				throw dclicd::Error(err);
			} catch (const dcl::ProtocolException& err) {
				throw dclicd::Error(err);
			}
        }

        dclicd::copy_info(i->second, param_value_size, param_value,
                        param_value_size_ret);
	}
	/* no break */
	}
}

#if defined(CL_VERSION_1_2)
void _cl_kernel::getArgInfo(
        cl_uint arg_indx,
        cl_kernel_arg_info param_name,
        size_t param_value_size,
        void *param_value,
        size_t *param_value_size_ret) const {
    assert(!"_cl_kernel::getArgInfo not implemented");
}
#endif // #if defined(CL_VERSION_1_2)

void _cl_kernel::getWorkGroupInfo(
        cl_device_id device,
        cl_kernel_work_group_info param_name,
        size_t param_value_size,
        void *param_value,
        size_t *param_value_size_ret) const {
    if (!device) {
        const std::vector<cl_device_id>& devices = _program->devices();
        if (devices.size() == 1) {
            /* If only a single device is associated with this kernel (i.e. with
             * its associated program) device can be NULL. */
            device = devices.front();
        } else {
            throw dclicd::Error(CL_INVALID_DEVICE);
        }
    } else {
        /* device must be associated with kernel, i.e. with is associated
         * program */
        if (!_program->hasDevice(device)) {
            throw dclicd::Error(CL_INVALID_DEVICE);
        }
    }

    {
        std::lock_guard<std::mutex> lock(_infoCacheMutex);

        /* (initialize and) obtain work group info cache for device */
        auto& infoCache = _workGroupInfoCaches[device];
        auto i = infoCache.find(param_name); // search work group info in cache
        if (i == std::end(infoCache)) {
            /*
             * Cache miss: query work group info from compute node
             */
            dcl::ComputeNode& computeNode = device->remote().getComputeNode();
            dclasio::message::GetKernelWorkGroupInfo request(_id, device->remote().getId(), param_name);

            try {
                std::unique_ptr<dclasio::message::InfoResponse> response(
                        static_cast<dclasio::message::InfoResponse *>(
                                computeNode.executeCommand(request, dclasio::message::InfoResponse::TYPE).release()));
                /* add work group info to cache */
                i = infoCache.insert(std::make_pair(param_name, response->param())).first;

                dcl::util::Logger << dcl::util::Info
                        << "Got kernel work group info (kernel ID=" << _id
                        << ", device ID=" << device->remote().getId()
                        << ')' << std::endl;
            } catch (const dcl::CLError& err) {
                throw dclicd::Error(err);
            } catch (const dcl::IOException& err) {
                throw dclicd::Error(err);
            } catch (const dcl::ProtocolException& err) {
                throw dclicd::Error(err);
            }
        }

        dclicd::copy_info(i->second, param_value_size, param_value,
                        param_value_size_ret);
    }
}
