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

#ifndef SESSION_H_
#define SESSION_H_

#include <dcl/CommandQueue.h>
#include <dcl/ComputeNode.h>
#include <dcl/Context.h>
#include <dcl/ContextListener.h>
#include <dcl/DCLTypes.h>
#include <dcl/Device.h>
#include <dcl/Event.h>
#include <dcl/Host.h>
#include <dcl/Kernel.h>
#include <dcl/Memory.h>
#include <dcl/Program.h>
#include <dcl/Session.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <cstddef>
#include <memory>
#include <set>
#include <vector>

namespace dcld {

/*!
 * The Session class saves the state of a connected host, i.e., it manages all
 * OpenCL objects that have been created on a compute node to implement a
 * corresponding OpenCL object on the host.
 */
class Session: public dcl::Session {
public:
    /*!
     * \brief Creates a session associated with the specified platform.
     *
     * \param[in]  platform the platform associated with this session
     */
    Session(
            const cl::Platform& platform);
    virtual ~Session();

	/* Session APIs */
    std::shared_ptr<dcl::Context> createContext(
	        dcl::Host&                                      host,
            const std::vector<dcl::ComputeNode *>&          computeNodes,
	        const std::vector<dcl::Device *>&               devices,
	        const std::shared_ptr<dcl::ContextListener>&    listener);
	void releaseContext(
	        const std::shared_ptr<dcl::Context>& context);

	std::shared_ptr<dcl::CommandQueue> createCommandQueue(
			const std::shared_ptr<dcl::Context>&    context,
			dcl::Device *                           device,
			cl_command_queue_properties             properties);
	void releaseCommandQueue(
	        const std::shared_ptr<dcl::CommandQueue>& commandQueue);

	std::shared_ptr<dcl::Buffer> createBuffer(
			const std::shared_ptr<dcl::Context>&    context,
			cl_mem_flags		                    flags,
			size_t				                    size,
			void *                                  ptr);
	void releaseMemObject(
	        const std::shared_ptr<dcl::Memory>& memory);

	std::shared_ptr<dcl::Program> createProgram(
			const std::shared_ptr<dcl::Context>&    context,
			const char *                            source,
			size_t                                  length);
	std::shared_ptr<dcl::Program> createProgram(
            const std::shared_ptr<dcl::Context>&    context,
            const std::vector<dcl::Device *>&       deviceList,
            const std::vector<size_t>&              lengths,
            const unsigned char **                  binaries,
            std::vector<cl_int> *                   binary_status);
	void releaseProgram(
	        const std::shared_ptr<dcl::Program>& program);

	std::shared_ptr<dcl::Kernel> createKernel(
			const std::shared_ptr<dcl::Program>&    program,
			const char *		                    name);
	std::vector<std::shared_ptr<dcl::Kernel>> createKernelsInProgram(
			const std::shared_ptr<dcl::Program>&    program,
            cl_uint                                 numKernels);
	void releaseKernel(
	        const std::shared_ptr<dcl::Kernel>& kernel);

    void addEvent(
            const std::shared_ptr<dcl::Event>& event);
	std::shared_ptr<dcl::Event> createEvent(
            dcl::object_id                                      id,
            const std::shared_ptr<dcl::Context>&                context,
            const std::vector<std::shared_ptr<dcl::Memory>>&    memoryObjects);
	void releaseEvent(
	        const std::shared_ptr<dcl::Event>& event);

private:
	/* Sessions must be non-copyable */
	Session(
	        const Session& rhs) = delete;
	Session& operator=(
	        const Session& rhs) = delete;

    cl::Platform _platform;

    std::set<std::shared_ptr<dcl::Context>> _contexts; //!< Context list
    std::set<std::shared_ptr<dcl::Memory>> _memoryObjects; //!< Memory object list
    std::set<std::shared_ptr<dcl::CommandQueue>> _commandQueues; //!< Command queue list
    std::set<std::shared_ptr<dcl::Program>> _programs; //!< Program list
    std::set<std::shared_ptr<dcl::Kernel>> _kernels; //!< Kernel list
    std::set<std::shared_ptr<dcl::Event>> _events; //!< Event list
};

} /* namespace dcld */

#endif /* SESSION_H_ */
