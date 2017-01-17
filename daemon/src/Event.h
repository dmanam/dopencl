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
 * \file Event.h
 *
 * \date 2012-07-28
 * \author Philipp Kegel
 * \author Sebastian Pribnow
 */

#ifndef EVENT_H_
#define EVENT_H_

#include <dcl/CommandListener.h>
#include <dcl/DCLTypes.h>
#include <dcl/Event.h>
#include <dcl/Remote.h>
#include <dcl/SynchronizationListener.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <memory>
#include <mutex>
#include <vector>

namespace dcld {

class Context;
class Memory;

/* ****************************************************************************/

/*!
 * \brief An abstract base class for a decorator for any kind of event.
 *
 * This wrapper associates a native event (not part of this class, hence it is
 * abstract) with a context and a list of memory objects.
 */
class Event: public dcl::Event {
public:
    Event(
            const std::shared_ptr<Context>&               context,
            const std::vector<std::shared_ptr<Memory>>&   memoryObjects);
    Event(
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<Memory>&  memoryObject);
    Event(
            const std::shared_ptr<Context>& context);

    /*!
     * \brief Returns the native event associated with this decorator
     */
    virtual operator cl::Event() const = 0;

protected:
    std::shared_ptr<Context> _context; //!< Context associated with this event
    /*!
     * \brief Memory objects associated with this event's command
     *
     * The memory objects in this list are synchronized when the command
     * associated with this event is complete, and when this event is passed to
     * a command in an event wait list.
     */
    std::vector<std::shared_ptr<Memory>> _memoryObjects;

private:
    /* Events must be non-copyable */
    Event(
            const Event& rhs) = delete;
    Event& operator=(
            const Event& rhs) = delete;
};

/* ****************************************************************************/

/*!
 * \brief A replacement for an event on another compute node or on the host.
 *
 * A remote event is a substitute event for a native event on another compute
 * node or host (i.e., a user event). Internally, a native user event is provided
 * and passed to native OpenCL functions as a replacement.
 */
class RemoteEvent: public dcl::Remote, public Event {
public:
    RemoteEvent(
            dcl::object_id                                id,
            const std::shared_ptr<Context>&               context,
            const std::vector<std::shared_ptr<Memory>>&   memoryObjects);

    operator cl::Event() const;

    /*!
     * \brief Synchronizes (acquires) the changes associated with the remote event
     *
     * \param[in]  commandQueue     the command queue used for acquiring changes
     * \param[in]  nativeEventList  the native events associated with the acquiring operations
     */
    void synchronize(
            const cl::CommandQueue&  commandQueue,
            VECTOR_CLASS<cl::Event>& nativeEventList);

    /*
     * Event API
     */
    void getProfilingInfo(
            cl_profiling_info   param_name,
            cl_ulong&           param_value) const;

    /*
     * Command listener API
     */

    void onExecutionStatusChanged(
            cl_int executionStatus);

    /*
     * Synchronization listener API
     */

    void onSynchronize(
            dcl::Process& process);

private:
    cl::UserEvent _event; //!< Native user event
    std::vector<cl::Event> _syncEvents; //!< Native events used for synchronization
    mutable std::mutex _syncMutex; //!< Mutex for synchronization event list
};

/* ****************************************************************************/

/*!
 * \brief A decorator for one or more native events that are associated with a command.
 *
 * The method onExecutionStatusChanged (inherited from CommandListener) is
 * called by wrapped events, i.e., locally, rather than by messages from other
 * processes.
 */
class LocalEvent: public dcl::Remote, public Event {
public:
    /*!
     * \brief Wraps a native OpenCL event on this compute node.
     */
    LocalEvent(
            dcl::object_id                              id,
            const std::shared_ptr<Context>&             context,
            const std::vector<std::shared_ptr<Memory>>& memoryObjects);
    LocalEvent(
            dcl::object_id                  id,
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<Memory>&  memoryObject);
    LocalEvent(
            dcl::object_id                  id,
            const std::shared_ptr<Context>& context);

    ~LocalEvent();

    /*
     * Synchronization listener API
     */

