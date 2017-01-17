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
 * \file ComputeNodeImpl.cpp
 *
 * \date 2011-10-30
 * \author Philipp Kegel
 */

#include "ComputeNodeImpl.h"

#include "CommunicationManagerImpl.h"
#include "DCLAsioTypes.h"
#include "DeviceImpl.h"

#include "comm/CLEventProcessor.h"
#include "comm/DataDispatcher.h"
#include "comm/MessageDispatcher.h"
#include "comm/MessageQueue.h"

#include "message/DeviceIDsResponse.h"
#include "message/DeviceInfosResponse.h"
#include "message/GetDeviceIDs.h"

#include <dclasio/message/ErrorResponse.h>
#include <dclasio/message/EventProfilingInfosResponse.h>
#include <dclasio/message/InfoResponse.h>
#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>

#include <dcl/Binary.h>
#include <dcl/CLError.h>
#include <dcl/DataTransfer.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>
#include <dcl/Device.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <boost/asio/io_service.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <vector>

namespace dclasio {

void ComputeNodeImpl::updateDevices(
        const std::vector<ComputeNodeImpl *>& computeNodes) {
	std::vector<ComputeNodeImpl *> connectedComputeNodes(computeNodes);

	/* Send request to all compute nodes, such that the command is executed
	 * simultaneously */
	message::GetDeviceIDs request(static_cast<cl_device_type>(CL_DEVICE_TYPE_ALL));
	for (auto i = std::begin(connectedComputeNodes); i != std::end(connectedComputeNodes);) {
		try {
			(*i)->sendRequest(request);
			++i;
		} catch (const dcl::DCLException& err) {
			i = connectedComputeNodes.erase(i);
			dcl::util::Logger << dcl::util::Warning
			        << err.what() << std::endl;
		}
	}

	// Await responses from all compute nodes
	for (auto computeNode : connectedComputeNodes) {
		try {
			std::unique_ptr<message::DeviceIDsResponse> response(
					static_cast<message::DeviceIDsResponse *>(
							computeNode->awaitResponse(request, message::DeviceIDsResponse::TYPE).release()));
			assert(response != nullptr); // response must not be NULL
		    dcl::util::Logger << dcl::util::Info
		            << "Found " << response->deviceIds.size()
		            << " devices on compute node " << computeNode->url() << std::endl;

			computeNode->updateDevices(response->deviceIds);
		} catch (const dcl::DCLException& err) {
			dcl::util::Logger << dcl::util::Error << err.what() << std::endl;
		}
	}
}

void ComputeNodeImpl::connect(
        const std::vector<ComputeNodeImpl *>& computeNodes,
        Type localProcessType,
        dcl::process_id pid) {
	std::vector<ComputeNodeImpl *> connectingComputeNodes(computeNodes);

	auto i = std::begin(connectingComputeNodes);
	while (i != std::end(connectingComputeNodes)) {
	    // Skip connected compute nodes
	    if ((*i)->isConnected()) {
            i = connectingComputeNodes.erase(i);
            continue;
	    }

		try {
			(*i)->connectMessageQueue(localProcessType, pid,
                    CommunicationManagerImpl::DEFAULT_CONNECTION_TIMEOUT);
			++i;
		} catch (const dcl::DCLException& err) {
			i = connectingComputeNodes.erase(i);
			dcl::util::Logger << dcl::util::Warning << err.what() << std::endl;
		}
	}

	i = std::begin(connectingComputeNodes);
	while (i != std::end(connectingComputeNodes)) {
		try {
			(*i)->connectDataStream(pid,
			        CommunicationManagerImpl::DEFAULT_CONNECTION_TIMEOUT);
			++i;
		} catch (const dcl::DCLException& err) {
			i = connectingComputeNodes.erase(i);
			dcl::util::Logger << dcl::util::Error << err.what() << std::endl;
		}
	}
}

void ComputeNodeImpl::awaitConnection(
        const std::vector<ComputeNodeImpl *>& computeNodes) {
    auto deadline = std::chrono::system_clock::now() +
            CommunicationManagerImpl::DEFAULT_CONNECTION_TIMEOUT;
    for (auto computeNode : computeNodes) {
        computeNode->awaitConnectionStatus(ProcessImpl::ConnectionStatus::CONNECTED, deadline);
    }
}

ComputeNodeImpl::ComputeNodeImpl(
        dcl::process_id pid,
        comm::MessageDispatcher& messageDispatcher,
        comm::DataDispatcher& dataDispatcher,
        comm::message_queue& messageQueue) :
    ProcessImpl(pid, messageDispatcher, dataDispatcher, messageQueue)
{
    dcl::util::Logger << dcl::util::Debug
            << "Created compute node '" << url() << '\'' << std::endl;
}

ComputeNodeImpl::ComputeNodeImpl(
        comm::MessageDispatcher& messageDispatcher,
        comm::DataDispatcher& dataDispatcher,
        const endpoint_type& endpoint) :
    ProcessImpl(messageDispatcher, dataDispatcher, endpoint)
{
    dcl::util::Logger << dcl::util::Debug
            << "Created compute node '" << url() << '\'' << std::endl;
}

ComputeNodeImpl::~ComputeNodeImpl() {
}

void ComputeNodeImpl::updateDevices() {
    message::GetDeviceIDs request((cl_device_type) CL_DEVICE_TYPE_ALL);
	std::unique_ptr<message::DeviceIDsResponse> response(
			static_cast<message::DeviceIDsResponse *>(
					executeCommand(request, message::DeviceIDsResponse::TYPE).release()));
	assert(response != nullptr); // response must not be NULL
	dcl::util::Logger << dcl::util::Info
			<< "Found " << response->deviceIds.size()
			<< " devices on compute node " << url() << std::endl;

    updateDevices(response->deviceIds);
}

void ComputeNodeImpl::updateDevices(
		const std::vector<dcl::object_id>& deviceIds) {
	std::lock_guard<std::recursive_mutex> lock(_devicesMutex);

	/* FIXME Do not clear but update the device list
	 * Add new devices; mark as valid
	 * Mark existing devices valid or invalid */
	if (_devices) {
		_devices->clear();
	} else {
		_devices.reset(new std::vector<std::unique_ptr<DeviceImpl>>());
	}

    _devices->reserve(deviceIds.size());
	for (auto deviceId : deviceIds) {
		_devices->emplace_back(new DeviceImpl(deviceId, *this));
		// TODO Register devices
	}
}

void ComputeNodeImpl::connect(Type localProcessType, dcl::process_id pid) {
    auto deadline = std::chrono::system_clock::now() +
            CommunicationManagerImpl::DEFAULT_CONNECTION_TIMEOUT;
    connectMessageQueue(localProcessType, pid, deadline);
    connectDataStream(pid, deadline);
}

void ComputeNodeImpl::getDevices(std::vector<dcl::Device *>& devices) {
	std::lock_guard<std::recursive_mutex> lock(_devicesMutex);

	/* Device list may be uninitialized, if the compute node's device is queried
	 * for the first time. */
	if (!_devices) {
		updateDevices();
	}
	assert(_devices != nullptr);
	
	/* Must not return a reference to the device list as it may change
	 * asynchronously */
	for (const auto& device : *_devices) {
	    devices.push_back(device.get());
	}
}

void ComputeNodeImpl::getInfo(
        cl_compute_node_info_WWU param_name,
        dcl::Binary& param) const {
    // TODO Implement ComputeNodeImpl::getInfo
    assert(!"ComputeNodeImpl::getInfo not implemented");

    dcl::util::Logger << dcl::util::Info
            << "Got compute node infos from '" << url()
            << '\'' << std::endl;
}

void ComputeNodeImpl::sendRequest(message::Request& request) const {
    /* Do not use sendMessage which should be used for dOpenCL messages only
     * Thus, sending messages and sending requests can be properly
     * distinguished. */
    /* TODO Check message queue before sending message
    if (!_messageQueue.isConnected()) {
        throw dcl::IOException("No connection to compute node at " + url());
    }
     */
    _messageQueue.send_message(request);
}

std::unique_ptr<message::Response> ComputeNodeImpl::awaitResponse(
		const message::Request& request, message::Response::class_type responseType) {
    std::unique_ptr<message::Response> response;

	try {
#ifndef NDEBUG
        // Wait for responses indefinitely during debug
		response = _responseBuffer.get(request);
#else
        /* FIXME Allow for blocking operation (e.g. finish) to wait indefinitely
         * Blocking operations may require more time than the default response
         * timeout. Hence no IO exception must be reported.
         * ALTERNATIVELY implement blocking operations asynchronously. */
        response = _responseBuffer.get(request, DEFAULT_RESPONSE_TIMEOUT);
#endif
	} catch (const dcl::ThreadInterrupted&) {
		// ignore interrupt; treat as timeout
	}
	if (!response) {
		throw dcl::IOException("No response from compute node at " + url());
	}

	// Intercept error message
	if (response->get_type() == message::ErrorResponse::TYPE) {
		throw dcl::CLError(response->get_errcode());
	}
	// Intercept illegal response (protocol error)
	if (response->get_type() != responseType) {
		throw dcl::ProtocolException("Illegal response from compute node at " + url());
	}

	return response;
}

void ComputeNodeImpl::awaitResponse(const message::Request& request) {
	awaitResponse(request, message::DefaultResponse::TYPE);
}

std::unique_ptr<message::Response> ComputeNodeImpl::executeCommand(
        message::Request& request, message::Response::class_type responseType) {
//#ifndef NDEBUG
#if 0
    sendRequest(request);
    dcl::util::Logger << dcl::util::Debug
            << "\tsent request (request ID=" << request.id
            << ", type=" << request.getType() << ')'
            << std::endl;

    std::unique_ptr<message::Response> response = awaitResponse(request, responseType);
    dcl::util::Logger << dcl::util::Debug
            << "\treceived response (request ID=" << response->requestId
            << ", type=" << response->getType() << ')'
            << std::endl;

    return response;
#else
	sendRequest(request);
	return awaitResponse(request, responseType);
#endif
}

void ComputeNodeImpl::executeCommand(message::Request& request) {
	executeCommand(request, message::DefaultResponse::TYPE);
}

comm::ResponseBuffer& ComputeNodeImpl::responseBuffer() {
	return _responseBuffer;
}

/* ****************************************************************************/

void sendMessage(
        const std::vector<ComputeNodeImpl *>& computeNodes,
        message::Message& message) {
    // TODO Use communication group to broadcast message
    for (auto computeNode : computeNodes) {
        computeNode->sendMessage(message);
    }
}

void sendRequest(
        const std::vector<ComputeNodeImpl *>& computeNodes,
        message::Request& request) {
    // TODO Use communication group to broadcast request
    for (auto computeNode : computeNodes) {
        computeNode->sendRequest(request);
    }
}

void executeCommand(
        const std::vector<ComputeNodeImpl *>& computeNodes,
        message::Request& request,
        message::Response::class_type responseType,
        std::vector<std::unique_ptr<message::Response>> *responses) {
    /* Send request to all compute nodes, such that the command is executed
     * simultaneously */
    sendRequest(computeNodes, request);

    // Await responses from all compute nodes
    // TODO Receive message from *all* compute nodes, i.e., do not stop receipt on first failure
    if (responses) {
        responses->clear();
        responses->reserve(computeNodes.size());

        for (auto computeNode : computeNodes) {
            // move response into responses
            responses->push_back(
                    computeNode->awaitResponse(request, responseType));
        }
    } else {
        for (auto computeNode : computeNodes) {
            // discard response
            computeNode->awaitResponse(request, responseType);
        }
    }
}

} /* namespace dclasio */
