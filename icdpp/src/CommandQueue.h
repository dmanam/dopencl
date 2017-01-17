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
 * @file CommandQueue.h
 *
 * @date 2011-05-31
 * @author Karsten Jeschkies
 */

#ifndef CL_COMMANDQUEUE_H_
#define CL_COMMANDQUEUE_H_

#include "Context.h"
#include "Device.h"
#include "Retainable.h"

#include "dclicd/command/Command.h"

#include <dcl/CommandQueueListener.h>
#include <dcl/ComputeNode.h>
#include <dcl/DataTransfer.h>
#include <dcl/DCLTypes.h>
#include <dcl/Remote.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace dclicd {

/* forward declarations */
class Buffer;
class ReadMemoryEvent;

} /* namespace dclicd */

/******************************************************************************/

class _cl_command_queue:
public _cl_retainable,
public dcl::Remote,
public dcl::CommandQueueListener {
public:
    static void enqueueBroadcast(
            std::vector<cl_command_queue>   commandQueueList,
            dclicd::Buffer *                src,
            std::vector<dclicd::Buffer *>   dsts,
            size_t                          srcOffset,
            const std::vector<size_t>       dstOffsets,
            size_t                          cb,
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);

    void enqueueReduce(
            std::vector<dclicd::Buffer *>   srcs,
            dclicd::Buffer *                dst,
//            const std::vector<size_t>       srcOffsets,
//            size_t                          dstOffset,
//            size_t                          cb,
            cl_kernel                       kernel,
            const std::vector<size_t>&      global_work_offset,
            const std::vector<size_t>&      global_work_size,
            const std::vector<size_t>&      local_work_size,
//            cl_reduce_operator_WWU          oper,
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);


    _cl_command_queue(
            cl_context                  context,
            cl_device_id                device,
            cl_command_queue_properties properties);
    virtual ~_cl_command_queue();

    dcl::ComputeNode& computeNode() const;

    void getInfo(
            cl_command_queue_info   param_name,
            size_t                  param_value_size,
            void *                  param_value,
            size_t *                param_value_size_ret) const;

    void finish();
    void flush();

#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
    void enqueueWaitForEvents(
            const std::vector<cl_event>& eventList);
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

    /**
     * @brief Enqueues a marker command which waits for either a list of events
     * to complete, or all previously enqueued commands to complete.
     *
     * This methods implements the OpenCL 1.2 clEnqueueMarkerWithWaitList API.
     * It is also used to implement the deprecated clEnqueueMarker API.
     *
     * @param[in]  event_wait_list
     * @param[in]  event
     */
    void enqueueMarker(
            const std::vector<cl_event>&    event_wait_list,
            cl_event *                      event);

    /**
     * @brief A synchronization point that enqueues a barrier operation.
     *
     * This methods implements the OpenCL 1.2 clEnqueueBarrierWithWaitList API.
     * It is also used to implement the deprecated clEnqueueBarrier API.
     *
     * @param[in]  event_wait_list
     * @param[in]  event
     */
    void enqueueBarrier(
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);

    void enqueueWrite(
            dclicd::Buffer *                buffer,
            cl_bool                         blocking_write,
            size_t                          offset,
            size_t                          cb,
            const void *                    ptr,
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);

    void enqueueRead(
            dclicd::Buffer *                buffer,
            cl_bool                         blocking_read,
            size_t                          offset,
            size_t                          cb,
            void *                          ptr,
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);

    void enqueueCopy(
            dclicd::Buffer *                src,
            dclicd::Buffer *                dst,
            size_t                          srcOffset,
            size_t                          dstOffset,
            size_t                          cb,
            const std::vector<cl_event>&    eventWaitList,
            cl_event *event = nullptr);

    void * enqueueMap(
            dclicd::Buffer *                buffer,
            cl_bool                         blocking_map,
            cl_map_flags                    map_flags,
            size_t                          offset,
            size_t                          cb,
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);

    void enqueueUnmap(
            cl_mem                          memobj,
            void *                          mapped_ptr,
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);

#if defined(CL_VERSION_1_2)
    void enqueueMigrateMemObjects(
            const std::vector<cl_mem>&      mem_objects,
            cl_mem_migration_flags          flags,
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);
#endif // #if defined(CL_VERSION_1_2)

    void enqueueNDRangeKernel(
            cl_kernel                       kernel,
            const std::vector<size_t>&      global_work_offset,
            const std::vector<size_t>&      global_work_size,
            const std::vector<size_t>&      local_work_size,
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);

    void enqueueTask(
            cl_kernel                       kernel,
            const std::vector<cl_event>&    event_wait_list,
            cl_event *event = nullptr);

    /*
     * Command queue listener APIs
     */

    void onFinish();

protected:
    void destroy();

private:
    void createEventIdWaitList(
            const std::vector<cl_event>&    event_wait_list,
            std::vector<dcl::object_id>&    eventIds);

    /**
     * @brief Enqueues a command.
     *
     * Add a command to this command queue's command list.
     * Besides, completed command are removed from the command list.
     *
     * @param[in]  command  the command to enqueue
     */
    void enqueueCommand(
            const std::shared_ptr<dclicd::command::Command>& command);

    /**
     * @brief Finishes this command queue locally.
     *
     * Unlike finish, which ensures that *all* commands queued to this command
     * queue have finished, this method only ensures that all commands managed
     * by this command queue, i.e., on the host, have finished.
     *
     * This is a blocking operation.
     */
    void finishLocally();

    cl_context _context;
    cl_device_id _device;
    cl_command_queue_properties _properties;
    /**
     * @brief A list of enqueued commands
     *
     * This list stored enqueued command which are listening to remote command.
     * Completed events are removed from this list when a new command is
     * enqueued or when the command queue is finished.
     *
     * This list ensures that command are not deleted before they have been
     * finished *and* have deleted their associated event.
     *
     * @see enqueueCommand
     * @see command::Command::onExecutionStatusChanged
     */
    std::vector<std::shared_ptr<dclicd::command::Command>> _commands;
    std::mutex _commandsMutex;
};

#endif /* CL_COMMANDQUEUE_H_ */
