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
 * \file    CommandQueue.h
 *
 * \date    2012-08-05
 * \author  Philipp Kegel
 *
 * dOpenCL command queue API
 */

#ifndef DCL_COMMANDQUEUE_H_
#define DCL_COMMANDQUEUE_H_

#include "DCLTypes.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <memory>
#include <vector>

namespace dcl {

class Buffer;
class ComputeNode;
class Event;
class Kernel;
class Memory;

/* ****************************************************************************/

/*!
 * \brief Remote interface of a command queue
 */
class CommandQueue {
public:
    virtual ~CommandQueue() { }

    virtual void flush() = 0;

    /*!
     * \brief Blocks until all previously queued OpenCL commands in this command queue are issued to the associated device and have completed.
     *
     * This method is a synchronization point.
     */
    virtual void finish() = 0;

    virtual void enqueueCopyBuffer(
            const std::shared_ptr<Buffer>&              src,
            const std::shared_ptr<Buffer>&              dst,
            size_t                                      srcOffset,
            size_t                                      dstOffset,
            size_t                                      size,
            const std::vector<std::shared_ptr<Event>> * eventWaitList,
            object_id                                   commandId,
            std::shared_ptr<Event> *                    event) = 0;

    virtual void enqueueReadBuffer(
            const std::shared_ptr<Buffer>&              buffer,
            bool                                        blockingRead,
            size_t                                      offset,
            size_t                                      size,
            const std::vector<std::shared_ptr<Event>> * eventWaitList,
            object_id                                   commandId,
            std::shared_ptr<Event> *                    event) = 0;

    virtual void enqueueWriteBuffer(
            const std::shared_ptr<Buffer>&              buffer,
            bool                                        blockingWrite,
            size_t                                      offset,
            size_t                                      size,
            const std::vector<std::shared_ptr<Event>> * eventWaitList,
            object_id                                   commandId,
            std::shared_ptr<Event> *                    event) = 0;

    virtual void enqueueMapBuffer(
            const std::shared_ptr<Buffer>&              buffer,
            bool                                        blockingMap,
            cl_map_flags                                mapFlags,
            size_t                                      offset,
            size_t                                      size,
            const std::vector<std::shared_ptr<Event>> * eventWaitList,
            object_id                                   commandId,
            std::shared_ptr<Event> *                    event) = 0;

    virtual void enqueueUnmapBuffer(
            const std::shared_ptr<Buffer>&              buffer,
            cl_map_flags                                mapFlags,
            size_t                                      offset,
            size_t                                      size,
            const std::vector<std::shared_ptr<Event>> * eventWaitList,
            object_id                                   commandId,
            std::shared_ptr<Event> *                    event) = 0;

    /*!
     * \brief Enqueues a kernel to this
     *
     * \param[in]  kernel        kernel to enqueue
     * \param[in]  offset        global work offset
     * \param[in]  global        global work size
     * \param[in]  local         local work size
     * \param[in]  eventWaitList event to wait for before executing the enqueued kernel
     * \param[in]  commandId     command ID
     * \param[out] event         event associated with this command, or \c NULL
     *
     * global work offset, global work size, and local work size must all have
     * the same number of entries (i.e., the same dimension)
     */
    virtual void enqueueNDRangeKernel(
            const std::shared_ptr<Kernel>&              kernel,
            const std::vector<size_t>&                  offset,
            const std::vector<size_t>&                  global,
            const std::vector<size_t>&                  local,
            const std::vector<std::shared_ptr<Event>> * eventWaitList,
            object_id                                   commandId,
            std::shared_ptr<Event> *                    event) = 0;

    /*!
     * \brief Enqueues a marker to this command queue
     *
     * \param[in]  eventWaitList    a list of events the marker waits for
     * \param[in]  commandId        an ID that remote event listeners associate with this command
     * \param[out] event            an event associated with the enqueued marker
     */
    virtual void enqueueMarker(
            const std::vector<std::shared_ptr<Event>> * eventWaitList,
            object_id                                   commandId,
            std::shared_ptr<Event> *                    event) = 0;

#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
    /*!
     * \brief Enqueues a wait for a specific event or a list of events to complete before any future commands queued in the command-queue are executed.
     */
    virtual void enqueueWaitForEvents(
            const std::vector<std::shared_ptr<Event>>&  eventList) = 0;
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

    /*!
     * \brief A synchronization point that enqueues a barrier operation.
     *
     * This method is a synchronization point. Note, however that according to
     * Section 3.4.3 of the OpenCL specification, a barrier can only be used to
     * synchronize between commands in a single command-queue.
     *
     * \param[in]  eventWaitList
     * \param[in]  commandId
     * \param[out] event
     */
    virtual void enqueueBarrier(
            const std::vector<std::shared_ptr<Event>> * eventWaitList,
            object_id                                   commandId,
            std::shared_ptr<Event> *                    event) = 0;
};

} /* namespace dcl */

#endif /* DCL_COMMANDQUEUE_H_ */
