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
 * \file Event.cpp
 *
 * \date 2012-07-28
 * \author Philipp Kegel
 */

#include "Event.h"

#include "Context.h"
#include "Memory.h"

#include <dclasio/message/CommandMessage.h>
#include <dclasio/message/EventSynchronizationMessage.h>

#include <dcl/ComputeNode.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>
#include <dcl/Host.h>
#include <dcl/Remote.h>

#include <dcl/util/Clock.h>
#include <dcl/util/Logger.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.hpp>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cassert>
#if 0
#include <functional>
#endif
#include <memory>
#include <mutex>
#include <ostream>

namespace {

#if 0
/*!
 * \brief Generic function to convert OpenCL event callback into a C++-style callback
 */
void eventCallback(
        cl_event object_,
        cl_int event_command_exec_status,
        void *user_data) {
    /* convert user data to C++ callback */
    std::unique_ptr<std::function<void (cl::Event, cl_int)>> pfn(
            static_cast<std::function<void (cl::Event, cl_int)> *>(user_data));

    /* assign event to C++ wrapper */
    cl::Event event;
    event() = object_;
    cl_int err = ::clRetainEvent(object_);
    assert(err == CL_SUCCESS);

    (*pfn)(event, event_command_exec_status); // execute callback
}
#endif

/*!
 * \brief Callback for updating an event status
 */
void onEventComplete(cl_event object_, cl_int execution_status,
        void *user_data) {
    auto event = static_cast<dcld::LocalEvent *>(user_data);
    assert(execution_status == CL_COMPLETE || execution_status < 0);
    // FIXME Event::operator() fails, if host process has been killed during data transfer
    assert(event != nullptr && event->operator cl::Event()() == object_);
    event->onExecutionStatusChanged(execution_status);
}

} /* unnamed namespace */

/* ****************************************************************************/

namespace dcld {

Event::Event(
        const std::shared_ptr<Context>& context,
        const std::vector<std::shared_ptr<Memory>>& memoryObjects) :
	_context(context), _memoryObjects(memoryObjects) {
    /* do not check context here, to allow for error handling in derived classes */
}

Event::Event(
        const std::shared_ptr<Context>& context,
        const std::shared_ptr<Memory>& memoryObject) :
    _context(context), _memoryObjects(1, memoryObject) { }

Event::Event(
        const std::shared_ptr<Context>& context) :
    _context(context) { }

/* ****************************************************************************/

RemoteEvent::RemoteEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const std::vector<std::shared_ptr<Memory>>& memoryObjects) :
    dcl::Remote(id), Event(context, memoryObjects) {
    if (!context) throw cl::Error(CL_INVALID_CONTEXT);

    _event = cl::UserEvent(*_context);
}

RemoteEvent::operator cl::Event() const {
    return _event;
}

void RemoteEvent::synchronize(
        const cl::CommandQueue& commandQueue,
        VECTOR_CLASS<cl::Event>& nativeEventList) {
    std::lock_guard<std::mutex> lock(_syncMutex);

    dcl::util::Logger << dcl::util::Debug
            << "Synchronizing replacement event with remote event (ID=" << _id << ')'
            << std::endl;

    if (       !_memoryObjects.empty() /* synchronization of memory objects required */
            &&     _syncEvents.empty() /* no synchronization performed yet */) {
        /*
         * Trigger event synchronization on host
         */
        /* TODO Use SychronizationListener interface to send message */
        /* TODO Send message to event owner (host or compute node) */
        dclasio::message::EventSynchronizationMessage msg(_id);
        _context->host().sendMessage(msg);
        dcl::util::Logger << dcl::util::Debug
                << "Sent event synchronization message to host (ID=" << _id << ')'
                << std::endl;

        for (auto memoryObject : _memoryObjects) {
            cl::Event acquire; /* Event representing the acquire operation of the current memory object.
                                * Serves as synchronization point for following commands and other devices */
            memoryObject->acquire(_context->host(), commandQueue, _event, &acquire);
            _syncEvents.push_back(acquire);
        }
    }

    nativeEventList = _syncEvents;
}

void RemoteEvent::getProfilingInfo(cl_profiling_info param_name,
        cl_ulong& param_value) const {
    /* Remote events rely on a user event and thus cannot provide profiling
     * info. This info can only be obtained from the native event this event
     * is associated with, i.e. listening to. */
    throw cl::Error(CL_PROFILING_INFO_NOT_AVAILABLE);
}

