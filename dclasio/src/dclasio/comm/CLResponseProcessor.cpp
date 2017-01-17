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
 * \file CLResponseProcessor.cpp
 *
 * \date 2014-05-01
 * \author Philipp Kegel
 */

#include "CLResponseProcessor.h"

#include "../CommunicationManagerImpl.h"
#include "../ComputeNodeImpl.h"

#include "../message/DeviceIDsResponse.h"
#include "../message/DeviceInfosResponse.h"

#include <dclasio/message/ErrorResponse.h>
#include <dclasio/message/EventProfilingInfosResponse.h>
#include <dclasio/message/InfoResponse.h>
#include <dclasio/message/Response.h>

#include <dcl/DCLTypes.h>

#include <dcl/util/Logger.h>

#include <cassert>
#include <memory>
#include <ostream>
#include <utility>

namespace dclasio {
namespace comm {

CLResponseProcessor::CLResponseProcessor(
        CommunicationManagerImpl& connectionManager) :
        _communicationManager(connectionManager) {
}

CLResponseProcessor::~CLResponseProcessor() {
}

bool CLResponseProcessor::dispatch(
        const message::Response& message,
        dcl::process_id pid) {
    std::unique_ptr<message::Response> response;

    ComputeNodeImpl *computeNode = _communicationManager.get_compute_node(pid);
    assert(computeNode && "No compute node for response");
    if (!computeNode)
        return false;

    switch (message.get_type()) {
    // command responses
    case message::DefaultResponse::TYPE:
        response.reset(new message::DefaultResponse(
                static_cast<const message::DefaultResponse&>(message)));
        break;
    case message::DeviceIDsResponse::TYPE:
        response.reset(new message::DeviceIDsResponse(
                static_cast<const message::DeviceIDsResponse&>(message)));
        break;
    case message::DeviceInfosResponse::TYPE:
        response.reset(new message::DeviceInfosResponse(
                static_cast<const message::DeviceInfosResponse&>(message)));
        break;
    case message::ErrorResponse::TYPE:
        response.reset(new message::ErrorResponse(
                static_cast<const message::ErrorResponse&>(message)));
        break;
    case message::EventProfilingInfosReponse::TYPE:
        response.reset(new message::EventProfilingInfosReponse(
                static_cast<const message::EventProfilingInfosReponse&>(message)));
        break;
    case message::InfoResponse::TYPE:
        response.reset(new message::InfoResponse(
                static_cast<const message::InfoResponse&>(message)));
        break;

    default: // unknown message
        return false;
    }

    assert(response && "No response");
    if (response) {
        // move response into the response buffer associated with sender
        computeNode->responseBuffer().put(std::move(response));
        dcl::util::Logger << dcl::util::Verbose
                << "Received response from compute node" << std::endl;
    }

    return true;
}

} /* namespace comm */
} /* namespace dclasio */
