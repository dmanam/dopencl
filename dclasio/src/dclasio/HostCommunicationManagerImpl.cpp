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
 * \file HostCommunicationManagerImpl.cpp
 *
 * \date 2011-10-26
 * \author Philipp Kegel
 */

#include "HostCommunicationManagerImpl.h"

#include "ComputeNodeImpl.h"
#include "HostImpl.h"

#include "comm/CLEventProcessor.h"
#include "comm/CLResponseProcessor.h"

#include <dclasio/message/Message.h>

#include <dcl/CLObjectRegistry.h>
#include <dcl/ComputeNode.h>
#include <dcl/ContextListener.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>

#include <dcl/util/Logger.h>

#include <cassert>
#include <iterator>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace dclasio {

/* ****************************************************************************
 * Host communication manager implementation
 ******************************************************************************/

HostCommunicationManagerImpl::HostCommunicationManagerImpl() {
    _clEventProcessor.reset(new comm::CLComputeNodeEventProcessor(
            *this, _objectRegistry));
    _clResponseProcessor.reset(new comm::CLResponseProcessor(*this));
}

HostCommunicationManagerImpl::~HostCommunicationManagerImpl() {
}

dcl::CLObjectRegistry& HostCommunicationManagerImpl::objectRegistry() {
    return _objectRegistry;
}

dcl::ComputeNode * HostCommunicationManagerImpl::createComputeNode(
        const std::string& url) {
    std::vector<dcl::ComputeNode *> computeNodes;

    createComputeNodes({ url }, computeNodes);
    assert(computeNodes.size() == 1);
    if (computeNodes[0]) throw dcl::ConnectionException("Connection failed");

    return computeNodes[0];
}

void HostCommunicationManagerImpl::createComputeNodes(
        const std::vector<std::string>& urls,
        std::vector<dcl::ComputeNode *>& computeNodes) {
    std::vector<ComputeNodeImpl *> createdComputeNodes;
    CommunicationManagerImpl::createComputeNodes(urls, createdComputeNodes);

    /* TODO Implement connection process in communication manager
     * The communication manager is able to detect a redundant connection
     * based on the process ID that is returned during the connection process
     * via the message queue. If a connection is already established, return
     * the existing process, rather than creating a new one.
     * WARNING This method must *not* delete compute nodes that have been
     * returned as a replacement for a duplicate connection. */
    // TODO Connect asynchronously
    // connect to compute nodes (parallelized operation)
    ComputeNodeImpl::connect(createdComputeNodes, ProcessImpl::Type::HOST, _pid);

    // add connected compute nodes to compute node list
    std::unique_lock<std::recursive_mutex> lock(_connectionsMutex);
    for (auto computeNode : createdComputeNodes) {
        if (computeNode->isConnected()) {
            assert(computeNode->get_id() != 0);
            _computeNodes.emplace(computeNode->get_id(), std::unique_ptr<ComputeNodeImpl>(computeNode));
            computeNodes.push_back(computeNode);
        } else {
            delete computeNode;
            computeNodes.push_back(nullptr); // return nullptr to indicate failed connection
        }
    }
    lock.unlock();

    // Prefetch device IDs
    ComputeNodeImpl::updateDevices(createdComputeNodes);
    /* TODO Handle connection error
     * Devices of compute nodes whose connections failed should become unavailable. */
}

void HostCommunicationManagerImpl::destroyComputeNode(
        dcl::ComputeNode *computeNode) {
    destroyComputeNode(dynamic_cast<ComputeNodeImpl *>(computeNode));
}

/*
 * Message listener API
 */

void HostCommunicationManagerImpl::message_received(
        comm::message_queue& msgq,
        const message::Message& message) {
    // TODO Determine sender's process ID
    dcl::process_id pid = msgq.get_process_id();

    assert(_clEventProcessor && "No event processor");
    if (_clEventProcessor->dispatch(message, pid))
        return;

    assert(_clResponseProcessor && "No response processor");
    auto response = dynamic_cast<const message::Response *>(&message);
    if (response && _clResponseProcessor->dispatch(*response, pid))
        return;

    dcl::util::Logger << dcl::util::Error
            << "Received unknown message" << std::endl;
}

} /* namespace dclasio */