void RemoteEvent::onExecutionStatusChanged(cl_int executionStatus) {
    assert(executionStatus == CL_COMPLETE || executionStatus < 0);
    _event.setStatus(executionStatus);
}

void RemoteEvent::onSynchronize(dcl::Process& process) {
    dcl::util::Logger << dcl::util::Error
            << "Synchronization attempt on replacement event (ID=" << _id << ')'
            << std::endl;
}

/* ****************************************************************************/

LocalEvent::LocalEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
		const std::vector<std::shared_ptr<Memory>>& memoryObjects) :
	dcl::Remote(id), Event(context, memoryObjects),
	_received(dcl::util::clock.getTime()) {
    /* local events are created by command queue methods, which should pass checked arguments */
    assert(context != nullptr && "Invalid context");
}

LocalEvent::LocalEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const std::shared_ptr<Memory>& memoryObject) :
    dcl::Remote(id), Event(context, memoryObject),
	_received(dcl::util::clock.getTime()) {
    /* local events are created by command queue methods, which should pass checked arguments */
    assert(context != nullptr && "Invalid context");
}

LocalEvent::LocalEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context) :
    dcl::Remote(id), Event(context),
	_received(dcl::util::clock.getTime()) {
    /* local events are created by command queue methods, which should pass checked arguments */
    assert(context != nullptr && "Invalid context");
}

LocalEvent::~LocalEvent() {
}

void LocalEvent::onSynchronize(dcl::Process& process) {
    cl::CommandQueue commandQueue = _context->ioCommandQueue();

    dcl::util::Logger << dcl::util::Debug
            << "Event synchronization (ID=" << _id
            << ") requested by '" << process.url() << '\''
            << std::endl;

    /* Acquire changes to memory objects associated with this event.
     * The acquire operations are performed using the context's I/O command
     * queue. This queue is reserved for synchronization and thus does not
     * interfere (e.g., deadlock) with application commands. */
    for (auto memoryObject : _memoryObjects) {
        memoryObject->release(process, commandQueue, *this);
    }

    /* The I/O command queue must be flushed to ensure instant execution of the
     * acquire operation */
    commandQueue.flush();
}

/* ****************************************************************************/

SimpleEvent::SimpleEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const std::vector<std::shared_ptr<Memory>>& memoryObjects,
        const cl::Event& event) :
    LocalEvent(id, context, memoryObjects), _event(event)
{
    /* Schedule event status update notification */
    _event.setCallback(CL_COMPLETE, &onEventComplete, this);
}

SimpleEvent::SimpleEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const std::shared_ptr<Memory>& memoryObject,
        const cl::Event& event) :
    LocalEvent(id, context, memoryObject), _event(event)
{
    /* Schedule event status update notification */
    _event.setCallback(CL_COMPLETE, &onEventComplete, this);
}

SimpleEvent::SimpleEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const cl::Event& event) :
    LocalEvent(id, context), _event(event)
{
    /* Schedule event status update notification */
    _event.setCallback(CL_COMPLETE, &onEventComplete, this);
}

SimpleEvent::operator cl::Event() const {
    return _event;
}

void SimpleEvent::getProfilingInfo(cl_profiling_info param_name,
        cl_ulong& param_value) const {
    switch (param_name) {
    case CL_PROFILING_COMMAND_RECEIVED_WWU:
        param_value = _received;
        break;
    default:
        _event.getProfilingInfo(param_name, &param_value);
        /* no break */
    }
}

void SimpleEvent::onExecutionStatusChanged(cl_int executionStatus) {
    dclasio::message::CommandExecutionStatusChangedMessage message(_id, executionStatus);

    try {
        /* Broadcast execution status to remote events */
        _context->host().sendMessage(message);
        sendMessage(_context->computeNodes(), message);

        dcl::util::Logger << dcl::util::Debug
                << "Sent update of command execution status (ID=" << _id
                << ", status=" << executionStatus << ')'
                << std::endl;
    } catch (const dcl::IOException& err) {
        dcl::util::Logger << dcl::util::Error
                << "Sending update of command execution status failed (ID=" << _id
                << ", status=" << executionStatus << ')'
                << std::endl;
    }
}

/* ****************************************************************************/

SimpleNodeEvent::SimpleNodeEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const cl::Event& event) :
    SimpleEvent(id, context, event) { }

