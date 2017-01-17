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
 * \file Command.cpp
 *
 * \date 2012-03-24
 * \author Philipp Kegel
 */

#include "Command.h"

#include "../../CommandQueue.h"
#include "../../Platform.h"
#include "../../Retainable.h"

#include "../Error.h"
#include "../Event.h"

#include <dcl/CLObjectRegistry.h>
#include <dcl/CommandListener.h>
#include <dcl/Remote.h>
#include <dcl/ComputeNode.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <ostream>

namespace dclicd {

namespace command {

Command::Command(cl_command_type type, cl_command_queue commandQueue) :
	_type(type), _commandQueue(commandQueue),
	_executionStatus(CL_QUEUED), _event(nullptr)
{
    cl_context context;

    assert(commandQueue != nullptr); // command queue must not be NULL

	// listen to remote command
    _commandQueue->getInfo(CL_QUEUE_CONTEXT, sizeof(context), &context, nullptr);
    context->getPlatform()->remote().objectRegistry().bind<dcl::CommandListener>(_id, *this);
}

Command::~Command() {
    cl_context context;

    _commandQueue->getInfo(CL_QUEUE_CONTEXT, sizeof(context), &context, nullptr);
    context->getPlatform()->remote().objectRegistry().unbind<dcl::CommandListener>(_id);
}

cl_command_type Command::type() const {
	return _type;
}

cl_command_queue Command::commandQueue() const {
	return _commandQueue;
}

void Command::setEvent(Event& event) {
	std::lock_guard<std::recursive_mutex> lock(_executionStatusMutex);

	assert(_event == nullptr); // event must be set only once

	_event = &event;
#ifdef NDEBUG
    _event->onCommandExecutionStatusChanged(_executionStatus);
#else
    assert(!_event->onCommandExecutionStatusChanged(_executionStatus));
#endif
}

bool Command::isComplete() const {
	std::lock_guard<std::recursive_mutex> lock(_executionStatusMutex);
	return (_executionStatus < 0 /* error */
			|| _executionStatus == CL_COMPLETE);
}

void Command::wait() const {
	std::lock_guard<std::recursive_mutex> lock(_executionStatusMutex);
	while (_executionStatus != CL_COMPLETE && _executionStatus >= 0) {
		_executionStatusChanged.wait(_executionStatusMutex);
	}
}

void Command::onExecutionStatusChanged(cl_int executionStatus) {
#ifndef NDEBUG
	/* Ensure that command execution status only changes in the following
	 * manner: QUEUED -> SUBMIT -> RUNNING -> COMPLETE | <error code> */
	{
		std::lock_guard<std::recursive_mutex> lock(_executionStatusMutex);
		assert(_executionStatus > 0 && executionStatus < _executionStatus);
	}
#endif

    try {
        if (executionStatus == CL_SUBMITTED) {
                executionStatus = submit();
        }
        if (executionStatus == CL_COMPLETE || executionStatus < 0) {
                executionStatus = complete(executionStatus);
        }
    } catch (const Error& err) {
        executionStatus = err.err();
    }

	{
		std::lock_guard<std::recursive_mutex> lock(_executionStatusMutex);
		/* executionStatus may have been changed concurrently, e.g., by an
		 * operation in method submit */
		if (executionStatus < _executionStatus) {
			_executionStatus = executionStatus;
            dcl::util::Logger << dcl::util::Debug
                    << "Changed command execution status (ID=" << _id
                    << ", status=" << _executionStatus << ')'
                    << std::endl;

			if (_event) {
				// update command execution status of associated event
				if (_event->onCommandExecutionStatusChanged(_executionStatus)) {
				    /* WARNING: When the event is deleted, it releases this
				     * command. There must be another owner of this command to
				     * ensure that the command is not deleted while this method
				     * in executed.
				     * Currently, the command queue is the other owner of a
				     * command. Unlike the event, the command queue waits for
				     * the command to complete (see CommandQueue::finishLocally)
				     * before deleting it. As waiting for a command is
				     * synchronized, the command will not be deleted before this
				     * method has been completely executed. */
				    // FIXME Ensure that event has been created with new
				    delete _event;
				    /* Do not set _event = nullptr, such that no other event can
				     * be attached to this command using setEvent. */
				}
			}

			_executionStatusChanged.notify_all();
		}
	}
}

cl_int Command::submit() {
	// no action
	return CL_RUNNING;
}

cl_int Command::complete(cl_int errcode) {
    // no action
    return errcode;
}

} /* namespace command */

} /* namespace dclicd */
