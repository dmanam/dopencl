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
 * \file CommandQueue.cpp
 *
 * \date 2012-08-12
 * \author Philipp Kegel
 */

#include "CommandQueue.h"

#include "Context.h"
#include "Device.h"
#include "Event.h"
#include "Kernel.h"
#include "Memory.h"

#include "command/Command.h"
#include "command/CopyDataCommand.h"
#include "command/SetCompleteCommand.h"

#include <dcl/DCLTypes.h>
#include <dcl/Event.h>
#include <dcl/Kernel.h>
#include <dcl/Memory.h>

#include <dcl/util/Logger.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ostream>
#include <vector>

namespace {

/*!
 * \brief Callback used to execute enqueued commands.
 */
void executeCommand(cl_event event, cl_int execution_status, void *user_data) {
    std::unique_ptr<dcld::command::Command> command(static_cast<dcld::command::Command *>(user_data));
    assert(command != nullptr);
    command->execute(execution_status);
}

#ifdef PROFILE
/*!
 * \brief Callback used to log event profiling info.
 */
void logEventProfilingInfo(cl_event object_, cl_int execution_status, void *user_data) {
    std::unique_ptr<std::string> str(static_cast<std::string *>(user_data));
    cl::Event event;
    cl_int err;

    /* assign event to C++ wrapper */
    event() = object_;
    err = ::clRetainEvent(object_);
    assert(err == CL_SUCCESS);

    /* query profiling info */
    try {
        cl_ulong queued, submit, start, end;

        event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &queued);
        event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &submit);
        event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
        event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);

        dcl::util::Logger << dcl::util::Debug
                << "Command completed (" << (str ? *str : "") << ")\n"
                << "\tqueued =" << queued << '\n'
                << "\tsubmit =" << submit << '\n'
                << "\tstart  =" << start << '\n'
                << "\tend    =" << end << '\n'
                << "\tdurance=" << (static_cast<double>(end - start) / 1000000000.0) << " sec"
                << std::endl;
    } catch (const cl::Error& err) {
        dcl::util::Logger << dcl::util::Error
                << "OpenCL error (ID=" << err.err() << "): " << err.what()
                << std::endl;
    }
}
#endif

cl::NDRange createNDRange(const std::vector<size_t>& vector) {
    switch (vector.size()) {
    case 0:
        return cl::NullRange;
    case 1:
        return cl::NDRange(vector[0]);
    case 2:
        return cl::NDRange(vector[0], vector[1]);
    case 3:
        return cl::NDRange(vector[0], vector[1], vector[2]);
    default:
        throw cl::Error(CL_INVALID_WORK_DIMENSION);
    }
}

} /* unnamed namespace */

/* ****************************************************************************/

