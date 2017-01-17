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
 * \file Session.h
 *
 * \date 2013-09-29
 * \author Philipp Kegel
 */

#ifndef DCL_SESSION_H_
#define DCL_SESSION_H_

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
class CommandQueue;
class ComputeNode;
class Context;
class ContextListener;
class Device;
class Event;
class Host;
class Kernel;
class Memory;
class Program;
class UserEvent;

/* ****************************************************************************/

/*!
 * \brief An interface for an application session
 *
 * An application session holds ownership of all OpenCL application objects.
 * It is a factory for these objects.
 */
class Session {
public:
	virtual ~Session() { }

    /*!
     * \brief Creates a context for this session
     */
	virtual std::shared_ptr<Context> createContext(
	        Host&                                   host,
	        const std::vector<ComputeNode *>&       computeNodes,
	        const std::vector<Device *>&            devices,
            const std::shared_ptr<ContextListener>& listener) = 0;

    /*!
     * \brief Deletes a context from this session
     *
     * \param[in]  context  the context to delete
     */
	virtual void releaseContext(
	        const std::shared_ptr<Context>& context) = 0;

    /*!
     * \brief Creates a command queue for this session
     */
    virtual std::shared_ptr<CommandQueue> createCommandQueue(
    		const std::shared_ptr<Context>& context,
    		Device *                        device,
	        cl_command_queue_properties     properties) = 0;

    /*!
     * \brief Deletes a command queue from this session
     *
     * \param[in]  commandQueue the command queue to delete
     */
	virtual void releaseCommandQueue(
	        const std::shared_ptr<CommandQueue>& commandQueue) = 0;

    /*!
     * \brief Creates a buffer for this session
     */
	virtual std::shared_ptr<Buffer> createBuffer(
			const std::shared_ptr<Context>& context,
            cl_mem_flags                    flags,
            size_t                          size,
            void *                          ptr) = 0;

    /*!
     * \brief Deletes a memory object (buffer or image) from this session.
     *
     * \param[in]  memory   the memory object to delete
     */
	virtual void releaseMemObject(
	        const std::shared_ptr<Memory>& memory) = 0;

    /*!
     * \brief Creates a program for this session from source
     */
	virtual std::shared_ptr<Program> createProgram(
			const std::shared_ptr<Context>& context,
			const char *                    source,
			size_t			                length) = 0;

    /*!
     * \brief Creates a program for this session from binary
     */
	virtual std::shared_ptr<Program> createProgram(
            const std::shared_ptr<Context>& context,
            const std::vector<Device *>&    deviceList,
			const std::vector<size_t>&      lengths,
			const unsigned char **          binaries,
			std::vector<cl_int> *           binary_status) = 0;

    /*!
     * \brief Deletes a program from this session.
     *
     * \param[in]  program  the program to delete
     */
	virtual void releaseProgram(
	        const std::shared_ptr<Program>& program) = 0;

    /*!
     * \brief Creates a kernel for this session
     */
    virtual std::shared_ptr<Kernel> createKernel(
    		const std::shared_ptr<Program>& program,
            const char *                    name) = 0;

    /*!
     * \brief Creates all kernels of a program for this session
     *
     * \param[in]  program      the program which kernels should be created
     * \param[in]  numKernels   expected number of kernels in program
     *             If \c program contains more or less than \c numKernels, \c CL_INVALID_VALUE is thrown
     * \return all kernels in \c program
     */
    virtual std::vector<std::shared_ptr<Kernel>> createKernelsInProgram(
            const std::shared_ptr<dcl::Program>&    program,
            cl_uint                                 numKernels) = 0;

    /*!
     * \brief Deletes a kernel from this session.
     *
     * \param[in]  kernel   the kernel to delete
     */
    virtual void releaseKernel(
            const std::shared_ptr<Kernel>& kernel) = 0;

    virtual void addEvent(
            const std::shared_ptr<Event>& event) = 0;

    /*!
     * \brief Creates a replacement event (remote or user event) and adds it to the session's event list
     *
     * \param[in]  id				command ID
     * \param[in]  context          the context associated with the event
     * \param[in]  memoryObjects    the memory objects associated with the event
     */
	virtual std::shared_ptr<Event> createEvent(
            object_id                                   id,
            const std::shared_ptr<Context>&             context,
            const std::vector<std::shared_ptr<Memory>>& memoryObjects) = 0;

    /*!
     * \brief Deletes an event from this session.
     *
     * \param[in]  event    the event to delete
     */
	virtual void releaseEvent(
	        const std::shared_ptr<Event>& event) = 0;
};

} /* namespace dcl */

#endif /* DCL_SESSION_H_ */
