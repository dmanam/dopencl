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
 * \file EnqueueNDRangeKernel.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/EnqueueNDRangeKernel.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#include <cassert>
#include <cstddef>
#include <vector>

namespace dclasio {
namespace message {

EnqueueNDRangeKernel::EnqueueNDRangeKernel() :
        _commandQueueId(0), _commandId(0), _kernelId(0), _event(false) {
}

EnqueueNDRangeKernel::EnqueueNDRangeKernel(
        dcl::object_id commandQueueId,
        dcl::object_id commandId,
        dcl::object_id kernelId,
        const std::vector<size_t>& offset,
        const std::vector<size_t>& global,
        const std::vector<size_t>& local,
        const std::vector<dcl::object_id> *eventIdWaitList,
        bool event) :
        _commandQueueId(commandQueueId), _commandId(commandId), _kernelId(
                kernelId), _offset(offset), _global(global), _local(local), _event(
                event) {
    if (eventIdWaitList) {
        _eventIdWaitList = *eventIdWaitList;
    }
}

EnqueueNDRangeKernel::EnqueueNDRangeKernel(const EnqueueNDRangeKernel& rhs) :
        Request(rhs), _commandQueueId(rhs._commandQueueId), _commandId(
                rhs._commandId), _kernelId(rhs._kernelId), _global(rhs._global), _local(
                rhs._local), _eventIdWaitList(rhs._eventIdWaitList), _event(
                rhs._event) {
}

dcl::object_id EnqueueNDRangeKernel::commandQueueId() const {
    return _commandQueueId;
}

dcl::object_id EnqueueNDRangeKernel::commandId() const {
    return _commandId;
}

dcl::object_id EnqueueNDRangeKernel::kernelId() const {
    return _kernelId;
}

const std::vector<size_t>& EnqueueNDRangeKernel::offset() const {
    return _offset;
}

const std::vector<size_t>& EnqueueNDRangeKernel::global() const {
    return _global;
}

const std::vector<size_t>& EnqueueNDRangeKernel::local() const {
    return _local;
}

const std::vector<dcl::object_id>& EnqueueNDRangeKernel::eventIdWaitList() const {
    return _eventIdWaitList;
}

bool EnqueueNDRangeKernel::event() const {
    return _event;
}

} /* namespace message */
} /* namespace dclasio */
