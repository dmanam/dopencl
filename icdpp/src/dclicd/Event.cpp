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
 * \date 2011-06-21
 * \author Philipp Kegel
 */

#include "Event.h"

#include "../Context.h"
#include "../CommandQueue.h"
#include "../Memory.h"
#include "../Platform.h"
#include "../Retainable.h"

#include "Error.h"
#include "utility.h"

#include "command/Command.h"

#include <dclasio/message/CommandMessage.h>
#include <dclasio/message/CreateEvent.h>
#include <dclasio/message/EventProfilingInfosResponse.h>
#include <dclasio/message/EventSynchronizationMessage.h>
#include <dclasio/message/GetEventProfilingInfos.h>

#include <dcl/CLError.h>
#include <dcl/CLObjectRegistry.h>
#include <dcl/ComputeNode.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>
#include <dcl/Remote.h>
#include <dcl/SynchronizationListener.h>

#include <dcl/util/Clock.h>
#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdlib> // abort
#include <functional>
#include <iterator>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace dclicd {

/******************************************************************************
 * Event
 ******************************************************************************/

Event::Event(cl_context context, 
	const std::shared_ptr<command::Command>& command,
	const std::vector<cl_mem>& memoryObjects) :
	_cl_event(context, CL_QUEUED),
	_command(command), _commandQueued(dcl::util::clock.getTime()),
	_memoryObjects(memoryObjects)
{
    assert(command != nullptr); // command must not be NULL
#ifndef NDEBUG
    /* Context must be context of command's command queue */
    command->commandQueue()->getInfo(CL_QUEUE_CONTEXT, sizeof(context), &context, nullptr);
    assert(context == _context);
#endif

    /* register event (required for consistency protocol) */
    _context->getPlatform()->remote().objectRegistry().bind<dcl::SynchronizationListener>(_command->remoteId(), *this);

	try {
		std::vector<dcl::object_id> memoryObjectIds;
		
		for (auto memoryObject : memoryObjects) {
		    memoryObjectIds.push_back(memoryObject->remoteId());
		}
		
		dclasio::message::CreateEvent createEvent(_context->remoteId(),
				_command->remoteId(), memoryObjectIds);
		std::vector<dcl::ComputeNode *> computeNodes(_context->computeNodes());

		/* Create list of 'other' compute nodes.
		 * Other compute nodes are compute nodes where no command has been
		 * enqueued but which belong to the same context as the command queue
		 * where the associated command has been enqueued. */
		computeNodes.erase(std::find(std::begin(computeNodes), std::end(computeNodes),
				&_command->commandQueue()->computeNode()));

		/* Create substitute events on other compute nodes */
		dcl::executeCommand(computeNodes, createEvent);
		dcl::util::Logger << dcl::util::Info
				<< "Event created (ID=" << _command->remoteId() << ')'
				<< std::endl;
	} catch (const dcl::CLError& err) {
		throw Error(err);
	} catch (const dcl::IOException& err) {
		throw Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw Error(err);
	}

    _command->setEvent(*this); // attach event to local command
	_command->commandQueue()->retain();
}

Event::~Event() {
    dclicd::release(_command->commandQueue());

	/* deregister event */
    _context->getPlatform()->remote().objectRegistry().unbind<dcl::SynchronizationListener>(remoteId());
}

dcl::object_id Event::remoteId() const {
	return _command->remoteId();
}

void Event::wait() const {
	/* Perform an implicit flush to ensure that the associated command will be
	 * executed eventually */
	_command->commandQueue()->flush();

	/* Do not wait for the associated command's execution status but on the
	 * event's one as the event status also includes the completion of
	 * operations associated with the event, e.g., triggering the callbacks. */
	waitNoFlush();
}

