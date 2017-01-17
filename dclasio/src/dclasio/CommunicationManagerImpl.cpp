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
 * \file CommunicationManagerImpl.cpp
 *
 * \date 2011-10-26
 * \author Philipp Kegel
 */

#include "CommunicationManagerImpl.h"

#include "ComputeNodeImpl.h"
#include "DCLAsioTypes.h"
#include "HostImpl.h"
#include "ProcessImpl.h"

#include "comm/CLEventProcessor.h"
#include "comm/DataDispatcher.h"

#include <dcl/ConnectionListener.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>

#include <dcl/util/Logger.h>

#include <boost/algorithm/string/trim.hpp>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/uuid/uuid.hpp>

#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace dclasio {

/* ****************************************************************************
 * Communication manager base implementation
 ******************************************************************************/

const std::chrono::seconds CommunicationManagerImpl::DEFAULT_CONNECTION_TIMEOUT = std::chrono::seconds(3);

void CommunicationManagerImpl::resolve_url(
		const std::string& url,
		std::string& hostName,
		port_type& port) {
	size_t delim = url.find(":");

	hostName = url.substr(0, delim);
	boost::trim(hostName); // remove leading and trailing white spaces
	if (delim != std::string::npos) {
		std::istringstream stream(url.substr(delim));
		port_type defaultPort = port;

		stream.ignore(); // skip delimiter
		if ((stream >> port).fail()) {
			port = defaultPort;
		}
	}
}

dcl::process_id CommunicationManagerImpl::create_process_id(
        const std::string& hostName,
        port_type port) {
    // FIXME Create unique process ID without host and port
    dcl::process_id pid = 0;

    for (std::string::size_type i = 0; i < hostName.size(); ++i) {
        pid += (unsigned char) hostName[i];
        pid <<= 4;
    }
    pid += port;

    return pid;
}

CommunicationManagerImpl::CommunicationManagerImpl() :
        _pid(create_process_id("", DEFAULT_PORT)),
        _messageDispatcher(_pid), _dataDispatcher(_pid) {
}

CommunicationManagerImpl::CommunicationManagerImpl(
        const std::string& host, port_type port) :
        _pid(create_process_id(host, port)), _messageDispatcher(_pid), _dataDispatcher(_pid) {
    bind(host, port);
}

CommunicationManagerImpl::~CommunicationManagerImpl() {
}

void CommunicationManagerImpl::bind(
        const std::string& host, port_type port) {
    if (host.empty()) {
        throw dcl::InvalidArgument(DCL_INVALID_NODE, "Missing host name");
    }

    // resolve local endpoint
    boost::asio::ip::tcp::resolver resolver(_io_service);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), host, std::to_string(port));
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    endpoint_type message_endpoint = *iterator;
    endpoint_type data_endpoint(
            message_endpoint.address(), message_endpoint.port() + 100);

    // bind local endpoints to connection acceptors
    _messageDispatcher.bind(message_endpoint);
    _dataDispatcher.bind(data_endpoint);

    dcl::util::Logger << dcl::util::Info
            << "Bound to " << host << ':' << port
            << std::endl;
}

void CommunicationManagerImpl::start() {
    _messageDispatcher.add_connection_listener(*this);
    _messageDispatcher.add_message_listener(*this);
	_dataDispatcher.add_connection_listener(*this);
    _messageDispatcher.start();
    _dataDispatcher.start();
}

void CommunicationManagerImpl::stop() {
    _messageDispatcher.remove_message_listener(*this);
    _messageDispatcher.remove_connection_listener(*this);
    _messageDispatcher.stop();
    _dataDispatcher.remove_connection_listener(*this);
    _dataDispatcher.stop(); // cancel all data stream operations
}

