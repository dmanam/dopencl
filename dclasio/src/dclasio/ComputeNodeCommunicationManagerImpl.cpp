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
 * \file ComputeNodeCommunicationManagerImpl.cpp
 *
 * \date 2011-10-26
 * \author Philipp Kegel
 */

#include "ComputeNodeCommunicationManagerImpl.h"

#include "ComputeNodeImpl.h"
#include "DCLAsioTypes.h"
#include "HostImpl.h"
#include "ProcessImpl.h"
#include "SmartCLObjectRegistry.h"

#include "comm/CLEventProcessor.h"
#include "comm/CLRequestProcessor.h"
#include "comm/MessageDispatcher.h"
#include "comm/MessageQueue.h"

#include <dcl/ComputeNode.h>
#include <dcl/ConnectionListener.h>
#include <dcl/Daemon.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>

#include <dcl/util/Logger.h>

#include <cassert>
#include <condition_variable>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dclasio {

/* ****************************************************************************
 * Compute node communication manager implementation
 ******************************************************************************/

ComputeNodeCommunicationManagerImpl::ComputeNodeCommunicationManagerImpl(
        const std::string& host, port_type port) :
		CommunicationManagerImpl(host, port), _daemon(nullptr) {
	_clEventProcessor.reset(new comm::CLHostEventProcessor(*this));
	_clRequestProcessor.reset(new comm::CLRequestProcessor(*this));
}

ComputeNodeCommunicationManagerImpl::~ComputeNodeCommunicationManagerImpl() {
}

SmartCLObjectRegistry& ComputeNodeCommunicationManagerImpl::objectRegistry() {
    return _objectRegistry;
}

void ComputeNodeCommunicationManagerImpl::connectComputeNodes(
        const std::vector<ComputeNodeImpl *>& computeNodes) {
    assert(!"Fix ComputeNodeCommunicationManagerImpl::connectComputeNodes");

    // Compute node that will connect to this process
    std::set<ComputeNodeImpl *> activeComputeNodes;
    // Compute nodes that this process will connect to
    std::set<ComputeNodeImpl *> passiveComputeNodes;

    /* A collective node-to-node connection operation:
     * A connection must be initiated by exactly one of the involved compute
     * nodes. Therefore, compute nodes must agree on which ones actively connect
     * while others passively accept connections.
     * For this purpose, an order of compute nodes is derived from the order of
     * message queue endpoints. Based on this order, computes nodes either have
     * a 'lower' or 'higher' rank than others.
     * A compute node only actively connects to other compute nodes that have a
     * lower rank, and passively wait for connections from compute nodes with
     * higher ranks. */

    // Determine active and passive compute nodes
    for (auto computeNode : computeNodes) {
        assert(computeNode != nullptr); // compute node must not be NULL

        /* FIXME Do not compare compute nodes by process ID
         * The process ID is 0 for non-connected processes. */
        if (computeNode->get_id() == _pid) {
            /* a compute node cannot connect to itself */
            continue;
        }

        if (computeNode->get_id() < _pid) {
            // actively connect to passive compute nodes
            passiveComputeNodes.insert(computeNode);
        } else {
            // await connection from active compute nodes
            activeComputeNodes.insert(computeNode);
        }
    }

    // Connect to passive compute nodes (parallelized operation)
    ComputeNodeImpl::connect(
            std::vector<ComputeNodeImpl *>(std::begin(passiveComputeNodes), std::end(passiveComputeNodes)),
            ProcessImpl::Type::COMPUTE_NODE, _pid);

    // Await connection of active compute nodes
    ComputeNodeImpl::awaitConnection(
            std::vector<ComputeNodeImpl *>(std::begin(activeComputeNodes), std::end(activeComputeNodes)));

    /* FIXME Add passive compute nodes (outgoing connections) to compute node list
     * Active compute nodes (incoming connections) will be added by the message dispatcher */
}

void ComputeNodeCommunicationManagerImpl::setDaemon(dcl::Daemon *daemon) {
    if (_daemon) {
        /* TODO Deregister devices of old daemon */
    }

    _daemon = daemon;

    if (_daemon) {
        std::vector<dcl::Device *> devices;

        // Register devices
        _daemon->getDevices(devices);
        for (auto device : devices) {
            _objectRegistry.bind(device->getId(), device);
        }
    }
}

dcl::Daemon * ComputeNodeCommunicationManagerImpl::getDaemon() const {
    return _daemon;
}

bool ComputeNodeCommunicationManagerImpl::addConnectionListener(
		dcl::ConnectionListener& listener) {
	std::lock_guard<std::mutex> lock(_connectionListenersMutex);
	return (_connectionListeners.insert(&listener)).second;
}

bool ComputeNodeCommunicationManagerImpl::removeConnectionListener(
		dcl::ConnectionListener& listener) {
	std::lock_guard<std::mutex> lock(_connectionListenersMutex);
	return (_connectionListeners.erase(&listener) == 1);
}

ProcessImpl * ComputeNodeCommunicationManagerImpl::get_process(
        dcl::process_id pid) const {
    std::lock_guard<std::recursive_mutex> lock(_connectionsMutex);
    /* Do not use ComputeNodeCommunicationManagerImpl::getHost and
     * CommunicationManagerImpl::getComputeNode to search for node in order to
     * avoid multiple locking operations on _connectionsMutex */
    auto i = _hosts.find(pid); // search host list
    if (i != std::end(_hosts)) return i->second.get();
    auto j = _computeNodes.find(pid); // search compute node list
    if (j != std::end(_computeNodes)) return j->second.get();
    return nullptr;
}

