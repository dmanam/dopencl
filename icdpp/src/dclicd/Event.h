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
 * \date 2011-06-21
 * \author Philipp Kegel
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "../Event.h"

#include "detail/EventProfilingInfo.h"

#include <dcl/DCLTypes.h>
#include <dcl/Process.h>
#include <dcl/Remote.h>
#include <dcl/SynchronizationListener.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <memory>
#include <vector>

namespace dclicd {

namespace command {

/* forward declarations */
class Command;

} /* namespace command */

/******************************************************************************/

class Event:
	public _cl_event,                       // extends _cl_event
	public dcl::SynchronizationListener {   // implements SynchronizationListener
public:
    /*!
     * \brief Creates an event associated with a specified command.
     *
     * On compute nodes other than the compute node where the associated command
     * has been enqueued, a substitute event is created to replace the native
     * OpenCL event which resides on the compute node where the associated
     * command has been enqueued.
     * Internally, these substitute event are implemented as user events, but
     * they hold additional information for synchronization purposes.
     *
     * \param[in]  context          the context that is associated with this event
     * \param[in]  command          the command that this event is associated with
     * \param[in]  memoryObjects    the memory objects associated with this event
     */
    Event(
            cl_context                                  context,
            const std::shared_ptr<command::Command>&    command,
            const std::vector<cl_mem>&                  memoryObjects = std::vector<cl_mem>());
    virtual ~Event();

    dcl::object_id remoteId() const;

    /*!
     * \brief Wait for the event to be completed.
     *
     * This method is a convenience method for _cl_event::waitForEvents.
     * Unlike _cl_event::wait it performs an implicit flush on the event's
     * associated command queue.
     */
    void wait() const;

    /*!
     * \brief Sets the event's command execution status.
     *
     * This method should only be called by the event's associated command to
     * synchronize the event's command execution status with the actual
     * execution status of its associated command
     *
     * \param[in]  status   the new command execution status
     * \return \c true, if this event has been destroyed, otherwise \c false
     *
     * \see _cl_event::setCommandExecutionStatus
     */
    bool onCommandExecutionStatusChanged(
            cl_int status);

    void getProfilingInfo(
            cl_profiling_info   param_name,
            size_t              param_value_size,
            void *              param_value,
            size_t *            param_value_size_ret) const;

    /*!
     * \brief Synchronizes (acquires) the changes associated with this event
     *
     * Usually, this method is implicitly called when waiting for an event.
     * However, clFinish also is a synchronization point and, therefore,
     * requires the implementation to call this method.
     */
    void synchronize();

    /*
     * SynchronizationListener API
     *
     * This method is only required to work-around the missing node-to-node
     * communication. The host will never own an event which a compute node can
     * synchronize with. However, the host can synchronize with a compute node's
     * event.
     */

    void onSynchronize(
            dcl::Process& process);

protected:
    cl_command_type commandType() const;
    cl_command_queue commandQueue() const;

private:
    std::shared_ptr<command::Command> _command; //!< Command that is associated with this event
    cl_ulong _commandQueued; //!< Queuing time of command on host
    std::vector<cl_mem> _memoryObjects; //!< Memory objects associated with this event

    mutable std::unique_ptr<detail::EventProfilingInfo> _profilingInfo; //!< Profiling info (optional, cached)
};

/******************************************************************************/

#if 0
class ReadMemoryEvent : public Event { // extends Event
public:
    /*!
     * \brief Creates an event associated with a specified read memory object command.
     *
     * Unlike other events, this event is responsible for broadcasting a
     * 'command complete' message to other compute nodes, as the command is
     * finished on the host.
     * Currently, the same is performed by Event but as a work-around for
     * missing node-to-node communication.
     *
     * \param[in]  context  the context that is associated with this event
     * \param[in]  command  the read memory command that this event is associated with
     */
    ReadMemoryEvent(
            cl_context                  context,
            command::ReadMemoryCommand& command);
    virtual ~Event();

    void onCommandExecutionStatusChanged(
            cl_int status);
};
#endif

/******************************************************************************/

class UserEvent: public dcl::Remote, public _cl_event {
public:
    UserEvent(
            cl_context context);
    virtual ~UserEvent();

    /*!
     * \brief Returns this user event's remote ID
     *
     * This method implements _cl_event::remoteId and overwrites dcl::Remote::remoteId.
     *
     * \return this user event's remote ID
     */
    dcl::object_id remoteId() const;

    void wait() const;

    void setStatus(
            cl_int status);

    void getProfilingInfo(
            cl_profiling_info   param_name,
            size_t              param_value_size,
            void *              param_value,
            size_t *            param_value_size_ret) const;

protected:
    cl_command_type commandType() const;
    cl_command_queue commandQueue() const;
};

} /* namespace dclicd */

#endif /* EVENT_H_ */
