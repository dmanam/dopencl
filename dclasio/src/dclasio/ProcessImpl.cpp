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
 * \file ProcessImpl.cpp
 *
 * \date 2012-03-18
 * \author Philipp Kegel
 */

#include "ProcessImpl.h"

#include "DCLAsioTypes.h"

#include "comm/DataDispatcher.h"
#include "comm/DataStream.h"
#include "comm/MessageDispatcher.h"
#include "comm/MessageQueue.h"

#include <dclasio/message/Message.h>

#include <dcl/DataTransfer.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>

#include <boost/asio/ip/tcp.hpp>

#include <boost/system/error_code.hpp>

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <sstream>

namespace dclasio {

const std::chrono::seconds ProcessImpl::DEFAULT_RESPONSE_TIMEOUT = std::chrono::seconds(3);

ProcessImpl::ProcessImpl(
        dcl::process_id id,
        comm::MessageDispatcher& messageDispatcher,
        comm::DataDispatcher& dataDispatcher,
        comm::message_queue& msgq) :
        _pid(id),
        _messageDispatcher(messageDispatcher), _messageQueue(msgq),
        _dataDispatcher(dataDispatcher), _dataStream(nullptr),
        _connectionStatus(ConnectionStatus::MESSAGE_QUEUE_CONNECTED) {
    assert(_pid != 0 && "Invalid process ID");
}

ProcessImpl::ProcessImpl(
        comm::MessageDispatcher& messageDispatcher,
        comm::DataDispatcher& dataDispatcher,
        const endpoint_type& endpoint) :
        _pid(0) /* unknown process ID */,
        _messageDispatcher(messageDispatcher),
        _messageQueue(*_messageDispatcher.create_message_queue(endpoint)),
        _dataDispatcher(dataDispatcher),
        _connectionStatus(ConnectionStatus::DISCONNECTED) {
    endpoint_type data_endpoint(endpoint.address(), endpoint.port() + 100);
    _dataStream = _dataDispatcher.create_data_stream(data_endpoint);
}

ProcessImpl::~ProcessImpl() {
    disconnect();

    _messageDispatcher.destroy_message_queue(&_messageQueue);
}

dcl::process_id ProcessImpl::get_id() const {
    return _pid;
}

void ProcessImpl::disconnect() {
    std::lock_guard<std::recursive_mutex> lock(_connectionStatusMutex);

    // disconnect message queue
    _messageQueue.disconnect();

    // detach data stream from process
    if (_dataStream) {
        _dataDispatcher.destroy_data_stream(_dataStream);
        _dataStream = nullptr;
    }

    _connectionStatus = ConnectionStatus::DISCONNECTED;
    _connectionStatusChanged.notify_all();
}

bool ProcessImpl::isConnected() {
    std::lock_guard<std::recursive_mutex> lock(_connectionStatusMutex);
    return _connectionStatus == ConnectionStatus::CONNECTED;
}

const std::string& ProcessImpl::url() const {
    if (_url.empty()) {
        std::stringstream ss;

        // TODO Generate process URL from message queue's remote endpoint
        /*
        // create URL string once
        if (!(ss << _messageQueue.getHostname() << ':' << _messageQueue.getPort()) ||
                !(ss >> _url) ||
                !((ss >> std::ws).eof())) // skip whitespace and check if EOF flag has been set
            _url.clear();
         */
    }

    return _url;
}

void ProcessImpl::sendMessage(const message::Message& message) const {
    /* TODO Check message queue before sending message
    if (!_messageQueue.isConnected()) {
        throw dcl::IOException("No connection to process at " + url());
    }
     */
    _messageQueue.send_message(message);
}

std::shared_ptr<dcl::DataTransfer> ProcessImpl::sendData(size_t size,
        const void *ptr) {
    return getDataStream().write(size, ptr);
}

std::shared_ptr<dcl::DataTransfer> ProcessImpl::receiveData(size_t size,
        void *ptr) {
    return getDataStream().read(size, ptr);
}

comm::DataStream& ProcessImpl::getDataStream() {
    std::lock_guard<std::recursive_mutex> lock(_connectionStatusMutex);
    while (!_dataStream) {
        /* TODO Use timed wait when waiting for data stream;
         * throw dcl::ConnectionException after timeout.
         * This method is deadlock-prone. However, without it (see sendData and
         * receiveData) the data stream sometimes is not ready when sending or
         * receiving data, in particular when using a large number (>=32) of
         * compute nodes. */
        _dataStreamReady.wait(_connectionStatusMutex);
    }
    return *_dataStream;
}

void ProcessImpl::setDataStream(
        comm::DataStream *dataStream) {
    {
        std::lock_guard<std::recursive_mutex> lock(_connectionStatusMutex);
        std::swap(_dataStream, dataStream);
        if (_dataStream) {
            assert(_connectionStatus == ConnectionStatus::MESSAGE_QUEUE_CONNECTED);
            _connectionStatus = ConnectionStatus::CONNECTED;
            _dataStreamReady.notify_all();
        }
    }

    // destroy old data stream
    if (dataStream) {
    	_dataDispatcher.destroy_data_stream(dataStream);
    }
}

} /* namespace dclasio */