void SimpleNodeEvent::onExecutionStatusChanged(cl_int executionStatus) {
    if (!_context->computeNodes().empty()) {
        dclasio::message::CommandExecutionStatusChangedMessage message(_id, executionStatus);

        try {
            /* Broadcast execution status to remote on other compute nodes.
             * No message has to be sent to the host. */
            sendMessage(_context->computeNodes(), message);

            dcl::util::Logger << dcl::util::Debug
                    << "Sent update of command execution status to compute nodes (ID=" << _id
                    << ", status=" << executionStatus << ')'
                    << std::endl;
        } catch (const dcl::IOException& err) {
            dcl::util::Logger << dcl::util::Error
                    << "Sending update of command execution status to compute nodes failed (ID=" << _id
                    << ", status=" << executionStatus << ')'
                    << std::endl;
        }
    }
}

/* ****************************************************************************/

CompoundEvent::CompoundEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const std::shared_ptr<Memory>& memoryObject,
        const cl::Event& startEvent,
        const cl::Event& endEvent) :
    LocalEvent(id, context, memoryObject), _startEvent(startEvent),
    _endEvent(endEvent) { }

CompoundEvent::CompoundEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const cl::Event& startEvent,
        const cl::Event& endEvent) :
    LocalEvent(id, context), _startEvent(startEvent), _endEvent(endEvent) { }

CompoundEvent::operator cl::Event() const {
    return _endEvent;
}

void CompoundEvent::getProfilingInfo(cl_profiling_info param_name,
        cl_ulong& param_value) const {
    switch (param_name) {
    case CL_PROFILING_COMMAND_RECEIVED_WWU:
        param_value = _received;
        break;
    case CL_PROFILING_COMMAND_QUEUED:
        /* no break */
    case CL_PROFILING_COMMAND_SUBMIT:
        /* no break */
    case CL_PROFILING_COMMAND_START:
        _startEvent.getProfilingInfo(param_name, &param_value);
        break;
    case CL_PROFILING_COMMAND_END:
        _endEvent.getProfilingInfo(param_name, &param_value);
        break;
    default:
        throw cl::Error(CL_INVALID_VALUE);
        /* no break */
    }
}

/* ****************************************************************************/

ReadMemoryEvent::ReadMemoryEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const cl::Event& startEvent,
        const cl::Event& endEvent) :
    CompoundEvent(id, context, startEvent, endEvent) { }

void ReadMemoryEvent::getProfilingInfo(
        cl_profiling_info param_name,
        cl_ulong& param_value) const {
    switch (param_name) {
    case CL_PROFILING_COMMAND_END:
        /* A read event is finished on the host when data receipt is complete.
         * In some cases, the receipt on the host may be complete while the
         * unmap operation on the daemon (associated with _endEvent) is not,
         * such that the profiling info for that event is not available yet.
         * Waiting for this event ensures that the unmap is complete before the
         * profiling info is obtained. */
        /* FIXME Waiting for an event must be timed to avoid deadlocks */
        _endEvent.wait();
        break;
    }

    CompoundEvent::getProfilingInfo(param_name, param_value);
}

void ReadMemoryEvent::onExecutionStatusChanged(cl_int executionStatus) {
    /* Do nothing - the read operation is finished by the host */
}

/* ****************************************************************************/

WriteMemoryEvent::WriteMemoryEvent(dcl::object_id id,
        const std::shared_ptr<Context>& context,
        const std::shared_ptr<Memory>& memoryObject,
        const cl::Event& startEvent,
        const cl::Event& endEvent) :
    CompoundEvent(id, context, memoryObject, startEvent, endEvent) {
    /* Schedule event status update notification */
    _endEvent.setCallback(CL_COMPLETE, &onEventComplete, this);
}

void WriteMemoryEvent::onExecutionStatusChanged(cl_int executionStatus) {
    if (!_context->computeNodes().empty()) {
        dclasio::message::CommandExecutionStatusChangedMessage message(_id, executionStatus);

        try {
            /* Broadcast execution status to remote on other compute nodes.
             * No message has to be sent to the host. */
            sendMessage(_context->computeNodes(), message);

            dcl::util::Logger << dcl::util::Debug
                    << "Sent update of command execution status to compute nodes (ID=" << _id
                    << ", status=" << executionStatus << ')'
                    << std::endl;
        } catch (const dcl::IOException& err) {
            dcl::util::Logger << dcl::util::Error
                    << "Sending update of command execution status to compute nodes failed (ID=" << _id
                    << ", status=" << executionStatus << ')'
                    << std::endl;
        }
    }
}

} /* namespace dcld */