HostImpl * ComputeNodeCommunicationManagerImpl::get_host(
        dcl::process_id pid) const {
    std::lock_guard<std::recursive_mutex> lock(_connectionsMutex);
    auto i = _hosts.find(pid); // search host list
    return (i != std::end(_hosts)) ? i->second.get() : nullptr;
}

void ComputeNodeCommunicationManagerImpl::host_connected(
        comm::message_queue& msgq,
        dcl::process_id pid) {
    auto host = std::unique_ptr<HostImpl>(new HostImpl(
            pid, _messageDispatcher, _dataDispatcher, msgq));
    bool accepted = false;

    // notify connection listeners
    {
        std::lock_guard<std::mutex> lock(_connectionListenersMutex);
        for (auto listener : _connectionListeners) {
            /* WARNING: do not change expression order, otherwise short-cut
             * evaluation might skip calling the listener */
            accepted = listener->connected(*host) || accepted;
        }
    }

    if (accepted) {
        dcl::util::Logger << dcl::util::Debug
                << "Accepted connection from host '" << host->url() << '\'' << std::endl;
        // add host to list
        {
            std::lock_guard<std::recursive_mutex> lock(_connectionsMutex);
            bool inserted = _hosts.emplace(pid, std::move(host)).second;
            assert(inserted && "Could not add host to list"); // assert insertion of host into list
        }
    } else {
        dcl::util::Logger << dcl::util::Warning
                << "Rejected connection from host '" << host->url() << '\'' << std::endl;
    }
}

void ComputeNodeCommunicationManagerImpl::compute_node_connected(
        comm::message_queue& msgq,
        dcl::process_id pid) {
    auto computeNode = std::unique_ptr<ComputeNodeImpl>(new ComputeNodeImpl(
            pid, _messageDispatcher, _dataDispatcher, msgq));
    bool accepted = false;

    // notify connection listeners
    {
        std::lock_guard<std::mutex> lock(_connectionListenersMutex);
        for (auto listener : _connectionListeners) {
            /* WARNING: do not change expression order, otherwise short-cut
             * evaluation might skip calling the listener */
            accepted = listener->connected(*computeNode) || accepted;
        }
    }

    if (accepted) {
        dcl::util::Logger << dcl::util::Debug
                << "Accepted connection from compute node '" << computeNode->url() << '\'' << std::endl;
        // add compute node to list
        {
            std::lock_guard<std::recursive_mutex> lock(_connectionsMutex);
            bool inserted = _computeNodes.emplace(pid, std::move(computeNode)).second;
            assert(inserted && "Could not add compute node to list"); // assert insertion of compute node into list
        }
    } else {
        dcl::util::Logger << dcl::util::Warning
                << "Rejected connection from compute node '" << computeNode->url() << '\'' << std::endl;
    }
}

/*
 * Connection listener API
 */

bool ComputeNodeCommunicationManagerImpl::approve_message_queue(
        ProcessImpl::Type process_type,
        dcl::process_id pid) {
    /* TODO Allow multiple message queues per process and reconnecting of message queues
    return true; // accept all incoming connections
     */
    return (get_process(pid) == nullptr); // process ID must not be associated with a process yet
}

void ComputeNodeCommunicationManagerImpl::message_queue_connected(
        comm::message_queue& msgq,
        ProcessImpl::Type process_type,
        dcl::process_id pid) {
    /* TODO Log node type ('host' or 'compute node') */
    dcl::util::Logger << dcl::util::Info
            << "Incoming message queue connection"
            << std::endl;

    // TODO Allow reconnecting of message queues
    assert(get_process(pid) == nullptr); // process ID must not be associated with a process yet

    switch (process_type) {
    case ProcessImpl::Type::HOST:
        host_connected(msgq, pid);
        break;
    case ProcessImpl::Type::COMPUTE_NODE:
        compute_node_connected(msgq, pid);
        break;
    default:
        assert(!"Invalid process type");
        /* no break */
    }
}

void ComputeNodeCommunicationManagerImpl::message_queue_disconnected(
        comm::message_queue& msgq) {
    // TODO Determine sender's process ID
    dcl::process_id pid = msgq.get_process_id();
    
    auto host = get_host(pid);
    auto compute_node = get_compute_node(pid);

    if (host) {
        assert(!compute_node);

        // notify connection listeners
        std::lock_guard<std::mutex> lock(_connectionListenersMutex);
        for (auto listener : _connectionListeners) {
            listener->disconnected(*host);
        }
    } else if (compute_node) {
        assert(!host);

        // notify connection listeners
        std::lock_guard<std::mutex> lock(_connectionListenersMutex);
        for (auto listener : _connectionListeners) {
            listener->disconnected(*compute_node);
        }
    } else {
        // unknown process disconnected
    }

    // TODO Delete disconnected process
    std::lock_guard<std::recursive_mutex> lock(_connectionsMutex);
    _hosts.erase(pid);
    _computeNodes.erase(pid);
}

/*
 * Message listener API
 */

void ComputeNodeCommunicationManagerImpl::message_received(
        comm::message_queue& msgq,
        const message::Message& message) {
    // TODO Determine sender's process ID
    dcl::process_id pid = msgq.get_process_id();

    assert(_clEventProcessor && "No event processor");
    if (_clEventProcessor->dispatch(message, pid))
        return;

    assert(_clRequestProcessor && "No request processor");
    auto request = dynamic_cast<const message::Request *>(&message);
    if (request && _clRequestProcessor->dispatch(*request, pid))
        return;

    // unknown message
    dcl::util::Logger << dcl::util::Error
            << "Received unknown message" << std::endl;
}

} /* namespace dclasio */