    void onSynchronize(
            dcl::Process& process);

protected:
    cl_ulong _received; //!< Receipt time of associated command
};

/* ****************************************************************************/

/*!
 * \brief A decorator for a single native event.
 *
 * This is a basic implementation of the LocalEvent class. It forwards API
 * calls to its native events and broadcasts a 'command complete' message when
 * the command associated with this event is completed or terminated.
 */
class SimpleEvent: public LocalEvent {
public:
    /*!
     * \brief Creates an event by decorating a native OpenCL event.
     */
    SimpleEvent(
            dcl::object_id                              id,
            const std::shared_ptr<Context>&             context,
            const std::vector<std::shared_ptr<Memory>>& memoryObjects,
            const cl::Event&                            event);
    SimpleEvent(
            dcl::object_id                  id,
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<Memory>&  memoryObject,
            const cl::Event&                event);
    SimpleEvent(
            dcl::object_id                  id,
            const std::shared_ptr<Context>& context,
            const cl::Event&                event);

    operator cl::Event() const;

    /*
     * Event API
     */

    void getProfilingInfo(
            cl_profiling_info   param_name,
            cl_ulong&           param_value) const;

    /*
     * Command listener API
     */

    void onExecutionStatusChanged(
            cl_int executionStatus);

private:
    cl::Event _event; //!< Native event
};

/* ****************************************************************************/

/*!
 * This is an implementation of the SimpleEvent class which sends a 'command
 * complete' message to other compute nodes, but *not* to the host. This message
 * is sent to the host by the mechanism that implements the associated command,
 * while this event is only responsible for sending the message to other compute
 * nodes.
 */
class SimpleNodeEvent: public SimpleEvent {
public:
    /*!
     * \brief Creates an event by decorating a native OpenCL event.
     */
    SimpleNodeEvent(
            dcl::object_id                  id,
            const std::shared_ptr<Context>& context,
            const cl::Event&                event);

    /*
     * Command listener API
     */

    void onExecutionStatusChanged(
            cl_int executionStatus);
};

/* ****************************************************************************/

/*!
 * \brief A decorator for the native events of a number associated commands.
 *
 * This abstract class is an implementation of LocalEvent which depends on two
 * native events. Its provides a native event which is used by a native OpenCL
 * implementation as a replacement for itself and obtains profiling information
 * by merging information from its two native events.
 */
class CompoundEvent: public LocalEvent {
public:
    /*!
     * \brief Creates a compound event from two native OpenCL events.
     */
    CompoundEvent(
            dcl::object_id                  id,
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<Memory>&  memoryObject,
            const cl::Event&                startEvent,
            const cl::Event&                endEvent);
    CompoundEvent(
            dcl::object_id                  id,
            const std::shared_ptr<Context>& context,
            const cl::Event&                startEvent,
            const cl::Event&                endEvent);

    operator cl::Event() const;

    /*
     * Event API
     */

    void getProfilingInfo(
            cl_profiling_info   param_name,
            cl_ulong&           param_value) const;

protected:
    cl::Event _startEvent; //!< Native event associated with the start of this compound event
    cl::Event _endEvent; //!< Native event associated with the completion of this compound event
};

/* ****************************************************************************/

/*!
 * \brief A compound event associated with a read buffer or read image command.
 *
 * This class is an implementation of CompoundEvent which sends no 'command
 * complete' messages, as this message is send by the host for read buffer/image
 * commands.
 */
class ReadMemoryEvent: public CompoundEvent {
public:
    ReadMemoryEvent(
            dcl::object_id                  id,
            const std::shared_ptr<Context>& context,
            const cl::Event&                startEvent,
            const cl::Event&                endEvent);

    /*
     * Event API
     */

    void getProfilingInfo(
            cl_profiling_info param_name,
            cl_ulong&         param_value) const;

    /*
     * Command listener API
     */

    void onExecutionStatusChanged(
            cl_int executionStatus);
};

/* ****************************************************************************/

/*!
 * \brief A compound event associated with a write buffer or write image command.
 *
 * This is an implementation of the CompoundEvent class which sends a 'command
 * complete' message to other compute nodes, but *not* to the host. This message
 * is sent to the host by the mechanism that implements the 'write memory
 * object' command while this event is only responsible for sending the message
 * to other compute nodes.
 */
class WriteMemoryEvent: public CompoundEvent {
public:
    WriteMemoryEvent(
            dcl::object_id                  id,
            const std::shared_ptr<Context>& context,
            const std::shared_ptr<Memory>&  memoryObject,
            const cl::Event&                startEvent,
            const cl::Event&                endEvent);

    /*
     * Command listener API
     */

    void onExecutionStatusChanged(
            cl_int executionStatus);
};

} /* namespace dcld */

#endif /* EVENT_H_ */
