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
 * \file CLEventProcessor.cpp
 *
 * \date 2014-04-04
 * \author Philipp Kegel
 */

#include "CLEventProcessor.h"

#include "../CommunicationManagerImpl.h"
#include "../ComputeNodeCommunicationManagerImpl.h"
#include "../ComputeNodeImpl.h"
#include "../HostImpl.h"
#include "../SmartCLObjectRegistry.h"

#include <dclasio/message/CommandMessage.h>
#include "../message/ContextErrorMessage.h"
#include <dclasio/message/EventSynchronizationMessage.h>
#include "../message/ProgramBuildMessage.h"

#include <dcl/Binary.h>
#include <dcl/BlockingQueue.h>
#include <dcl/CLObjectRegistry.h>
#include <dcl/CommandListener.h>
#include <dcl/ContextListener.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>
#include <dcl/Device.h>
#include <dcl/Process.h>
#include <dcl/ProgramBuildListener.h>
#include <dcl/SynchronizationListener.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <thread>
#include <vector>

namespace dclasio {

namespace comm {

/* ****************************************************************************
 * Host side event processor
 ******************************************************************************/

CLComputeNodeEventProcessor::CLComputeNodeEventProcessor(
        const CommunicationManagerImpl& communicationManager,
        const dcl::CLObjectRegistry& objectRegistry) :
    _communicationManager(communicationManager), _objectRegistry(objectRegistry) {
    // Message dispatcher must be initialized before the worker thread is created
    _worker = std::thread(&CLComputeNodeEventProcessor::run, this);
}

CLComputeNodeEventProcessor::~CLComputeNodeEventProcessor() {
    // interrupt blocking queue to stop thread
    _taskList.interrupt();
    if (_worker.joinable()) _worker.join();
}

void CLComputeNodeEventProcessor::run() {
    try {
        // wait for and dispatch execution status changes
        while (true) {
            _taskList.front()();
            _taskList.pop();
        }
    } catch (const dcl::ThreadInterrupted&) {
        // task queue has been interrupted
    }
}

void CLComputeNodeEventProcessor::contextError(
        const message::ContextErrorMessage& notification) const {
    auto contextListener = _objectRegistry.lookup<dcl::ContextListener>(notification.contextId);
    if (contextListener) {
        contextListener->onError(notification.errorInfo.c_str(),
                notification.privateInfo.value(), notification.privateInfo.size());
    } else {
        dcl::util::Logger << dcl::util::Error
                << "Context listener not found (ID=" << notification.contextId
                << ')' << std::endl;
    }
}

void CLComputeNodeEventProcessor::executionStatusChanged(
        const message::CommandExecutionStatusChangedMessage& notification) {
    auto commandListener = _objectRegistry.lookup<dcl::CommandListener>(notification.commandId());
    if (commandListener) {
        // pass function call to worker thread
        _taskList.push(
                std::bind(&dcl::CommandListener::onExecutionStatusChanged,
                        commandListener, notification.status()));
    } else {
        dcl::util::Logger << dcl::util::Error
                << "Command listener not found (command ID=" << notification.commandId()
                << ')' << std::endl;
    }
}

void CLComputeNodeEventProcessor::synchronizeEvent(
        const message::EventSynchronizationMessage& notification,
        dcl::Process& process) const {
    auto synchronizationlistener = _objectRegistry.lookup<dcl::SynchronizationListener>(notification.commandId());
    if (synchronizationlistener) {
        synchronizationlistener->onSynchronize(process);
    } else {
        dcl::util::Logger << dcl::util::Error
                << "Synchronization listener not found (command ID=" << notification.commandId()
                << ')' << std::endl;
    }
}

void CLComputeNodeEventProcessor::programBuildComplete(
        const message::ProgramBuildMessage& notification) const {
    auto programBuildListener = _objectRegistry.lookup<dcl::ProgramBuildListener>(notification.programBuildId);
    if (programBuildListener) {
        std::vector<dcl::Device *> devices;
        std::vector<cl_build_status> buildStatus;

        /* TODO Lookup devices from notification.deviceIds() */

        programBuildListener->onComplete(devices, notification.buildStatus);
    } else {
        dcl::util::Logger << dcl::util::Error
                << "Program build listener not found (ID=" << notification.programBuildId
                << ')' << std::endl;
    }
}

bool CLComputeNodeEventProcessor::dispatch(
        const message::Message& message,
        dcl::process_id pid) {
    ComputeNodeImpl *computeNode = nullptr;

    switch (message.get_type()) {
    case message::ContextErrorMessage::TYPE:
        dcl::util::Logger << dcl::util::Debug
                << "Received context error message from compute node" << std::endl;
        contextError(
                static_cast<const message::ContextErrorMessage&>(message));
        break;

    case message::CommandExecutionStatusChangedMessage::TYPE:
        dcl::util::Logger << dcl::util::Debug
                << "Received command execution status changed message from compute node" << std::endl;
        executionStatusChanged(
                static_cast<const message::CommandExecutionStatusChangedMessage&>(message));
        break;

    case message::EventSynchronizationMessage::TYPE:
        dcl::util::Logger << dcl::util::Debug
                << "Received event synchronization message from compute node" << std::endl;

        computeNode = _communicationManager.get_compute_node(pid);
        assert(computeNode && "No host for event");
        if (!computeNode)
            return false;

        synchronizeEvent(
                static_cast<const message::EventSynchronizationMessage&>(message),
                *computeNode);
        break;

    case message::ProgramBuildMessage::TYPE:
        dcl::util::Logger << dcl::util::Debug
                << "Received program build message" << std::endl;
        programBuildComplete(
                static_cast<const message::ProgramBuildMessage&>(message));
        break;

    default: // unknown message
        return false;
    }

    return true;
}

/* ****************************************************************************
 * Compute node side event processor
 ******************************************************************************/

CLHostEventProcessor::CLHostEventProcessor(
        const ComputeNodeCommunicationManagerImpl& communicationManager) :
    _communicationManager(communicationManager) { }

SmartCLObjectRegistry& CLHostEventProcessor::getObjectRegistry(
        HostImpl& host) const {
    /* TODO Do not make registry a member of host */
    return host.objectRegistry();
}

void CLHostEventProcessor::executionStatusChanged(
        const message::CommandExecutionStatusChangedMessage& notification,
        HostImpl& host) const {
    auto event = getObjectRegistry(host).lookup<std::shared_ptr<dcl::Event>>(notification.commandId());
    if (event) {
        event->onExecutionStatusChanged(notification.status());
    } else {
        dcl::util::Logger << dcl::util::Error
                << "Event not found (command ID=" << notification.commandId()
                << ')' << std::endl;
    }
}

void CLHostEventProcessor::synchronizeEvent(
        const message::EventSynchronizationMessage& notification,
        HostImpl& host) const {
    auto event = getObjectRegistry(host).lookup<std::shared_ptr<dcl::Event>>(notification.commandId());
    if (event) {
        event->onSynchronize(host);
    } else {
        dcl::util::Logger << dcl::util::Error
                << "Event not found (command ID=" << notification.commandId()
                << ')' << std::endl;
    }
}

bool CLHostEventProcessor::dispatch(
        const message::Message& message,
        dcl::process_id pid) {
    HostImpl *host = _communicationManager.get_host(pid);
    assert(host && "No host for event");
    if (!host)
        return false;

    switch (message.get_type()) {
    case message::CommandExecutionStatusChangedMessage::TYPE:
        dcl::util::Logger << dcl::util::Debug
                << "Received command execution status changed message from host" << std::endl;
        executionStatusChanged(
                static_cast<const message::CommandExecutionStatusChangedMessage&>(message), *host);
        break;

    case message::EventSynchronizationMessage::TYPE:
        dcl::util::Logger << dcl::util::Debug
                << "Received event synchronization message from host" << std::endl;
        synchronizeEvent(
                static_cast<const message::EventSynchronizationMessage&>(message), *host);
        break;

    default: // unknown message
        return false;
    }

    return true;
}

} /* namespace comm */

} /* namespace dclasio */