bool Event::onCommandExecutionStatusChanged(cl_int status) {
    /* TODO Send event status update to other compute nodes directly from the
     *      compute node hosting the original event.
     * The following is a work-around for missing node-to-node communication.
     * However, commands that are finished by the host, e.g., 'read buffer',
     * still have to send the event status update. */
	if (status < 0 || status == CL_COMPLETE) { /* command failed or has been completed */
		std::vector<dcl::ComputeNode *> computeNodes = _context->computeNodes();

		/*
		 * Forward event status change to other compute nodes in context
		 * The event status has been set to an error code or to 'complete' on
		 * the compute node owning the event. The statuses of the corresponding
		 * substitute events on other compute nodes of the context now have to
		 * be updated accordingly.
		 */
		computeNodes.erase(std::find(std::begin(computeNodes), std::end(computeNodes),
				&_command->commandQueue()->computeNode()));
		if (!computeNodes.empty()) {
            try {
                dclasio::message::CommandExecutionStatusChangedMessage message(remoteId(), status);

                dcl::sendMessage(computeNodes, message);
                dcl::util::Logger << dcl::util::Debug
                        << "Forwarded update of command execution status to compute nodes (ID=" << remoteId()
                        << ", status=" << status
                        << ')' << std::endl;
            } catch (const dcl::DCLException& err) {
                /* Application state has become inconsistent, abort */
                std::cerr << "ERROR: event status update failed" << std::endl;
                abort();
            }
		}
	}

	return setCommandExecutionStatus(status);
}

void Event::getProfilingInfo(
		cl_kernel_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) const {
    /*
     * Do NOT check locally if event is complete!
     * Remote event may be complete after _cl_command_queue::finish, but local
     * event still can be incomplete because of message latency
	if (!isComplete()) throw Error(CL_PROFILING_INFO_NOT_AVAILABLE);
     */

	if (!_profilingInfo) {
		/* Query profiling info from compute node */
		try {
			dclasio::message::GetEventProfilingInfos request(remoteId());
			std::unique_ptr<dclasio::message::EventProfilingInfosReponse> response(
					static_cast<dclasio::message::EventProfilingInfosReponse *>(
							_command->commandQueue()->computeNode().executeCommand(
									request, dclasio::message::EventProfilingInfosReponse::TYPE).release()));

			_profilingInfo.reset(new detail::EventProfilingInfo(
			        response->received,response->queued, response->submit,
			        response->start, response->end));
		} catch (const std::bad_alloc&) {
			throw Error(CL_OUT_OF_HOST_MEMORY);
		} catch (const dcl::CLError& err) {
			throw Error(err);
		} catch (const dcl::IOException& err) {
			throw Error(err);
		} catch (const dcl::ProtocolException& err) {
			throw Error(err);
		}
	}

    switch (param_name) {
    case CL_PROFILING_COMMAND_QUEUED:
        /* time of queuing the command on the host */
        copy_info(_commandQueued, param_value_size, param_value,
                param_value_size_ret);
        break;
    case CL_PROFILING_COMMAND_RECEIVED_WWU:
        /* time of receiving/queuing the command on the compute node */
        copy_info(_profilingInfo->received(), param_value_size,
                param_value, param_value_size_ret);
        break;
    case CL_PROFILING_COMMAND_SUBMIT:
        copy_info(_profilingInfo->submit(), param_value_size,
                param_value, param_value_size_ret);
        break;
    case CL_PROFILING_COMMAND_START:
        copy_info(_profilingInfo->start(), param_value_size,
                param_value, param_value_size_ret);
        break;
    case CL_PROFILING_COMMAND_END:
        copy_info(_profilingInfo->end(), param_value_size,
                param_value, param_value_size_ret);
        break;
    default:
        throw Error(CL_INVALID_VALUE);
    }
}

cl_command_type Event::commandType() const {
	return _command->type();
}

cl_command_queue Event::commandQueue() const {
	return _command->commandQueue();
}

void Event::synchronize() {
    /* TODO Implement Event::synchronize
     * 1. Send synchronization request to compute node
     *    Presumably, this step can be omitted as a synchronization request is
     *    implied by clWaitForEvents and clFinish, which are the only APIs (?)
     *    that require event synchronization on the host.
     * 2. Acquire associated memory object changes */
}

