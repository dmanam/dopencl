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
 * \file Command.h
 *
 * \date 2012-03-24
 * \author Philipp Kegel
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include <dcl/CommandListener.h>
#include <dcl/Remote.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <condition_variable>
#include <mutex>

namespace dclicd {

class Event;

/* ****************************************************************************/

namespace command {

class Command: public dcl::Remote, public dcl::CommandListener {
public:
    /*!
     * \brief Creates a command
     *
     * \param[in]  command_type     the type of this command
     * \param[in]  command_queue    the command queue that is associated with this command
     */
    Command(
            cl_command_type     type,
            cl_command_queue    commandQueue);
    virtual ~Command();

    cl_command_type type() const;
    cl_command_queue commandQueue() const;

    /*!
     * \brief Attaches an event to this command.
     *
     * This operation must be called only once, as the event shall not be
     * detached or changed.
     *
     * \param[in]  event    the event to attach to this command
     */
    void setEvent(
            dclicd::Event& event);

    /*!
     * \brief Checks, if this command is complete
     *
     * \return \c true, if this command is complete, otherwise \c false
     */
    bool isComplete() const;

    /*!
     * \brief Wait for the command to be completed.
     */
    void wait() const;

    /*
     * Command listener API
     */

    void onExecutionStatusChanged(
            cl_int executionStatus);

protected:
    /*!
     * \brief Executes this command when its execution status changes to \c CL_SUBMITTED.
     *
     * \return a new execution status of the command
     * The returned execution status depends on whether the host can complete
     * the command directly (\c CL_COMPLETE is returned), or the command will be
     * completed later (by the host or compute node, \c CL_RUNNING will be
     * returned).
     */
    virtual cl_int submit();

    /*!
     * \brief Finishes this command when its execution status changes to \c CL_COMPLETE or an error code
     *
     * \param[in]  errcode  return (error) code of the preceding command execution steps
     * \return the commands final return code; \c CL_COMPLETE on success
     */
    virtual cl_int complete(
            cl_int errcode);

    cl_command_type _type; //!< Command type
    cl_command_queue _commandQueue; //!< Command queue that the command has been enqueued to
    cl_int _executionStatus; //!< Execution status
    mutable std::recursive_mutex _executionStatusMutex; //!< Mutex for command execution status
    mutable std::condition_variable_any _executionStatusChanged; //!< Condition: command execution status changed

    dclicd::Event *_event; //!< Associated event; can be \c NULL
};

} /* namespace command */

} /* namespace dclicd */

#endif /* COMMAND_H_ */
