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
 * @file cl_wwu_collective.cpp
 *
 * @date 2012-07-06
 * @author Philipp Kegel
 *
 * @brief Implementation of the OpenCL API extension specified for collective operations.
 *
 * Function calls of the C API are redirected to corresponding method of a C++
 * implementation. Functions in this file only perform type conversions and
 * related operations, e.g. validating list parameters that are converted into
 * vectors. The C++ methods validate parameters.
 */

#include "CommandQueue.h"
#include "ComputeNode.h"
#include "Context.h"
#include "Kernel.h"
#include "Memory.h"

#include "dclicd/Buffer.h"
#include "dclicd/Error.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_collective.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_collective.h>
#endif

#include <cstddef>
#include <stdexcept>
#include <vector>


/* Collective operation APIs */

cl_int clEnqueueBroadcastBufferWWU(cl_command_queue *command_queue_list,
		cl_mem src_buffer, cl_uint num_dst_buffers, cl_mem *dst_buffer_list,
		size_t src_offset, const size_t *dst_offset_list, size_t cb,
		cl_uint num_events_in_wait_list, const cl_event * event_wait_list,
		cl_event *event) {
    std::vector<dclicd::Buffer *> dsts;
    std::vector<size_t> dstOffsets;

    if (!command_queue_list) return CL_INVALID_VALUE;
	if ((num_dst_buffers == 0) || !dst_buffer_list) {
		return CL_INVALID_VALUE;
	}
	if ((num_events_in_wait_list > 0 && !event_wait_list)
			|| (num_events_in_wait_list == 0 && event_wait_list)) {
		return CL_INVALID_VALUE;
	}

    /* convert destination buffer list */
    dsts.reserve(num_dst_buffers);
    for (cl_mem *i = dst_buffer_list; i != dst_buffer_list + num_dst_buffers; ++i) {
        dsts.push_back(dynamic_cast<dclicd::Buffer *>(*i));
    }

    if (dst_offset_list) {
        dstOffsets.assign(dst_offset_list, dst_offset_list + num_dst_buffers);
    }

	try {
		_cl_command_queue::enqueueBroadcast(
		        std::vector<cl_command_queue>(command_queue_list,
		                command_queue_list + num_dst_buffers),
				dynamic_cast<dclicd::Buffer *>(src_buffer), dsts,
				src_offset, dstOffsets, cb,
				std::vector<cl_event>(event_wait_list, event_wait_list
						+ num_events_in_wait_list), event);
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}

cl_int clEnqueueReduceBufferWWU(cl_command_queue command_queue,
		cl_uint num_src_buffers, cl_mem *src_buffer_list, cl_mem dst_buffer,
//		const size_t *src_offset_list, size_t dst_offset, size_t cb,
		cl_kernel kernel, cl_uint work_dim, const size_t * global_work_offset,
		const size_t * global_work_size, const size_t * local_work_size,
//      cl_reduce_operator_WWU oper,
		cl_uint num_events_in_wait_list, const cl_event * event_wait_list,
		cl_event *event) {
    std::vector<dclicd::Buffer *> srcs;
    std::vector<size_t> offset;
    std::vector<size_t> global;
    std::vector<size_t> local;

    if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if ((num_src_buffers == 0) || !src_buffer_list) {
		return CL_INVALID_VALUE;
	}
	if (work_dim < 1 || work_dim > 3) {
		return CL_INVALID_WORK_DIMENSION;
	}
	if (!global_work_size) return CL_INVALID_GLOBAL_WORK_SIZE;
	if ((num_events_in_wait_list > 0 && !event_wait_list)
			|| (num_events_in_wait_list == 0 && event_wait_list)) {
		return CL_INVALID_VALUE;
	}

    /* convert source buffer list */
    srcs.reserve(num_src_buffers);
    for (cl_mem *i = src_buffer_list; i != src_buffer_list + num_src_buffers; ++i) {
        srcs.push_back(dynamic_cast<dclicd::Buffer *>(*i));
    }

    /* Convert global work offset and local work size */
    if (global_work_offset) {
        offset.assign(global_work_offset, global_work_offset + work_dim);
    }
    global.assign(global_work_size, global_work_size + work_dim);
    if (local_work_size) {
        local.assign(local_work_size, local_work_size + work_dim);
    }

	try {
	    command_queue->enqueueReduce(
				srcs, dynamic_cast<dclicd::Buffer *>(dst_buffer),
//				std::vector<size_t>(src_offset_list, src_offset_list + num_src_buffers),
//				dst_offset,	cb,
				kernel, offset, global, local,
//				oper,
				std::vector<cl_event>(event_wait_list, event_wait_list
						+ num_events_in_wait_list), event);
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}
