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
 * \file CommandQueue.h
 *
 * \date 2012-08-05
 * \author Philipp Kegel
 */

#ifndef COMMANDQUEUE_H_
#define COMMANDQUEUE_H_

#include <dcl/CommandQueue.h>
#include <dcl/DCLTypes.h>
#include <dcl/Event.h>
#include <dcl/Kernel.h>
#include <dcl/Memory.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <cstddef>
#include <memory>
#include <vector>

namespace dcld {

class Buffer;
class Context;
class Device;

/* ****************************************************************************/

/*!
 * \brief A decorator for a native command queue.
 *
 * This wrapper is required to store the context decorator that should be
 * associated with events that are created when enqueuing commands to this
 * command queue.
 */
class CommandQueue: public dcl::CommandQueue {
public:
    CommandQueue(
            const std::shared_ptr<Context>& context,
            Device *                        device,
            cl_command_queue_properties     properties);
    virtual ~CommandQueue();

    /*!
     * \brief Returns the native command queue.
     */
    operator cl::CommandQueue() const;

    void flush();
    void finish();

    void enqueueCopyBuffer(
            const std::shared_ptr<dcl::Buffer>&             srcBuffer,
            const std::shared_ptr<dcl::Buffer>&             dstBuffer,
            size_t                                          srcOffset,
            size_t                                          dstOffset,
            size_t                                          size,
            const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
            dcl::object_id                                  commandId,
            std::shared_ptr<dcl::Event> *                   event);

    void enqueueReadBuffer(
            const std::shared_ptr<dcl::Buffer>&             buffer,
            bool                                            blockingRead,
            size_t                                          offset,
            size_t                                          size,
            const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
            dcl::object_id                                  commandId,
            std::shared_ptr<dcl::Event> *                   event);

    void enqueueWriteBuffer(
            const std::shared_ptr<dcl::Buffer>&             buffer,
            bool                                            blockingWrite,
            size_t                                          offset,
            size_t                                          size,
            const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
            dcl::object_id                                  commandId,
            std::shared_ptr<dcl::Event> *                   event);

    void enqueueMapBuffer(
            const std::shared_ptr<dcl::Buffer>&             buffer,
            bool                                            blockingMap,
            cl_map_flags                                    mapFlags,
            size_t                                          offset,
            size_t                                          size,
            const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
            dcl::object_id                                  commandId,
            std::shared_ptr<dcl::Event> *                   event);

    void enqueueUnmapBuffer(
            const std::shared_ptr<dcl::Buffer>&             buffer,
            cl_map_flags                                    map_flags,
            size_t                                          offset,
            size_t                                          size,
            const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
            dcl::object_id                                  commandId,
            std::shared_ptr<dcl::Event> *                   event);

    void enqueueNDRangeKernel(
            const std::shared_ptr<dcl::Kernel>&             kernel,
            const std::vector<size_t>&                      offset,
            const std::vector<size_t>&                      global,
            const std::vector<size_t>&                      local,
            const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
            dcl::object_id                                  commandId,
            std::shared_ptr<dcl::Event> *                   event);

    void enqueueMarker(
            const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
            dcl::object_id                                  commandId,
            std::shared_ptr<dcl::Event> *                   event);

#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
    void enqueueWaitForEvents(
            const std::vector<std::shared_ptr<dcl::Event>>& eventList);
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

    void enqueueBarrier(
            const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
            dcl::object_id                                  commandId,
            std::shared_ptr<dcl::Event> *                   event);

    const std::shared_ptr<Context>& context() const;

private:
    /* Command queues must be non-copyable */
    CommandQueue(
            const CommandQueue& rhs) = delete;
    CommandQueue& operator=(
            const CommandQueue& rhs) = delete;

    /*!
     * \brief Synchronizes this command queue with the events in the event wait list.
     *
     * If an eventWaitList is found linked to an enqueued command,
     * the wait list is searched for events and event listeners.
     * Event listener indicate a remote event and could lead into
     * an acquire-operation, which is part of the consistency protocol
     *
     * \param[in]  eventWaitList		an event wait list
     * \param[out] nativeEventWaitList  a list of corresponding native events
     */
    void synchronize(
            const std::vector<std::shared_ptr<dcl::Event>>& eventWaitList,
            VECTOR_CLASS<cl::Event>&                        nativeEventWaitList);

    void enqueueReadBuffer(
            const std::shared_ptr<Buffer>&  buffer,
            bool                            blocking,
            size_t                          offset,
            size_t                          size,
            const VECTOR_CLASS<cl::Event>&  nativeEventWaitList,
            dcl::object_id                  commandId,
            cl::Event&                      mapData,
            cl::Event&                      unmapData);

    void enqueueWriteBuffer(
            const std::shared_ptr<Buffer>&  buffer,
            bool                            blocking,
            size_t                          offset,
            size_t                          size,
            const VECTOR_CLASS<cl::Event>&  nativeEventWaitList,
            dcl::object_id                  commandId,
            cl::Event&                      mapData,
            cl::Event&                      unmapData);

    void enqueuePhonyMarker(
            bool                            blocking,
            const VECTOR_CLASS<cl::Event>&  nativeEventWaitList,
            dcl::object_id                  commandId,
            cl::Event&                      marker);

    cl::CommandQueue _commandQueue; //!< Native command queue

    std::shared_ptr<Context> _context; //!< Associated context
};

} /* namespace dcld */

#endif /* COMMANDQUEUE_H_ */