void CommunicationManagerImpl::createComputeNodes(
        const std::vector<std::string>& urls,
        std::vector<ComputeNodeImpl *>& computeNodes) {
    assert(_clEventProcessor && "Event processor not initialized");
    computeNodes.clear();

    for (const auto& url : urls) {
        std::string host;
        port_type port = DEFAULT_PORT;

        resolve_url(url, host, port);
        if (host.empty()) {
            dcl::util::Logger << dcl::util::Warning
                    << "Invalid URL '" << url << '\'' << std::endl;
            continue;
        }
        boost::asio::ip::tcp::resolver resolver(_io_service);
        boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), host, std::to_string(port));
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

        /* TODO Prevent creation of duplicates
         * Connect message queue to obtain remote process ID and look for this
         * ID in list of connected processes *and* pending process connections.
         * Return the existing process, rather than a new one in this case. */

        auto computeNode = new ComputeNodeImpl(
                _messageDispatcher, _dataDispatcher, *iterator);

        /* A process does not have a valid process ID before it is connected.
         * Hence, the compute node cannot be added to the list of compute nodes yet. */
        computeNodes.push_back(computeNode);
    }
}

void CommunicationManagerImpl::destroyComputeNode(
        ComputeNodeImpl *computeNode) {
    if (computeNode) {
        computeNode->disconnect();
        {
            std::lock_guard<std::recursive_mutex> lock(_connectionsMutex);
            _computeNodes.erase(computeNode->get_id());
        }
    } else {
        throw dcl::InvalidArgument(DCL_INVALID_NODE);
    }
}

ProcessImpl * CommunicationManagerImpl::get_process(
        dcl::process_id pid) const {
    return get_compute_node(pid);
}

ComputeNodeImpl * CommunicationManagerImpl::get_compute_node(
        dcl::process_id pid) const {
    std::lock_guard<std::recursive_mutex> lock(_connectionsMutex);
    auto i = _computeNodes.find(pid); // search compute node list
    return (i != std::end(_computeNodes)) ? i->second.get() : nullptr;
}

void CommunicationManagerImpl::get_compute_nodes(
        const std::vector<dcl::process_id>& pids,
        std::vector<ComputeNodeImpl *>& computeNodes) const {
    computeNodes.clear();
    std::lock_guard<std::recursive_mutex> lock(_connectionsMutex);
    for (auto pid : pids) {
        auto i = _computeNodes.find(pid);
        if (i == std::end(_computeNodes)) { // compute node not found
            throw dcl::ConnectionException("Invalid process ID");
        }
        computeNodes.push_back(i->second.get());
    }
}

/*
 * Connection listener API
 */

bool CommunicationManagerImpl::approve_message_queue(
        ProcessImpl::Type process_type,
        dcl::process_id pid) {
    return false; // reject all incoming connections
}

void CommunicationManagerImpl::message_queue_connected(
        comm::message_queue& msgq,
        ProcessImpl::Type process_type,
        dcl::process_id pid) {
    // ignore process
    dcl::util::Logger << dcl::util::Warning
            << "Ignoring incoming connection" << std::endl;
}

void CommunicationManagerImpl::message_queue_disconnected(
        comm::message_queue& msgq) {
    // ignore process disconnect
    dcl::util::Logger << dcl::util::Warning
            << "Ignoring closed connection" << std::endl;
}

bool CommunicationManagerImpl::approve_data_stream(
        dcl::process_id pid) {
    // check if the source process is already registered
    return (get_process(pid) != nullptr);
}

void CommunicationManagerImpl::data_stream_connected(
        comm::DataStream& data_stream,
        dcl::process_id pid) {
    /* The connections list must be locked, such that a node is not
     * destroyed while its associated data stream is changed */
    std::lock_guard<std::recursive_mutex> lock(_connectionsMutex);

    // lookup process for ID
    auto process = get_process(pid);
    if (process) {
        /* TODO Log node type ('host' or 'compute node') */
        dcl::util::Logger << dcl::util::Info
                << "Incoming data stream connection from process '" << process->url()
                << "' (pid=" << process->get_id() << ')'
                << std::endl;

        process->setDataStream(&data_stream);
    } else {
        dcl::util::Logger << dcl::util::Warning
                << "Incoming data stream connection from unknown process"
                << " (pid=" << process->get_id() << ')'
                << std::endl;
    }
}

/*
 * Message listener API
 */

void CommunicationManagerImpl::message_received(
        comm::message_queue& msgq,
        const message::Message& message) {
    // TODO Determine sender's process ID
    dcl::process_id pid = msgq.get_process_id();

    assert(_clEventProcessor && "No event processor");
    if (_clEventProcessor->dispatch(message, pid))
        return;

    // unknown message
    dcl::util::Logger << dcl::util::Error
            << "Received unknown message" << std::endl;
}

} /* namespace dclasio */
