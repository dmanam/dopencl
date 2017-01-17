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
 * @file cl_wwu_collective.h
 *
 * @date 2012-07-06
 * @author Philipp Kegel
 *
 * OpenCL API extension for collective operations
 */

#ifndef CL_WWU_COLLECTIVE_H_
#define CL_WWU_COLLECTIVE_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define cl_wwu_collective 1

/******************************************************************************/

/* cl_command_type */
#define CL_COMMAND_BROADCAST_BUFFER_WWU             0x1300
#define CL_COMMAND_REDUCE_BUFFER_WWU                0x1301

/* cl_kernel_arg_placeholder */
#define CL_KERNEL_ARG_1                             0x1
#define CL_KERNEL_ARG_2                             0x2

/******************************************************************************/

/* Collective operation APIs */

/**
 * @brief Broadcasts host memory to a set of buffers.
 * This operation is equivalent to a number of calls of clEnqueueWriteBuffer
 * where the same host memory is copied to multiple destination buffers.
 * A command queue has to be specified for each destination buffer in order to
 * determine the device where the buffer is migrated to.
 * Upload is performed in two steps: 1) upload host memory to compute node;
 * 2) upload data to buffers using clEnqueueWriteBuffer.
 *
 * @param[in]  command_queue_list  a list of command queues
 * @param[in]  num_buffers         number of destination buffers
 * @param[in]  buffer_list         the buffers to broadcast to (receive buffers)
 * @param[in]  offsets
 * @param[in]  cb                  the number of bytes to broadcast
 * @param[in]  ptr                 pointer to host memory that should be broadcasted
 * @param[in]  num_events_in_wait_list
 * @param[in]  event_wait_list
 * @param[out] event
 * @return error code
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffersWWU(
        cl_command_queue * /* command_queue_list */,
        cl_uint            /* num_buffers */,
        cl_mem *           /* buffer_list */,
        const size_t *     /* offsets */,
        size_t             /* cb */,
        const void *       /* ptr */,
        cl_uint            /* num_events_in_wait_list */,
        const cl_event *   /* event_wait_list */,
        cl_event *         /* event */) CL_EXT_SUFFIX__VERSION_1_1;

/**
 * @brief Broadcasts a buffer to a set of buffers.
 * This operation is equivalent to a number of calls of clEnqueueCopyBuffer
 * where the same source buffer is copied to multiple destination buffers.
 * A command queue has to be specified for each destination buffer in order to
 * determine the device where the buffer is migrated to. Actually, a 'copy
 * buffer' operation is issued to this command queue.
 * Data is only copied between compute nodes by means of the memory consistency
 * protocol.
 *
 * @param[in]  command_queue_list  a list of command queues
 * @param[in]  src_buffer          the buffer to broadcast (send buffer)
 * @param[in]  num_dst_buffer      number of destination buffers
 * @param[in]  dst_buffer_list     the buffers to broadcast to (receive buffers)
 * @param[in]  src_offset
 * @param[in]  dst_offset_list
 * @param[in]  cb                  the number of bytes to broadcast
 * @param[in]  num_events_in_wait_list
 * @param[in]  event_wait_list
 * @param[out] event
 * @return error code
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBroadcastBufferWWU(
        cl_command_queue * /* command_queue_list */,
        cl_mem             /* src_buffer */,
        cl_uint            /* num_dst_buffers */,
        cl_mem *           /* dst_buffer_list */,
        size_t             /* src_offset */,
        const size_t *     /* dst_offset_list */,
        size_t             /* cb */,
        cl_uint            /* num_events_in_wait_list */,
        const cl_event *   /* event_wait_list */,
        cl_event *         /* event */) CL_EXT_SUFFIX__VERSION_1_1;