void Event::onSynchronize(dcl::Process& process) {
    dcl::util::Logger << dcl::util::Debug
            << "(MEM) Event synchronization (ID=" << remoteId()
            << ") requested by compute node '" << process.url() << '\''
            << std::endl;

    if (_memoryObjects.empty()) return;

    /* forward synchronization request to event's compute node */
    dclasio::message::EventSynchronizationMessage msg(remoteId());
    _command->commandQueue()->computeNode().sendMessage(msg);
    dcl::util::Logger << dcl::util::Debug
            << "(MEM) Forwarded event synchronization request (ID=" << remoteId()
            << ") to compute node '" << _command->commandQueue()->computeNode().url() << '\''
            << std::endl;

    /* TODO Implement forwarding of acquire request in this method rather than using a memory object's mechanisms
     * The acquire/release operations will be implemented on the host to
     * acquire/release the associated changes of the memory objects from a
     * compute node to the memory objects' host pointers. These operations must
     * not interfere with synchronization operations between compute nodes that
     * are currently mediated by the host  (work-around for missing
     * node-to-node communication). */

    /* acquire and release memory objects from event's compute node */
    for (auto memoryObject : _memoryObjects) {
        memoryObject->onAcquire(process, _command->commandQueue()->computeNode());
    }
}

/******************************************************************************
 * User event
 ******************************************************************************/

UserEvent::UserEvent(cl_context context) :
	_cl_event(context, CL_SUBMITTED)
{
	try {
		dclasio::message::CreateEvent request(context->remoteId(), _id,
		        std::vector<dcl::object_id>());
		dcl::executeCommand(_context->computeNodes(), request);
		dcl::util::Logger << dcl::util::Info
				<< "User event created (ID=" << _id << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw Error(err);
	} catch (const dcl::IOException& err) {
		throw Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw Error(err);
	}

	_context->retain();
}

UserEvent::~UserEvent() {
    dclicd::release(_context);
}

dcl::object_id UserEvent::remoteId() const {
	return _id;
}

void UserEvent::wait() const {
	/*
	 * User events are not associated with a command queue. Therefore, no
	 * command queue is flushed when waiting for a user event.
	 */
	waitNoFlush();
}

void UserEvent::setStatus(cl_int status) {
	if (status > 0) {
		/* Event status to set must be either CL_COMPLETE or a negative integer
		 * value */
		throw Error(CL_INVALID_VALUE);
	}

	{
		std::lock_guard<std::recursive_mutex> lock(_statusMutex);
		if (_status < 0	|| _status == CL_COMPLETE) {
			/* Event status has already been set */
			throw Error(CL_INVALID_OPERATION);
		}
	}

	/*
	 * Broadcast user event status update
	 */
	try {
	    dclasio::message::CommandExecutionStatusChangedMessage request(_id, status);
		dcl::sendMessage(_context->computeNodes(), request);
		dcl::util::Logger << dcl::util::Info
				<< "User event status set (ID=" << remoteId()
				<< ", status=" << status
				<< ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw Error(err);
	} catch (const dcl::IOException& err) {
		throw Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw Error(err);
	}

	setCommandExecutionStatus(status);
}

void UserEvent::getProfilingInfo(
		cl_profiling_info /* param_name */,
		size_t            /* param_value_size */,
		void *            /* param_value */,
		size_t *          /* param_value_size_ret */) const {
	/* Profiling is never available for user events */
	throw Error(CL_PROFILING_INFO_NOT_AVAILABLE);
}

cl_command_type UserEvent::commandType() const {
	/* User events always are associated with a user command */
	return CL_COMMAND_USER;
}

cl_command_queue UserEvent::commandQueue() const {
	/* User events are not associated with a command queue */
	return nullptr;
}

} /* namespace dclicd */
