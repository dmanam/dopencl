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

/**
 * @file CreateCommandQueue.cpp
 *
 * @date 2011-05-31
 * @author Tunc Taylan Turunc
 */

#include <dclasio/message/CreateCommandQueue.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cassert>
#include <cstddef>

namespace dclasio {
namespace message {

CreateCommandQueue::CreateCommandQueue() {
}

CreateCommandQueue::CreateCommandQueue(dcl::object_id contextId,
		dcl::object_id deviceId,
		dcl::object_id commandQueueId,
		cl_command_queue_properties properties) :
		_contextId(contextId),
		_deviceId(deviceId),
		_commandQueueId(commandQueueId),
		_properties(properties) {
}

CreateCommandQueue::CreateCommandQueue(const CreateCommandQueue& rhs) : Request(rhs),
		_contextId(rhs._contextId),
		_deviceId(rhs._deviceId),
		_commandQueueId(rhs._commandQueueId),
		_properties(rhs._properties) {
}

dcl::object_id CreateCommandQueue::contextId() const {
	return _contextId;
}

dcl::object_id CreateCommandQueue::deviceId() const {
	return _deviceId;
}

dcl::object_id CreateCommandQueue::commandQueueId() const {
	return _commandQueueId;
}

cl_command_queue_properties CreateCommandQueue::properties() const {
	return _properties;
}

} /* namespace message */
} /* namespace dclasio */
