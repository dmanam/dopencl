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
 * \file EnqueueMapBuffer.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/EnqueueMapBuffer.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <vector>

namespace dclasio {
namespace message {

EnqueueMapBuffer::EnqueueMapBuffer() {
}

EnqueueMapBuffer::EnqueueMapBuffer(
        dcl::object_id commandQueueId,
        dcl::object_id commandId,
        dcl::object_id bufferId,
        bool blocking_read,
        cl_map_flags map_flags,
        size_t offset,
        size_t cb,
        const std::vector<dcl::object_id> *eventIdWaitList,
        bool event) :
        _commandQueueId(commandQueueId), _commandId(commandId), _bufferId(
                bufferId), _blocking(blocking_read), _mapFlags(map_flags), _offset(
                offset), _cb(cb), _event(event) {
    if (eventIdWaitList) {
        _eventIdWaitList = *eventIdWaitList;
    }
}

EnqueueMapBuffer::EnqueueMapBuffer(const EnqueueMapBuffer& rhs) :
        Request(rhs), _commandQueueId(rhs._commandQueueId), _commandId(
                rhs._commandId), _bufferId(rhs._bufferId), _blocking(
                rhs._blocking), _mapFlags(rhs._mapFlags), _offset(rhs._offset), _cb(
                rhs._cb), _eventIdWaitList(rhs._eventIdWaitList), _event(
                rhs._event) {
}

dcl::object_id EnqueueMapBuffer::commandQueueId() const {
    return _commandQueueId;
}

dcl::object_id EnqueueMapBuffer::commandId() const {
    return _commandId;
}

dcl::object_id EnqueueMapBuffer::bufferId() const {
    return _bufferId;
}

bool EnqueueMapBuffer::blocking() const {
    return _blocking;
}

cl_mem_flags EnqueueMapBuffer::mapFlags() const {
    return _mapFlags;
}

size_t EnqueueMapBuffer::offset() const {
    return _offset;
}

size_t EnqueueMapBuffer::cb() const {
    return _cb;
}

const std::vector<dcl::object_id>& EnqueueMapBuffer::eventIdWaitList() const {
    return _eventIdWaitList;
}

bool EnqueueMapBuffer::event() const {
    return _event;
}

} /* namespace message */
} /* namespace dclasio */
