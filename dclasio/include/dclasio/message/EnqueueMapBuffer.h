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
 * \file EnqueueMapBuffer.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef ENQUEUEMAPBUFFER_H_
#define ENQUEUEMAPBUFFER_H_

#include "Request.h"

#include <dcl/ByteBuffer.h>
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

class EnqueueMapBuffer: public Request {
public:
    EnqueueMapBuffer();
    EnqueueMapBuffer(
            dcl::object_id commandQueueId,
            dcl::object_id commandId,
            dcl::object_id bufferId,
            bool blocking_map,
            cl_map_flags map_flags,
            size_t offset,
            size_t cb,
            const std::vector<dcl::object_id> * eventIdWaitList = nullptr,
            bool event = false);
    EnqueueMapBuffer(const EnqueueMapBuffer& rhs);

    dcl::object_id commandQueueId() const;
    dcl::object_id commandId() const;
    dcl::object_id bufferId() const;
    bool blocking() const;
    cl_map_flags mapFlags() const;
    size_t offset() const;
    size_t cb() const;
    const std::vector<dcl::object_id>& eventIdWaitList() const;
    bool event() const;

    static const class_type TYPE = 100 + ENQUEUE_MAP_BUFFER;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _commandQueueId << _commandId << _bufferId << _blocking
                << _mapFlags << _offset << _cb << _eventIdWaitList << _event;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _commandQueueId >> _commandId >> _bufferId >> _blocking
                >> _mapFlags >> _offset >> _cb >> _eventIdWaitList >> _event;
    }

private:
    dcl::object_id _commandQueueId;
    dcl::object_id _commandId;
    dcl::object_id _bufferId;
    bool _blocking;
    cl_map_flags _mapFlags;
    size_t _offset;
    size_t _cb;
    std::vector<dcl::object_id> _eventIdWaitList;
    bool _event;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* ENQUEUEMAPBUFFER_H_ */
