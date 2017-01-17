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
 * \file EnqueueReduceBuffer.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef ENQUEUEREDUCEBUFFER_H_
#define ENQUEUEREDUCEBUFFER_H_

#include "Request.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <cstddef>
#include <vector>

namespace dclasio {
namespace message {

class EnqueueReduceBuffer : public Request {
public:
    EnqueueReduceBuffer();
	EnqueueReduceBuffer(
			dcl::object_id                      commandQueueId,
			dcl::object_id                      commandId,
			const std::vector<dcl::object_id>&  srcIds,
			dcl::object_id                      dstId,
			dcl::object_id                      kernelId,
			const std::vector<size_t>&          offset,
			const std::vector<size_t>&          lobal,
			const std::vector<size_t>&          local,
			const std::vector<dcl::object_id> * eventIdWaitList = nullptr,
			bool                                event = false);
	EnqueueReduceBuffer(
	        const EnqueueReduceBuffer& rhs);

	dcl::object_id commandQueueId() const;
	dcl::object_id commandId() const;
	const std::vector<dcl::object_id>& srcIds() const;
	dcl::object_id dstId() const;
	dcl::object_id kernelId() const;

	const std::vector<size_t>& offset() const;
	const std::vector<size_t>& global() const;
	const std::vector<size_t>& local() const;

    const std::vector<dcl::object_id>& eventIdWaitList() const;
	bool event() const;

    static const class_type TYPE = 100 + ENQUEUE_REDUCE_BUFFER;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _commandQueueId << _commandId << _srcIds << _dstId << _kernelId
                << _offset << _global << _local <<  _eventIdWaitList << _event;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _commandQueueId >> _commandId >> _srcIds >>  _dstId >> _kernelId
                >> _offset >> _global >> _local >>  _eventIdWaitList >> _event;
    }

private:
	dcl::object_id _commandQueueId;
	dcl::object_id _commandId;
	std::vector<dcl::object_id> _srcIds;
	dcl::object_id _dstId;
	dcl::object_id _kernelId;
	std::vector<size_t> _offset;
	std::vector<size_t> _global;
	std::vector<size_t> _local;
    std::vector<dcl::object_id> _eventIdWaitList;
	bool _event;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* ENQUEUEREDUCEBUFFER_H_ */
