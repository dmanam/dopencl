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
 * \file EnqueueWaitForEvents.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/EnqueueWaitForEvents.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#include <vector>

namespace dclasio {
namespace message {

EnqueueWaitForEvents::EnqueueWaitForEvents() {
}

EnqueueWaitForEvents::EnqueueWaitForEvents(
        dcl::object_id commandQueueId,
        const std::vector<dcl::object_id>& eventIdList) :
        _commandQueueId(commandQueueId), _eventIdList(eventIdList) {
}

EnqueueWaitForEvents::EnqueueWaitForEvents(const EnqueueWaitForEvents& rhs) :
        Request(rhs), _commandQueueId(rhs._commandQueueId), _eventIdList(
                rhs._eventIdList) {
}

dcl::object_id EnqueueWaitForEvents::commandQueueId() const {
    return _commandQueueId;
}

const std::vector<dcl::object_id>& EnqueueWaitForEvents::eventIdList() const {
    return _eventIdList;
}

} /* namespace message */
} /* namespace dclasio */
