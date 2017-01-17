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
 * \file EnqueueCopyBuffer.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/EnqueueCopyBuffer.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#include <cstddef>
#include <vector>

namespace dclasio {

namespace message {

EnqueueCopyBuffer::EnqueueCopyBuffer() :
        _commandQueueId(0), _commandId(0), _srcBufferId(0), _dstBufferId(0),
        _event(false) {
}

EnqueueCopyBuffer::EnqueueCopyBuffer(
        dcl::object_id commandQueueId,
        dcl::object_id commandId,
        dcl::object_id srcBufferId,
        dcl::object_id dstBufferId,
        size_t srcOffset,
        size_t dstOffset,
        size_t cb,
        const std::vector<dcl::object_id> *eventIdWaitList,
        bool event) :
        _commandQueueId(commandQueueId), _commandId(commandId),
        _srcBufferId(srcBufferId), _dstBufferId(dstBufferId),
        _srcOffset(srcOffset), _dstOffset(dstOffset), _cb(cb), _event(event) {
    if (eventIdWaitList) {
        _eventIdWaitList = *eventIdWaitList;
    }
}

EnqueueCopyBuffer::EnqueueCopyBuffer(const EnqueueCopyBuffer& rhs) :
        Request(rhs), _commandQueueId(rhs._commandQueueId), _commandId(
                rhs._commandId), _srcBufferId(rhs._srcBufferId), _dstBufferId(
                rhs._dstBufferId), _srcOffset(rhs._srcOffset), _dstOffset(
                rhs._dstOffset), _cb(rhs._cb), _eventIdWaitList(
                rhs._eventIdWaitList), _event(rhs._event) {
}

dcl::object_id EnqueueCopyBuffer::commandQueueId() const {
    return _commandQueueId;
}

dcl::object_id EnqueueCopyBuffer::commandId() const {
    return _commandId;
}

dcl::object_id EnqueueCopyBuffer::srcBufferId() const {
    return _srcBufferId;
}

dcl::object_id EnqueueCopyBuffer::dstBufferId() const {
    return _dstBufferId;
}

size_t EnqueueCopyBuffer::srcOffset() const {
    return _srcOffset;
}

size_t EnqueueCopyBuffer::dstOffset() const {
    return _dstOffset;
}

size_t EnqueueCopyBuffer::cb() const {
    return _cb;
}

const std::vector<dcl::object_id>& EnqueueCopyBuffer::eventIdWaitList() const {
    return _eventIdWaitList;
}

bool EnqueueCopyBuffer::event() const {
    return _event;
}

} /* namespace message */
} /* namespace dclasio */
