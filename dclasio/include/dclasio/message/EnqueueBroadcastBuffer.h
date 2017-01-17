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
 * \file EnqueueBroadcastBuffer.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef ENQUEUEBROADCASTBUFFER_H_
#define ENQUEUEBROADCASTBUFFER_H_

#include "Request.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <cstddef>
#include <vector>

namespace dclasio {
namespace message {

class EnqueueBroadcastBuffer : public Request {
public:
    EnqueueBroadcastBuffer();
	EnqueueBroadcastBuffer(
			const std::vector<dcl::object_id>&  commandQueueIds,
			dcl::object_id                      commandId,
			dcl::object_id                      srcBufferId,
			const std::vector<dcl::object_id>&  dstBufferIds,
			size_t                              srcOffset,
			const std::vector<size_t>&          dstOffsets,
			size_t                              cb,
			const std::vector<dcl::object_id> * eventIdWaitList = nullptr,
			bool                                event = false);
	EnqueueBroadcastBuffer(
	        const EnqueueBroadcastBuffer& rhs);

	const std::vector<dcl::object_id>& commandQueueIds() const;
    dcl::object_id commandId() const;
    dcl::object_id srcBufferId() const;
    const std::vector<dcl::object_id>& dstBufferIds() const;
    size_t srcOffset() const;
    const std::vector<size_t>& dstOffsets() const;
    size_t cb() const;
    const std::vector<dcl::object_id>& eventIdWaitList() const;
    bool event() const;

    static const class_type TYPE = 100 + ENQUEUE_BROADCAST_BUFFER;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _commandQueueIds << _commandId << _srcBufferId << _dstBufferIds
                << _srcOffset << _dstOffsets << _cb << _eventIdWaitList << _event;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _commandQueueIds >> _commandId >> _srcBufferId >> _dstBufferIds
                >> _srcOffset >> _dstOffsets >> _cb >> _eventIdWaitList >> _event;
    }

private:
    std::vector<dcl::object_id> _commandQueueIds;
    dcl::object_id _commandId;
    dcl::object_id _srcBufferId;
    std::vector<dcl::object_id> _dstBufferIds;
    size_t _srcOffset;
    std::vector<size_t> _dstOffsets;
    size_t _cb;
    std::vector<dcl::object_id> _eventIdWaitList;
    bool _event;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* ENQUEUEBROADCASTBUFFER_H_ */
