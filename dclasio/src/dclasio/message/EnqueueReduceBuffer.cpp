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
 * \file EnqueueReduceBuffer.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/EnqueueReduceBuffer.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#include <cassert>
#include <cstddef>
#include <vector>

namespace dclasio {
namespace message {

EnqueueReduceBuffer::EnqueueReduceBuffer() :
        _commandId(0), _dstId(0), _kernelId(0), _event(false) {
}

EnqueueReduceBuffer::EnqueueReduceBuffer(
        dcl::object_id commandQueueId,
        dcl::object_id commandId,
        const std::vector<dcl::object_id>& srcIds,
        dcl::object_id dstId,
        dcl::object_id kernelId,
        const std::vector<size_t>& offset,
        const std::vector<size_t>& global,
        const std::vector<size_t>& local,
        const std::vector<dcl::object_id> *eventIdWaitList,
        bool event) :
        _commandQueueId(commandQueueId), _commandId(commandId), _srcIds(srcIds), _dstId(
                dstId), _kernelId(kernelId), _offset(offset), _global(global), _local(
                local), _event(event) {
    if (eventIdWaitList) {
        _eventIdWaitList = *eventIdWaitList;
    }
}

EnqueueReduceBuffer::EnqueueReduceBuffer(const EnqueueReduceBuffer& rhs) :
        Request(rhs), _commandQueueId(rhs._commandQueueId), _commandId(
                rhs._commandId), _srcIds(rhs._srcIds), _dstId(rhs._dstId), _kernelId(
                rhs._kernelId), _global(rhs._global), _local(rhs._local), _eventIdWaitList(
                rhs._eventIdWaitList), _event(rhs._event) {
}

dcl::object_id EnqueueReduceBuffer::commandQueueId() const {
    return _commandQueueId;
}

dcl::object_id EnqueueReduceBuffer::commandId() const {
    return _commandId;
}

const std::vector<dcl::object_id>& EnqueueReduceBuffer::srcIds() const {
    return _srcIds;
}

dcl::object_id EnqueueReduceBuffer::dstId() const {
    return _dstId;
}

dcl::object_id EnqueueReduceBuffer::kernelId() const {
    return _kernelId;
}

const std::vector<size_t>& EnqueueReduceBuffer::offset() const {
    return _offset;
}

const std::vector<size_t>& EnqueueReduceBuffer::global() const {
    return _global;
}

const std::vector<size_t>& EnqueueReduceBuffer::local() const {
    return _local;
}

const std::vector<dcl::object_id>& EnqueueReduceBuffer::eventIdWaitList() const {
    return _eventIdWaitList;
}

bool EnqueueReduceBuffer::event() const {
    return _event;
}

} /* namespace message */
} /* namespace dclasio */
