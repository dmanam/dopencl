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
 * \author Louay Hashim
 */

#include "Event.h"

#include "Context.h"
#include "CommandQueue.h"
#include "Retainable.h"

#include "dclicd/Error.h"
#include "dclicd/utility.h"

#include <dclasio/message/DeleteEvent.h>

#include <dcl/CLError.h>
#include <dcl/ComputeNode.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <iterator>
#include <map>
#include <mutex>
#include <ostream>
#include <set>
#include <utility>
#include <vector>


_cl_event::_cl_event(cl_context context, cl_int status) :
	_context(context), _status(status) {
    if (!context) { throw dclicd::Error(CL_INVALID_CONTEXT); }
}

_cl_event::~_cl_event() { }

void _cl_event::retain() {
    std::lock_guard<std::recursive_mutex> lock(_statusMutex);
    ++_ref_count;
}

bool _cl_event::release() {
    std::lock_guard<std::recursive_mutex> lock(_statusMutex);
    cl_uint refCount;

    refCount = _ref_count--;
    assert(refCount > 0); // _ref_count must be greater than zero when calling release

    if (refCount == 1 && isComplete()) {
        destroy();
        return true;
    } else {
        return false;
    }
}

void _cl_event::destroy() {
    /* Events must only be deleted if their reference count is 0 *and* their
     * associated command is completed (or terminated).
     * The event is required to forward its execution status to remote events.
     *
     * Note that the event on the host does not have to be retained for
     * enqueued commands that require a wait for this event as the remote
     * events are implicitly retained by the compute nodes' OpenCL
     * implementations. */
	assert(_ref_count == 0);
	assert(isComplete());

	try {
		dclasio::message::DeleteEvent request(remoteId());
		dcl::executeCommand(_context->computeNodes(), request);
		dcl::util::Logger << dcl::util::Info
				<< "Event deleted (ID=" << remoteId() << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

void _cl_event::waitForEvents(const std::vector<cl_event>& event_list) {
	cl_event event;
	cl_context context;
	cl_command_queue queue;
	std::set<cl_command_queue> queues;

	if (event_list.empty()) throw dclicd::Error(CL_INVALID_VALUE);

	/*
	 * Validate first event in list
	 */
	event = event_list.front();
	if (!event) throw dclicd::Error(CL_INVALID_EVENT);

	context = event->_context;

	queue = event->commandQueue();
	if (queue) {
		queues.insert(queue);
	}

	/*
	 * Validate remaining events
	 */
	for (auto event : event_list) {
		if (!event) throw dclicd::Error(CL_INVALID_EVENT);
		if (event->_context != context) throw dclicd::Error(CL_INVALID_CONTEXT);

		queue = event->commandQueue();
		if (queue) {
			queues.insert(queue);
		}
	}

	/* Flush command queues */
	for (auto queue : queues) {
	    queue->flush();
	}

	/*
	 * Wait for events
	 */
	try {
		/* Here, @c waitNoFlush is used to avoid redundant flushing of command queues. */
	    for (auto event : event_list) {
	        event->waitNoFlush();
	    }
	} catch (const dclicd::Error&) {
		/* the execution status of any of the events in event_list is a negative
		 * integer value */
		throw dclicd::Error(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
	}
}

void _cl_event::setCallback(
		cl_int command_exec_callback_type,
		void (CL_CALLBACK *pfn_event_notify)(
				cl_event event,
				cl_int event_command_exec_status,
				void *user_data),
		void *user_data) {
	if (command_exec_callback_type != CL_COMPLETE) throw dclicd::Error(CL_INVALID_VALUE);
	if (!pfn_event_notify) throw dclicd::Error(CL_INVALID_VALUE);

	{
		std::lock_guard<std::recursive_mutex> lock(_statusMutex);

		/*
		 * The following mechanism is the reason an event requires its own
		 * command execution status rather than querying it from its associated
		 * command.
		 * When a callback is set, it must be called immediately if the event's
		 * command execution status already is equal to (or lower than) the
		 * callback's command execution status. However, this status must not
		 * change between adding the callback to the list and checking if the
		 * callback should be triggered immediately.
		 * Therefore, adding the callback and checking the event's command
		 * execution status must be performed atomically.
		 */
		if (_status <= command_exec_callback_type) {
			pfn_event_notify(this, CL_COMPLETE, user_data);
		}

		/* Add callback to list */
		_callbacks[command_exec_callback_type].push_back(
				std::make_pair(pfn_event_notify, user_data));
	}
}

void _cl_event::getInfo(
		cl_event_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) const {
	switch (param_name) {
	case CL_EVENT_COMMAND_QUEUE:
		dclicd::copy_info(commandQueue(), param_value_size,
				param_value, param_value_size_ret);
		break;
	case CL_EVENT_CONTEXT:
		dclicd::copy_info(_context, param_value_size,
				param_value, param_value_size_ret);
		break;
	case CL_EVENT_COMMAND_EXECUTION_STATUS:
    {
        std::lock_guard<std::recursive_mutex> lock(_statusMutex);
		dclicd::copy_info(_status, param_value_size,
			param_value, param_value_size_ret);
		break;
    }
	case CL_EVENT_REFERENCE_COUNT:
	{
		dclicd::copy_info(_ref_count, param_value_size,
				param_value, param_value_size_ret);
		break;
	}
	case CL_EVENT_COMMAND_TYPE:
		dclicd::copy_info(commandType(), param_value_size,
				param_value, param_value_size_ret);
		break;
	default:
		throw dclicd::Error(CL_INVALID_VALUE);
	}
}

bool _cl_event::isComplete() const {
	return (_status < 0 /* terminated */
			|| _status == CL_COMPLETE);
}

void _cl_event::waitNoFlush() const {
	std::lock_guard<std::recursive_mutex> lock(_statusMutex);
	while (_status > CL_COMPLETE) _statusChanged.wait(_statusMutex);
}

bool _cl_event::setCommandExecutionStatus(cl_int status) {
    std::lock_guard<std::recursive_mutex> lock(_statusMutex);

    /* Trigger callback's *before* setting the event complete. Otherwise, an
     * application thread waiting for the event to complete may be resumed and
     * then releases *and* deletes the event concurrently.
     *
     * In order to allow callbacks to call methods of the event, the event's
     * lock has to be reentrant. */
    triggerCallbacks(status);

    _status = status;
    _statusChanged.notify_all();

    if (_ref_count == 0 && isComplete()) {
        destroy();
        return true;
    } else {
        return false;
    }
}

void _cl_event::triggerCallbacks(cl_int status) {
	/* FIXME Event callbacks should be able to release their event
	 * Though, an event must not be deleted (by the implementation) before all
	 * callbacks have been called, as the event is passed as an argument. */

	/*
	 * Call callback functions registered for CL_COMPLETE if the event's
	 * command has completed successfully or is abnormally terminated (i.e.,
	 * status < 0).
	 */
	auto i = _callbacks.find((status <= CL_COMPLETE) ? CL_COMPLETE : status);
	if (i != std::end(_callbacks)) {
		for (auto callback : i->second) {
		    callback.first(this, status, callback.second);
		}
	}
}
