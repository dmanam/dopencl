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
 * \author Louay Hashim
 */

#ifndef CL_EVENT_H_
#define CL_EVENT_H_

#include "Retainable.h"

#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <condition_variable>
#include <map>
#include <mutex>
#include <utility>
#include <vector>


class _cl_event:
public _cl_retainable {
public:
    /*!
     * \brief Waits until all events in event_list are complete.
     * This method implements clWaitForEvents.
     * As by the OpenCL specification this is a blocking operation and thus
     * implicitly calls _cl_command_queue::flush on the events' command
     * queues.
     *
     * \param[in]  event_list   the events to wait for
     */
    static void waitForEvents(
            const std::vector<cl_event>& event_list);


    virtual ~_cl_event();

    void retain();
    bool release();

    virtual dcl::object_id remoteId() const = 0;

    /**
     * \brief Wait for the event to be completed.
     *
     * This method is a convenience method for waitForEvents.
     */
    virtual void wait() const = 0;

    void setCallback(
            cl_int              command_exec_callback_type,
            void (CL_CALLBACK * pfn_event_notify)(
                    cl_event event,
                    cl_int event_command_exec_status,
                    void *user_data),
            void *              user_data);

    void getInfo(
            cl_event_info   param_name,
            size_t          param_value_size,
            void *          param_value,
            size_t *        param_value_size_ret) const;

    /*!
     * \brief Returns profiling information on this event.
     *
     * \param[in]  param_name
     * \param[in]  param_size
     * \param[out] param_value
     * \param[out] param_size_ret
     */
    virtual void getProfilingInfo(
            cl_profiling_info   param_name,
            size_t              param_value_size,
            void *              param_value,
            size_t *            param_value_size_ret) const = 0;

protected:
    /*!
     * \brief Creates a (user) event.
     *
     * \param[in]  context
     * \param[in]  status   initial event status
     */
    _cl_event(
            cl_context  context,
            cl_int      status);

    /*!
     * \brief Query command type.
     *
     * This method is reserved for internal use by getInfo.
     *
     * \return the type of the command that is associated with this event
     */
    virtual cl_command_type commandType() const = 0;

    /*!
     * \brief Query the command queue associated with this event.
     *
     * This method is reserved for internal use by getInfo.
     *
     * \return the command queue associated with this event
     */
    virtual cl_command_queue commandQueue() const = 0;

    /*!
     * \brief Checks, if this event is complete
     *
     * This method is reserved for internal use.
     *
     * \return true, if this event is complete, otherwise false
     */
    bool isComplete() const;

    /*!
     * \brief Waits for the event to be completed.
     *
     * This method is reserved for internal use.
     * Unlike \c wait it does *not* perform an implicit flush.
     */
    void waitNoFlush() const;

    /*!
     * \brief Sets the event's command execution status.
     *
     * This event is destroyed, if its reference count is zero and its
     * associated command has finished. The caller is responsible for deleting
     * this event.
     *
     * This method is reserved for internal use.
     *
     * \param[in]  status   the command execution status this events should switch to
     * \return \c true, if this event has been destroyed, otherwise \c false
     */
    bool setCommandExecutionStatus(
            cl_int status);

    void destroy();

    cl_context _context;

    cl_int _status; //!< Event status
    mutable std::recursive_mutex _statusMutex; //!< Mutex for event status
    mutable std::condition_variable_any _statusChanged; //!< Condition: event status changed

private:
    /*!
     * \brief Triggers callbacks that have been registered for this event using \c setCallback.
     *
     * \param[in]  status   the command execution status for which the callbacks
     *                      should be triggered for
     */
    void triggerCallbacks(
            cl_int status);

    /*! Saves a list of callbacks for each command execution status */
    std::map<cl_int, std::vector<std::pair<void (*)(cl_event, cl_int, void *), void *>>> _callbacks;
};

#endif /* CL_EVENT_H_ */
