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
 * \file ProgramBuild.cpp
 *
 * \date 2012-08-04
 * \author Philipp Kegel
 */

#include "ProgramBuild.h"

#include "../../Context.h"
#include "../../Device.h"
#include "../../Platform.h"
#include "../../Program.h"

#include "../Error.h"

#include <dclasio/message/BuildProgram.h>

#include <dcl/CLError.h>
#include <dcl/CLObjectRegistry.h>
#include <dcl/ComputeNode.h>
#include <dcl/DCLTypes.h>
#include <dcl/DCLException.h>
#include <dcl/ProgramBuildListener.h>
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
#include <condition_variable>
#include <map>
#include <mutex>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace dclicd {

namespace detail {

ProgramBuild::ProgramBuild(
        cl_program program,
        const std::vector<cl_device_id>& devices,
        const char *options,
        void (*pfn_notify)(cl_program, void *),
        void *user_data) :
    _program(program), _devices(devices), _options(options ? options : ""),
    _pfnNotify(pfn_notify), _userData(user_data), _buildStatus(CL_BUILD_NONE)
{
    assert(program != nullptr); // constructor is only called internally
    assert(!devices.empty()); // program must be build for at least one device
    if (pfn_notify == nullptr && user_data != nullptr) {
        throw Error(CL_INVALID_VALUE);
    }

    /* register build *before* sending request;
     * otherwise a build completion message might be lost */
    _program->context()->getPlatform()->remote().objectRegistry().bind<dcl::ProgramBuildListener>(_id, *this);

    submit();

    /* TODO Trigger callback asynchronously */
    if (_buildStatus == CL_BUILD_SUCCESS && _pfnNotify) {
        _pfnNotify(_program, _userData);
    }
}

ProgramBuild::~ProgramBuild() {
    /* Deregister program build */
    _program->context()->getPlatform()->remote().objectRegistry().unbind<dcl::ProgramBuildListener>(_id);
}

bool ProgramBuild::isComplete() const {
    std::lock_guard<std::mutex> lock(_buildStatusMutex);
    return testComplete();
}

bool ProgramBuild::hasFailed() const {
    std::lock_guard<std::mutex> lock(_buildStatusMutex);
    return _buildStatus == CL_BUILD_PROGRAM_FAILURE;
}

void ProgramBuild::wait() {
    std::lock_guard<std::mutex> lock(_buildStatusMutex);
    while (!testComplete()) {
        _buildCompleted.wait(_buildStatusMutex);
    }
}

void ProgramBuild::onComplete(
        const std::vector<dcl::Device *>& devices,
        const std::vector<cl_build_status>& buildStatus) {
    assert(devices.size() == buildStatus.size() && "Number of devices and build status do not match");

    /* TODO Update build status and save build info (status and options) for
     * devices of completed (!) build operation
    for (unsigned int i = 0; i < devices->size(); ++i) {
        _program->onBuildStatusChanged(devices[i], buildStatus[i], _options);
    }
     */
    /* TODO Set _buildStatus = CL_BUILD_SUCCESS, when all computes nodes finished building the program */

    {
        std::lock_guard<std::mutex> lock(_buildStatusMutex);
        if (testComplete()) {
            /* Trigger callback if all compute nodes reported completion of program
             * build */
            if (_pfnNotify) {
                _pfnNotify(_program, _userData);
            }
        }
        _buildCompleted.notify_all();
    }
}

void ProgramBuild::submit() {
    std::map<dcl::ComputeNode *, std::vector<dcl::object_id>> nodeDeviceIds;

    /*
     * Create compute nodes' lists of device IDs
     */
    for (auto device : _devices) {
        assert(device != nullptr); // devices must not be NULL
        nodeDeviceIds[&device->remote().getComputeNode()].push_back(device->remote().getId());
    }

    {
        std::lock_guard<std::mutex> lock(_buildStatusMutex);
        std::vector<std::pair<dcl::ComputeNode *, dclasio::message::BuildProgram>> requests;
        cl_int err = CL_SUCCESS; // assume successful command submission

        /*
         * Create and send requests
         */
        for (auto i : nodeDeviceIds) {
            auto computeNode = i.first;
            /* TODO Avoid copying 'build program' requests */
            dclasio::message::BuildProgram request(_program->remoteId(), i.second, _options, _id);

            /* Send requests to *all* compute nodes, i.e. do not stop on failure */
            try {
                computeNode->sendRequest(request);
                requests.push_back(std::make_pair(computeNode, request)); // save pending request
            } catch (const dcl::IOException&) {
                err = CL_IO_ERROR_WWU;
            } catch (const dcl::ProtocolException&) {
                err = CL_PROTOCOL_ERROR_WWU;
            }
        }
        if (requests.empty()) {
            /* no compute node will build the program; the program build failed */
            assert(err != CL_SUCCESS && "Invalid program build");
            _buildStatus = CL_BUILD_PROGRAM_FAILURE;
            _buildCompleted.notify_all();
            throw dclicd::Error(err);
        }

        /*
         * Await responses from all compute nodes
         * Also await responses if request failed on some compute nodes.
         */
        for (auto i : requests) {
            auto computeNode = i.first;
            cl_int nodeErr = CL_SUCCESS; // assume successful response from compute node

            /* Receive responses from *all* compute nodes, i.e. do not stop on failure */
            try {
                computeNode->awaitResponse(i.second);
                _computeNodes.push_back(computeNode); // compute node is now building program

                /* TODO Update build info (status and options) for devices on compute node
                for (auto device : devices) {
                    _program->onBuildStatusChanged(device, CL_BUILD_IN_PROGRESS, _options));
                }
                 */
            } catch (const dcl::CLError& err) {
                nodeErr = err.err();
            } catch (const dcl::IOException&) {
                nodeErr = CL_IO_ERROR_WWU;
            } catch (const dcl::ProtocolException&) {
                nodeErr = CL_PROTOCOL_ERROR_WWU;
            }

            if (nodeErr != CL_SUCCESS && err == CL_SUCCESS) {
                err = nodeErr; // return error from first failure
            }
        }
        if (err != CL_SUCCESS) { // program build has not been submitted to all compute nodes
            /* FIXME Program build did not fail completely, i.e., is is *not finished* yet */
//            _buildStatus = CL_BUILD_PROGRAM_FAILURE;
        }
        /* TODO Implement asynchronous program build */
//        _buildStatus = CL_BUILD_IN_PROGRESS;
        _buildStatus = CL_BUILD_SUCCESS;

        dcl::util::Logger << dcl::util::Info
                << "Program build submitted (program ID=" << _program->remoteId()
                << ", build ID=" << _id
                << ')' << std::endl;
    }
}

bool ProgramBuild::testComplete() const {
    return (_buildStatus == CL_BUILD_SUCCESS ||
            _buildStatus == CL_BUILD_PROGRAM_FAILURE);
}

} /* namespace detail */

} /* namespace dclicd */