/**
 * @brief Reduces a set of buffers into a single buffer
 *
 * @param[in]  command_queue        command queue for writing destination buffer
 * @param[in]  num_src_buffers      number of buffers to reduce
 * @param[in]  src_buffer_list      the buffers to reduce (send buffers)
 * @param[in]  dst_buffer           the buffers to reduce into (receive buffer)
 * @param[in]  kernel               a kernel for reducing two buffers
 *             This kernel must accept at least two arguments, but may also use
 *             more. The two mandatory arguments must be CL_KERNEL_ARG_1 and
 *             CL_KERNEL_ARG_2.
 * @param[in]  work_dim
 * @param[in]  global_work_offset
 * @param[in]  global_work_size
 * @param[in]  local_work_size
 * @param[in]  num_events_in_wait_list
 * @param[in]  event_wait_list
 * @param[out] event
 * @return error code
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReduceBufferWWU(
        cl_command_queue    /* command_queue */,
        cl_uint             /* num_src_buffers */,
        cl_mem *            /* src_buffer_list */,
        cl_mem              /* dst_buffer */,
//        const size_t *      /* src_offset_list */,
//        size_t              /* dst_offset */,
//        size_t              /* cb */,
        cl_kernel           /* kernel */,
        cl_uint             /* work_dim */,
        const size_t *      /* global_work_offset */,
        const size_t *      /* global_work_size */,
        const size_t *      /* local_work_size */,
//        cl_reduce_operator_WWU /* operator */,
        cl_uint             /* num_events_in_wait_list */,
        const cl_event *    /* event_wait_list */,
        cl_event *          /* event */) CL_EXT_SUFFIX__VERSION_1_1;

/*
 * Not all collectives known from MPI are reasonable in OpenCL, because memory
 * objects are shared by all devices of a context.
 *
 * Scattering a buffer is equivalent to dividing a buffer into sub-buffers,
 * while gathering buffers means addressing a buffer which had been 'scattered'
 * into sub-buffers. Moreover, as all devices share the sub-buffers' associated
 * buffer, 'gather' would actually mean 'all-gather'.
 * Hence, the following collectives are not required in OpenCL:
 *   * clEnqueueScatterBufferWWU
 *   * clEnqueueGatherBufferWWU
 *   * clEnqueueAllGatherBufferWWU
 *
 * Collectives in OpenCL will implicitly provide their result for all devices.
 * Hence, the following collectives are not required in OpenCL:
 *   * clEnqueueAllReduceBufferWWU (just use clEnqueueReduceBufferWWU; the
 *       reduced buffer is accessible by all devices.)
 *
 * Collectives that include one of the aforementioned redundant collectives,
 * also are not required. This holds for:
 *   * clEnqueueReduceScatterBufferWWU (just use clEnqueueReduceBufferWWU and
 *         define sub-buffers for the reduced buffer.)
 */

/******************************************************************************/

typedef CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueWriteBuffersWWU_fn)(
        cl_command_queue * /* command_queue_list */,
        cl_uint            /* num_buffers */,
        cl_mem *           /* buffer_list */,
        const size_t *     /* offsets */,
        size_t             /* cb */,
        const void *       /* ptr */,
        cl_uint            /* num_events_in_wait_list */,
        const cl_event *   /* event_wait_list */,
        cl_event *         /* event */);

typedef CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueBroadcastBufferWWU_fn)(
        cl_command_queue *  /* command_queue_list */,
        cl_mem              /* src_buffer */,
        cl_uint             /* num_dst_buffers */,
        cl_mem *            /* dst_buffer_list */,
        size_t              /* src_offset */,
        const size_t *      /* dst_offset_list */,
        size_t              /* cb */,
        cl_uint             /* num_events_in_wait_list */,
        const cl_event *    /* event_wait_list */,
        cl_event *          /* event */);

typedef CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueReduceBufferWWU_fn)(
        cl_command_queue    /* command_queue */,
        cl_uint             /* num_src_buffers */,
        cl_mem *            /* src_buffer_list */,
        cl_mem              /* dst_buffer */,
//        const size_t *      /* src_offset_list */,
//        size_t              /* dst_offset */,
//        size_t              /* cb */,
        cl_kernel           /* kernel */,
        cl_uint             /* work_dim */,
        const size_t *      /* global_work_offset */,
        const size_t *      /* global_work_size */,
        const size_t *      /* local_work_size */,
//        cl_reduce_operator_WWU  /* oper */,
        cl_uint             /* num_events_in_wait_list */,
        const cl_event *    /* event_wait_list */,
        cl_event *          /* event */);

#ifdef __cplusplus
}
#endif

#endif /* CL_WWU_COLLECTIVE_H_ */