namespace dcld {

CommandQueue::CommandQueue(
        const std::shared_ptr<Context>& context,
        Device *device,
        cl_command_queue_properties properties) :
        _context(context) {
    if (!context) throw cl::Error(CL_INVALID_CONTEXT);
    if (!device) throw cl::Error(CL_INVALID_DEVICE);
    _commandQueue = cl::CommandQueue(*context, *device, properties);
}

CommandQueue::~CommandQueue() { }

/*!
 * \brief Returns the native command queue.
 */
CommandQueue::operator cl::CommandQueue() const {
    return _commandQueue;
}

/*
 * INFO: Synchronization
 *
 * Synchronization always is based on events in a wait list which act as
 * synchronization points (the only exception is the enqueueBarrier command).
 * For events associated with a command on the same compute node and for user
 * events, this requires no special measures; the events are just passed to the
 * implementation. But for events which are associated with a command on another
 * compute node (instances of dcld::EventListener), a synchronization across
 * compute nodes is required, if a memory object is associated with such an
 * event.
 * For each associated memory object, a data transfer from the memory object's
 * owner (the compute node that executes the command associated with the event)
 * is enqueued to this command queue. A native event is created for each data
 * transfer.
 * While the associated memory objects are removed from the event, the native
 * events associated with the data transfers are added to it. Thus,
 * synchronization is only enqueued once to the command queue that first uses
 * this event for synchronization.
 * The native events that have been added (associated) with the event during
 * synchronization are added to the native event wait list, such that the OpenCL
 * implementation synchronizes the memory objects. This is particularly
 * important if the event is used by another command queue on the same compute
 * node: while the data has already been transferred to the compute node, it is
 * only visible to the device that performed the synchronization. Thus, all
 * other devices have to synchronize in order to obtain the data.
 */
void CommandQueue::synchronize(
        const std::vector<std::shared_ptr<dcl::Event>>& eventWaitList,
        VECTOR_CLASS<cl::Event>& nativeEventWaitList) {
    bool synchronizationPending = false;

    nativeEventWaitList.clear();

    if (eventWaitList.empty()) return;

    dcl::util::Logger << dcl::util::Debug
            << "Synchronizing event wait list with " << eventWaitList.size()
            << " event(s)" << std::endl;

    for (auto event : eventWaitList) {
        /* TODO Create Event::synchronize method
         * Rather than checking if an event is of type RemoteEvent before
         * calling synchronize, make synchronize a member of all event classes
         * that returns a list native events for local events and performs
         * synchronization for remote events */
        auto remoteEvent = std::dynamic_pointer_cast<RemoteEvent>(event);
        if (remoteEvent) { // event is a remote event
            VECTOR_CLASS<cl::Event> synchronizeEvents;
            remoteEvent->synchronize(_commandQueue, synchronizeEvents);
            /* FIXME Only synchronize memory objects once if associated with multiple events in wait list
             * Different events may be associated with the same memory object
             * However, a memory object must only be synchronized once.
             * Synchronizing with multiple events associated with the same memory
             * object may be considered undefined behavior. */
            nativeEventWaitList.insert(std::end(nativeEventWaitList),
                    std::begin(synchronizeEvents), std::end(synchronizeEvents));

            synchronizationPending = true;
        } else { // event is a local event
            auto eventImpl = std::dynamic_pointer_cast<Event>(event);
            if (!eventImpl) throw cl::Error(CL_INVALID_EVENT_WAIT_LIST);
            nativeEventWaitList.push_back(*eventImpl);
        }
    }

    if (synchronizationPending) {
        /* Flush the command queue to ensure instant execution of the acquire
         * operation */
        _commandQueue.flush();
    }
}

void CommandQueue::enqueueReadBuffer(
        const std::shared_ptr<Buffer>& buffer,
        bool blocking,
        size_t offset,
        size_t size,
        const VECTOR_CLASS<cl::Event>& nativeEventWaitList,
        dcl::object_id commandId,
        cl::Event& mapData, cl::Event& unmapData) {
    cl::UserEvent copyData(*_context);

    // enqueue map buffer (implicit download)
    void *ptr = _commandQueue.enqueueMapBuffer(
            *buffer,
            CL_FALSE,    // non-blocking map
            CL_MAP_READ, // map for reading
            offset, size,
            (nativeEventWaitList.empty() ? nullptr : &nativeEventWaitList),
            &mapData);
#ifdef PROFILE
    mapData.setCallback(CL_COMPLETE, &logEventProfilingInfo, new std::string("map buffer for reading"));
#endif
    // enqueue unmap buffer
    VECTOR_CLASS<cl::Event> unmapEventWaitList(1, copyData);
    _commandQueue.enqueueUnmapMemObject(
            *buffer,
            ptr,
            &unmapEventWaitList, &unmapData);
#ifdef FORCE_FLUSH
    _commandQueue.flush();
#else
    if (blocking) {
        _commandQueue.flush();
    }
#endif

    try {
        // schedule data transfer
        mapData.setCallback(CL_COMPLETE, &executeCommand,
                new command::CopyDataCommand<command::DeviceToHost>(
                        _context->host(), commandId, size, ptr, copyData));
        /* The read buffer command is finished on the host, such that no
         * 'command complete' message must be sent by the compute node. */
    } catch (const std::bad_alloc&) {
        throw cl::Error(CL_OUT_OF_RESOURCES);
    }
}

void CommandQueue::enqueueWriteBuffer(
        const std::shared_ptr<Buffer>& buffer,
        bool blocking,
        size_t offset,
        size_t size,
        const VECTOR_CLASS<cl::Event>& nativeEventWaitList,
        dcl::object_id commandId,
        cl::Event& mapData, cl::Event& unmapData) {
    cl::UserEvent copyData(*_context);

    // enqueue map buffer
    /* !!! WARNING !!!
     * NVIDIA only: the reference count of events in wait list is *not*
     * increased by clEnqueueMapBuffer. This may be a bug, if the event is not
     * retained by other means than its reference count.
     * !!!!!!!!!!!!!!! */
    void *ptr = _commandQueue.enqueueMapBuffer(
            *buffer,
            CL_FALSE,     // non-blocking map
            CL_MAP_WRITE, // map for writing
            offset, size,
            (nativeEventWaitList.empty() ? nullptr : &nativeEventWaitList),
            &mapData);
    // enqueue unmap buffer (implicit upload)
    VECTOR_CLASS<cl::Event> unmapEventWaitList(1, copyData);
    /* !!! WARNING !!!
     * NVIDIA only: the reference count of copyData is *not* increased by
     * clEnqueueUnmapMemObject. This may be a bug, if the event is not retained
     * by other means than its reference count.
     * !!!!!!!!!!!!!!! */
//    std::cout << "!!! before unmap copyData refCount=" << copyData.getInfo<CL_EVENT_REFERENCE_COUNT>() << std::endl;
//    ::clRetainEvent(copyData()); // tentative fix for NVIDIA
    _commandQueue.enqueueUnmapMemObject(
            *buffer,
            ptr,
            &unmapEventWaitList, &unmapData);
//    std::cout << "!!! after unmap copyData refCount=" << copyData.getInfo<CL_EVENT_REFERENCE_COUNT>() << std::endl;
#ifdef PROFILE
    unmapData.setCallback(CL_COMPLETE, &logEventProfilingInfo, new std::string("unmap buffer after writing"));
#endif
#ifdef FORCE_FLUSH
    _commandQueue.flush();
#else
    if (blocking) {
        _commandQueue.flush();
    }
#endif

    try {
        // schedule data transfer
        mapData.setCallback(CL_COMPLETE, &executeCommand,
                new command::CopyDataCommand<command::HostToDevice>(
                        _context->host(), commandId, size, ptr, copyData));
        // schedule completion message for host
        /* A 'command complete' message is sent to the host.
         * Note that this message must also be sent, if no event is associated
         * with this command, such that a blocking write succeeds. */
        unmapData.setCallback(CL_COMPLETE, &executeCommand,
                new command::SetCompleteCommand(_context->host(), commandId, cl::UserEvent(*_context)));
    } catch (const std::bad_alloc&) {
        throw cl::Error(CL_OUT_OF_RESOURCES);
    }
}

void CommandQueue::enqueuePhonyMarker(
        bool blocking,
        const VECTOR_CLASS<cl::Event>& nativeEventWaitList,
        dcl::object_id commandId, cl::Event& marker) {
#if defined(CL_VERSION_1_2)
    _commandQueue.enqueueMarkerWithWaitList(&nativeEventWaitList, &marker);
#else // #if defined(CL_VERSION_1_2)
    if (nativeEventWaitList.empty()) {
        _commandQueue.enqueueMarker(&marker);
    } else {
        /* TODO Use pre-OpenCL 1.2 APIs to implement CommandQueue::enqueueMarker with wait list
         * clEnqueueMarker (deprecated) does not support an event list. */
        assert(!"clEnqueueMarker does not support event wait list");
    }
#endif // #if defined(CL_VERSION_1_2)
#ifdef FORCE_FLUSH
    _commandQueue.flush();
#else
    if (blocking) {
        _commandQueue.flush();
    }
#endif

    try {
        // schedule completion message for host
        /* A 'command complete' message is sent to the host.
         * Note that this message must also be sent, if no event is associated
         * with this command, such that a blocking operation succeeds. */
        marker.setCallback(CL_COMPLETE, &executeCommand,
                new command::SetCompleteCommand(_context->host(), commandId, cl::UserEvent(*_context)));
    } catch (const std::bad_alloc&) {
        throw cl::Error(CL_OUT_OF_RESOURCES);
    }
}

void CommandQueue::flush() {
    _commandQueue.flush();
}

/* clFinish is a synchronization point */
void CommandQueue::finish() {
    _commandQueue.finish();

    /* TODO Implement synchronization for clFinish
     * It is not clear from the specification what this means; maybe it acts as
     * an active release, i.e., the memory objects modified by this command queue
     * are replicated to all other compute nodes. */
}

void CommandQueue::enqueueCopyBuffer(
        const std::shared_ptr<dcl::Buffer>& src,
        const std::shared_ptr<dcl::Buffer>& dst,
        size_t srcOffset,
        size_t dstOffset,
        size_t size,
        const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
        dcl::object_id commandId,
        std::shared_ptr<dcl::Event> *event) {
    auto srcImpl = std::dynamic_pointer_cast<Buffer>(src);
    auto dstImpl = std::dynamic_pointer_cast<Buffer>(dst);
    VECTOR_CLASS<cl::Event> nativeEventWaitList;
    cl::Event copyBuffer;

    if (!srcImpl) throw cl::Error(CL_INVALID_MEM_OBJECT);
    if (!dstImpl) throw cl::Error(CL_INVALID_MEM_OBJECT);

    /* Obtain wait list of native events */
    if (eventWaitList) {
        synchronize(*eventWaitList, nativeEventWaitList);
    }

    /* Enqueue copy buffer
     * Only create event if requested by caller */
    _commandQueue.enqueueCopyBuffer(
            *srcImpl, *dstImpl,
            srcOffset, dstOffset, size,
            &nativeEventWaitList, event ? &copyBuffer : nullptr);
#ifdef FORCE_FLUSH
    _commandQueue.flush();
#endif

    if (event) { // an event should be associated with this command
        try {
            *event = std::make_shared<SimpleEvent>(commandId, _context, dstImpl, copyBuffer);
        } catch (const std::bad_alloc&) {
            throw cl::Error(CL_OUT_OF_RESOURCES);
        }
    }
}

void CommandQueue::enqueueReadBuffer(
        const std::shared_ptr<dcl::Buffer>& buffer,
        bool blockingRead,
        size_t offset,
        size_t size,
        const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
        dcl::object_id commandId,
        std::shared_ptr<dcl::Event> *event) {
    auto bufferImpl = std::dynamic_pointer_cast<Buffer>(buffer);
    VECTOR_CLASS<cl::Event> nativeEventWaitList;
    cl::Event mapData, unmapData;
    cl::UserEvent copyData(*_context);

    if (!bufferImpl) throw cl::Error(CL_INVALID_MEM_OBJECT);

    /* Obtain wait list of native events */
    if (eventWaitList) {
        synchronize(*eventWaitList, nativeEventWaitList);
    }

    /* Enqueue map buffer (implicit download) */
    void *ptr = _commandQueue.enqueueMapBuffer(
            *bufferImpl,
            CL_FALSE,    // non-blocking map
            CL_MAP_READ, // map for reading
            offset, size,
            &nativeEventWaitList, &mapData);
    /* Enqueue unmap buffer
     * Only create a native event for unmapping, if an event should be
     * associated with this read buffer command */
    nativeEventWaitList.assign(1, copyData);
    _commandQueue.enqueueUnmapMemObject(
            *bufferImpl,
            ptr,
            &nativeEventWaitList, event ? &unmapData : nullptr);
#ifdef FORCE_FLUSH
    _commandQueue.flush();
#else
    if (blockingRead) {
        _commandQueue.flush();
    }
#endif

    try {
        /* Schedule data sending
         * A 'command submitted' message will be sent to the host in order to
         * start data receipt. */
        mapData.setCallback(CL_COMPLETE, &executeCommand,
                new command::CopyDataCommand<command::DeviceToHost>(
                        _context->host(), commandId, size, ptr, copyData));
        /* The read buffer command is finished on the host such that no 'command
         * complete' message must be sent by the compute node. */

        if (event) { // an event should be associated with this command
            /* WARNING: No callback must be registered for any native event of
             * the ReadMemoryEvent object, that access the object. As the host
             * finishes the read buffer command, the application may delete the
             * ReadMemoryEvent object, while or even *before* the callbacks are
             * processed. Thus, a callback that accesses the ReadMemoryEvent
             * object may raise a SIGSEGV. */
            *event = std::make_shared<ReadMemoryEvent>(commandId, _context,
                    mapData, unmapData);
        }
    } catch (const std::bad_alloc&) {
        throw cl::Error(CL_OUT_OF_RESOURCES);
    }
}

void CommandQueue::enqueueWriteBuffer(
        const std::shared_ptr<dcl::Buffer>& buffer,
        bool blockingWrite,
        size_t offset,
        size_t size,
        const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
        dcl::object_id commandId,
        std::shared_ptr<dcl::Event> *event) {
    auto bufferImpl = std::dynamic_pointer_cast<Buffer>(buffer);
    VECTOR_CLASS<cl::Event> nativeEventWaitList;
    cl::Event mapData, unmapData;
    cl::UserEvent copyData(*_context);

    if (!bufferImpl) throw cl::Error(CL_INVALID_MEM_OBJECT);

    /* Obtain wait list of native events */
    if (eventWaitList) {
        synchronize(*eventWaitList, nativeEventWaitList);
    }

    /* Enqueue map buffer */
    void *ptr = _commandQueue.enqueueMapBuffer(
            *bufferImpl,
            CL_FALSE,     // non-blocking map
            CL_MAP_WRITE, // map for writing
            offset, size,
            &nativeEventWaitList, &mapData);
    /* Enqueue unmap buffer (implicit upload) */
    nativeEventWaitList.assign(1, copyData);
    _commandQueue.enqueueUnmapMemObject(
            *bufferImpl,
            ptr,
            &nativeEventWaitList, &unmapData);
#ifdef FORCE_FLUSH
    _commandQueue.flush();
#else
    if (blockingWrite) {
        _commandQueue.flush();
    }
#endif

    try {
        /* TODO Sketch of a possible solution for a command-based implementation
         * Complex commands, i.e., commands which include interaction with the
         * host, should be implemented by a class and must be independent of an
         * event. */
#if 0
        /* The write buffer command notifies the host about its submission and
         * completion, regardless an event is associated with it or not. */
        std::shared_ptr<command::WriteBuffer> writeBuffer(
                std::make_shared<command::WriteBufferCommand>(_context->host(),
                        commandId, size, ptr, mapBuffer, dataReceipt, unmapData));

        if (event) { // an event should be associated with this command
            /* An event wrapper retains its associated command for further
             * commands from the host. Moreover, it broadcast command execution
             * status changes to other compute nodes in the command's context,
             * while the command only communicates with the host. */
            *event = std::make_shared<WriteMemoryEvent>(commandId, _context,
                    bufferImpl, writeBuffer);
        }
#else
        /* Schedule data receipt
         * A 'command submitted' message will be sent to the host. */
        mapData.setCallback(CL_COMPLETE, &executeCommand,
                new command::CopyDataCommand<command::HostToDevice>(
                        _context->host(), commandId, size, ptr, copyData));
        /* Schedule completion message for host
         * A 'command complete' message is sent to the host.
         * Note that this message must also be sent, if no event is associated
         * with this command, such that a blocking write succeeds. */
        unmapData.setCallback(CL_COMPLETE, &executeCommand,
                new command::SetCompleteCommand(_context->host(), commandId, cl::UserEvent(*_context)));

        if (event) { // an event should be associated with this command
            /* This event must only broadcast its status on other compute nodes
             * but not to the host, as a 'command complete' message is already
             * sent to the host by the callback that has been set for the native
             * event unmapData. */
            /* FIXME Avoid race condition in write buffer command
             * Callbacks are registered for unmapData to 1) notify the host
             * about command completion, and 2) to synchronize the memory object
             * associated with WriteMemoryEvent.
             * As the execution order of callback is unspecified, the host may
             * be notified about command completion (callback 1) before
             * callback 2 is executed. If the application and the network
             * respond quickly to callback 1 in order to delete the
             * WriteMemoryEvent object, it may be deleted *before* callback 2 is
             * processed. In this case callback 2 tries to access the deleted
             * WriteMemoryEvent object, such that a SIGSEGV will be raised. */
            *event = std::make_shared<WriteMemoryEvent>(commandId, _context,
                    bufferImpl, mapData, unmapData);
        }
#endif
    } catch (const std::bad_alloc&) {
        throw cl::Error(CL_OUT_OF_RESOURCES);
    }
}

void CommandQueue::enqueueMapBuffer(
        const std::shared_ptr<dcl::Buffer>& buffer,
        bool blockingMap,
        cl_map_flags map_flags,
        size_t offset,
        size_t size,
        const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
        dcl::object_id commandId,
        std::shared_ptr<dcl::Event> *event) {
    auto bufferImpl = std::dynamic_pointer_cast<Buffer>(buffer);
    VECTOR_CLASS<cl::Event> nativeEventWaitList;

    if (!bufferImpl) throw cl::Error(CL_INVALID_MEM_OBJECT);

    /* Obtain wait list of native events */
    if (eventWaitList) {
        synchronize(*eventWaitList, nativeEventWaitList);
    }

    if (map_flags & CL_MAP_READ) {
        /* The mapped memory region has to be synchronized, i.e., it has to be
         * downloaded to the mapped host pointer. */
        cl::Event mapData, unmapData;

        enqueueReadBuffer(bufferImpl, blockingMap, offset, size,
                nativeEventWaitList, commandId, mapData, unmapData);

        if (event) { // an event should be associated with this command
            /* The event must only broadcast its status on other compute nodes
             * but not to the host, as a 'command complete' message will be sent
             * to the host by the callback that has been set for the native
             * event unmapData. */
            /* WARNING: No callback must be registered for any native event of
             * the CompoundNodeEvent object, that access the object. As the order
             * of execution of callbacks is undefined, the application may delete
             * the CompoundNodeEvent object, while the callbacks are processed.
             * Thus, a callback that accesses the CompoundNodeEvent object may
             * raise a SIGSEGV. */
            try {
                *event = std::make_shared<ReadMemoryEvent>(commandId, _context,
                        mapData, unmapData);
            } catch (const std::bad_alloc&) {
                throw cl::Error(CL_OUT_OF_RESOURCES);
            }
        }
    } else {
        /* The mapped memory region has *not* to be synchronized, as it will not
         * be read. */
        cl::Event marker;

        enqueuePhonyMarker(blockingMap, nativeEventWaitList, commandId, marker);

        if (event) { // an event should be associated with this command
            /* This event must only broadcast its status on other compute nodes
             * but not to the host, as a 'command complete' message is already
             * sent to the host by the callback that has been set for the native
             * event marker. */
            /* WARNING: No callback must be registered for the native event of
             * the SimpleNodeEvent object, that access the object. As the order
             * of execution of callbacks is undefined, the application may delete
             * the SimpleNodeEvent object, while the callbacks are processed.
             * Thus, a callback that accesses the SimpleNodeEvent object may
             * raise a SIGSEGV. */
            try {
                *event = std::make_shared<SimpleNodeEvent>(commandId, _context, marker);
            } catch (const std::bad_alloc&) {
                throw cl::Error(CL_OUT_OF_RESOURCES);
            }
        }
    }
}

void CommandQueue::enqueueUnmapBuffer(
        const std::shared_ptr<dcl::Buffer>& buffer,
        cl_map_flags map_flags,
        size_t offset,
        size_t size,
        const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
        dcl::object_id commandId,
        std::shared_ptr<dcl::Event> *event) {
    auto bufferImpl = std::dynamic_pointer_cast<Buffer>(buffer);
    VECTOR_CLASS<cl::Event> nativeEventWaitList;

    if (!bufferImpl) throw cl::Error(CL_INVALID_MEM_OBJECT);

    /* Obtain wait list of native events */
    if (eventWaitList) {
        synchronize(*eventWaitList, nativeEventWaitList);
    }

    if (map_flags & CL_MAP_WRITE) {
        /* The mapped memory region has to be synchronized, i.e. its data
         * has to be uploaded to the buffer. */
        cl::Event mapData, unmapData;

        enqueueWriteBuffer(bufferImpl, false, offset, size,
                nativeEventWaitList, commandId, mapData, unmapData);

        if (event) { // an event should be associated with this command
            /* The event must only broadcast its status on other compute nodes
             * but not to the host, as a 'command complete' message will be sent
             * to the host by the callback that has been set for the native
             * event unmapData. */
            try {
                *event = std::make_shared<WriteMemoryEvent>(commandId, _context,
                        bufferImpl, mapData, unmapData);
            } catch (const std::bad_alloc&) {
                throw cl::Error(CL_OUT_OF_RESOURCES);
            }
        }
    } else {
        /* The mapped memory region has *not* to be synchronized, as it has
         * not been written. */
        cl::Event marker;

        enqueuePhonyMarker(false, nativeEventWaitList, commandId, marker);

        if (event) { // an event should be associated with this command
            /* The event must only broadcast its status on other compute nodes
             * but not to the host, as a 'command complete' message will be sent
             * to the host by the callback that has been set for the native
             * event marker. */
            /* WARNING: No callback must be registered for the native event of
             * the SimpleNodeEvent object, that access the object. As the order
             * of execution of callbacks is undefined, the application may delete
             * the SimpleNodeEvent object, while the callbacks are processed.
             * Thus, a callback that accesses the SimpleNodeEvent object may
             * raise a SIGSEGV. */
            try {
                *event = std::make_shared<SimpleNodeEvent>(commandId, _context, marker);
            } catch (const std::bad_alloc&) {
                throw cl::Error(CL_OUT_OF_RESOURCES);
            }
        }
    }
}

void CommandQueue::enqueueNDRangeKernel(
        const std::shared_ptr<dcl::Kernel>& kernel,
        const std::vector<size_t>& offset,
        const std::vector<size_t>& global,
        const std::vector<size_t>& local,
        const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
        dcl::object_id commandId,
        std::shared_ptr<dcl::Event> *event) {
    auto kernelImpl = std::dynamic_pointer_cast<Kernel>(kernel);
    VECTOR_CLASS<cl::Event> nativeEventWaitList;
    cl::Event ndRangeKernel;

    if (!kernelImpl) throw cl::Error(CL_INVALID_KERNEL);

    /* Obtain wait list of native events */
    if (eventWaitList) {
        synchronize(*eventWaitList, nativeEventWaitList);
    }

    /* Enqueue ND range kernel
     * Only create event if requested by caller */
    _commandQueue.enqueueNDRangeKernel(
            *kernelImpl,
            createNDRange(offset), createNDRange(global), createNDRange(local),
            &nativeEventWaitList, event ? &ndRangeKernel : nullptr);
#ifdef FORCE_FLUSH
    _commandQueue.flush();
#endif

    if (event) { // an event should be associated with this command
        try {
            *event = std::make_shared<SimpleEvent>(commandId, _context,
                    kernelImpl->writeMemoryObjects(), ndRangeKernel);
        } catch (const std::bad_alloc&) {
            throw cl::Error(CL_OUT_OF_RESOURCES);
        }
    }
}

/*
 * The semantics of enqueueMarker is unclear:
 * What is clEnqueueMarkerWithWaitList good for, if no event is returned?
 */
void CommandQueue::enqueueMarker(
        const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
        dcl::object_id commandId, std::shared_ptr<dcl::Event> *event) {
    VECTOR_CLASS<cl::Event> nativeEventWaitList;
    cl::Event marker;

    /* Obtain wait list of native events */
    if (eventWaitList) {
        synchronize(*eventWaitList, nativeEventWaitList);
    }

#if defined(CL_VERSION_1_2)
    /* Use native enqueueMarkerWithWaitList */
    _commandQueue.enqueueMarkerWithWaitList(
            &nativeEventWaitList, event ? &marker : nullptr);
#else // #if defined(CL_VERSION_1_2)
    if (eventWaitList) {
        /* TODO Use pre-OpenCL 1.2 APIs to implement CommandQueue::enqueueMarker with wait list
         * clEnqueueMarker (deprecated) does not support an event list. */
        assert(!"clEnqueueMarkerWithWaitList not implemented");
#if 0
        /* TODO Use count-down latch to implement clEnqueueMarkerWithWaitList
         * Each event in wait list decrements the latch once. When the latch is
         * decremented to 0 (triggered), it sets the marker to complete. The
         * event which triggered the latch also deletes it.
         * To ensure in-order execution (for in-order command-queues), a marker
         * is enqueued and added to the event wait list. */
        /* WARNING: The marker event is implemented by a user event. As such it
         * cannot be used in the event list of clEnqueueWaitForEvents. Hence,
         * clEnqueueMarkerWithWaitList cannot be fully implemented using the
         * OpenCL 1.1 APIs. */
        cl::UserEvent complete;

        if ((_commandQueue.getInfo<CL_QUEUE_PROPERTIES>() & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)) {
            cl::Event execution;
            _commandQueue.enqueueMarker(&execution);
            nativeEventWaitList.push_back(execution);
        }

        auto cdl = new CountDownLatch(nativeEventWaitList.size(),
                std::bind(&cl::UserEvent::setStatus, complete, CL_COMPLETE));
        for (auto nativeEvent : nativeEventWaitList) {
            nativeEvent->setCallback(CL_COMPLETE, &countDown, cdl);
        }

        marker = complete; // downcast marker to cl::Event
#endif
    } else {
        _commandQueue.enqueueMarker(&marker);
    }
#endif // #if defined(CL_VERSION_1_2)

    if (event) { // an event should be associated with this command
        try {
            *event = std::make_shared<SimpleEvent>(commandId, _context, marker);
        } catch (const std::bad_alloc&) {
            throw cl::Error(CL_OUT_OF_RESOURCES);
        }
    }
}

#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
void CommandQueue::enqueueWaitForEvents(
        const std::vector<std::shared_ptr<dcl::Event>>& eventList) {
    VECTOR_CLASS<cl::Event> nativeEventList;

	assert(!eventList.empty()); // event list must no be empty

    dcl::util::Logger << dcl::util::Debug
            << "Synchronizing event list with " << eventList.size() << " event(s)"
            << std::endl;

    /* Obtain native event list
     * Unlike other enqueued commands, wait-for-events throws CL_INVALID_EVENT,
     * rather than CL_INVALID_EVENT_WAIT_LIST, if the event list contains an
     * invalid event. */
    for (auto event : eventList) {
        /* TODO Create Event::synchronize method (see CommandQueue::synchronize) */
        auto remoteEvent = std::dynamic_pointer_cast<RemoteEvent>(event);
        if (remoteEvent) { // event is a remote event
            VECTOR_CLASS<cl::Event> synchronizeEvents;
            remoteEvent->synchronize(_commandQueue, synchronizeEvents);
            nativeEventList.insert(std::end(nativeEventList),
                    std::begin(synchronizeEvents), std::end(synchronizeEvents));
        } else { // event is a local event
            auto eventImpl = std::dynamic_pointer_cast<Event>(event);
            if (!eventImpl) throw cl::Error(CL_INVALID_EVENT);
            nativeEventList.push_back(*eventImpl);
        }
    }

    _commandQueue.enqueueWaitForEvents(nativeEventList);
}
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

/* enqueueBarrier is a synchronization point */
void CommandQueue::enqueueBarrier(
        const std::vector<std::shared_ptr<dcl::Event>> *eventWaitList,
        dcl::object_id commandId, std::shared_ptr<dcl::Event> *event) {
    VECTOR_CLASS<cl::Event> nativeEventWaitList;
    cl::Event barrier;

    /* Obtain wait list of native events */
    if (eventWaitList) {
	    synchronize(*eventWaitList, nativeEventWaitList);
	}

#if defined(CL_VERSION_1_2)
    /* Use native enqueueBarrierWithWaitList */
    _commandQueue.enqueueBarrierWithWaitList(
            &nativeEventWaitList, event ? &barrier : nullptr);
#else // #if defined(CL_VERSION_1_2)
    /* Use pre-OpenCL 1.2 APIs to implement CommandQueue::enqueueBarrier with wait list */
    /* clEnqueueBarrier (deprecated) does not support an event wait list nor
     * does is return an event. */
    if (eventWaitList) {
        assert(!"clEnqueueBarrierWithWaitList not implemented");
#if 0
        try {
            /* WARNING: According to the OpenCL 1.1 specification the event list
             * of clEnqueueWaitForEvents must not contain user events. Moreover,
             * unlike clEnqueueBarrier, clEnqueueWaitForEvents is not explicitly
             * defined as a synchronization point. Hence,
             * clEnqueueBarrierWithEventList cannot be fully implemented using
             * the OpenCL 1.1 APIs. */
            _commandQueue.enqueueWaitForEvents(nativeEventWaitList);
        } catch (const cl::Error& err) {
            /* cl::CommandQueue::enqueueWaitForEvents throws CL_INVALID_EVENT if
             * an event in the wait list is not a valid event, but
             * enqueueBarrier (with wait list) must throw
             * CL_INVALID_EVENT_WAIT_LIST in this case. */
            if (err.err() == CL_INVALID_EVENT) {
                throw cl::Error(CL_INVALID_EVENT_WAIT_LIST);
            } else {
                throw err;
            }
        }
#endif
    } else {
        _commandQueue.enqueueBarrier();
    }
    if (event) {
        _commandQueue.enqueueMarker(&barrier);
        /* Ensure that the marker has finished execution before any future
         * command is executed. This is implied for in-order command-queues but
         * requires additional synchronization using clEnqueueWaitForEvents for
         * out-of-order command-queues. */
        _commandQueue.enqueueWaitForEvents(VECTOR_CLASS<cl::Event>(1, barrier));
        /* TODO Lock command-queue when enqueuing clEnqueueBarrierWithWaitList
         * No other command must be enqueued concurrently, such that
         * clEnqeueueWaitForEvents is executed right after clEnqueueMarker. */
    }
#endif // #if defined(CL_VERSION_1_2)

    /* TODO Implement synchronization for clEnqueueBarrierWithWaitList
     * It is not clear from the specification what this means; maybe it acts as
     * an active release, i.e. changes to the memory objects by this
     * command-queue are replicated to all other compute nodes. */

    if (event) { // an event should be associated with this command
        /* We assume that an event returned by clEnqueueBarrierWithWaitList
         * cannot be used to synchronize changes to a memory object across
         * command queues, as the specification explicitly stipulates to use
         * the event for the command that modified the memory objects for
         * synchronization (see Appendix A.1). */
        try {
            *event = std::make_shared<SimpleEvent>(commandId, _context, barrier);
        } catch (const std::bad_alloc&) {
            throw cl::Error(CL_OUT_OF_RESOURCES);
        }
    }
}

const std::shared_ptr<Context>& CommandQueue::context() const {
    return _context;
}

} /* namespace dcld */
