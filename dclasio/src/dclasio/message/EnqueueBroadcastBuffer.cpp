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
 * \file EnqueueBroadcastBuffer.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/EnqueueBroadcastBuffer.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#include <cstddef>
#include <vector>

namespace dclasio {
namespace message {

EnqueueBroadcastBuffer::EnqueueBroadcastBuffer() :
	_commandId(0), _srcBufferId(0),	_event(false) {
}

EnqueueBroadcastBuffer::EnqueueBroadcastBuffer(
		const std::vector<dcl::object_id>& commandQueueIds,
		dcl::object_id commandId,
		dcl::object_id srcBufferId,
		const std::vector<dcl::object_id>& dstBufferIds,
		size_t srcOffset,
		const std::vector<size_t>& dstOffsets,
		size_t cb,
		const std::vector<dcl::object_id> *eventIdWaitList,
		bool event) :
	_commandQueueIds(commandQueueIds), _commandId(commandId),
			_srcBufferId(srcBufferId), _dstBufferIds(dstBufferIds),
			_srcOffset(srcOffset), _dstOffsets(dstOffsets), _cb(cb), _event(event)
{
	if (eventIdWaitList) {
		_eventIdWaitList = *eventIdWaitList;
	}
}

EnqueueBroadcastBuffer::EnqueueBroadcastBuffer(const EnqueueBroadcastBuffer& rhs) :
	Request(rhs), _commandQueueIds(rhs._commandQueueIds), _commandId(
			rhs._commandId), _srcBufferId(rhs._srcBufferId), _dstBufferIds(
			rhs._dstBufferIds), _srcOffset(rhs._srcOffset), _dstOffsets(
			rhs._dstOffsets), _cb(rhs._cb), _eventIdWaitList(
			rhs._eventIdWaitList), _event(rhs._event)
{
}

const std::vector<dcl::object_id>& EnqueueBroadcastBuffer::commandQueueIds() const {
	return _commandQueueIds;
}

dcl::object_id EnqueueBroadcastBuffer::commandId() const {
	return _commandId;
}

dcl::object_id EnqueueBroadcastBuffer::srcBufferId() const {
	return _srcBufferId;
}

const std::vector<dcl::object_id>& EnqueueBroadcastBuffer::dstBufferIds() const {
	return _dstBufferIds;
}

size_t EnqueueBroadcastBuffer::srcOffset() const {
	return _srcOffset;
}

const std::vector<size_t>& EnqueueBroadcastBuffer::dstOffsets() const {
	return _dstOffsets;
}

size_t EnqueueBroadcastBuffer::cb() const {
	return _cb;
}

const std::vector<dcl::object_id>& EnqueueBroadcastBuffer::eventIdWaitList() const {
	return _eventIdWaitList;
}

bool EnqueueBroadcastBuffer::event() const {
	return _event;
}

} /* namespace message */
} /* namespace dclasio */
