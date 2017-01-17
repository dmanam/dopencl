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
 * \file Context.h
 *
 * \date 2012-03-06
 * \author Philipp Kegel
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <dcl/ComputeNode.h>
#include <dcl/Context.h>
#include <dcl/ContextListener.h>
#include <dcl/Device.h>
#include <dcl/Host.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace dcld {

/*!
 * \brief A decorator for a native context.
 *
 * This wrapper is required to notify context listeners about context errors.
 * Moreover, this wrapper holds a command queue for asynchronously reading and
 * writing data.
 */
class Context: public dcl::Context {
public:
    /* TODO Pass context listener by raw pointer or reference */
    Context(
            dcl::Host&                              		host,
            const std::vector<dcl::ComputeNode *>&          computeNodes,
            const cl::Platform&                             platform,
            const std::vector<dcl::Device *>&               devices,
            const std::shared_ptr<dcl::ContextListener>&    listener);
    virtual ~Context();

    operator cl::Context() const;

    dcl::Host& host() const;
    const cl::CommandQueue& ioCommandQueue() const;
    const std::vector<dcl::ComputeNode *>& computeNodes() const;

private:
    /* Contexts must be non-copyable */
    Context(
            const Context& rhs) = delete;
    Context& operator=(
            const Context& rhs) = delete;

    dcl::Host& _host; //!< Host associated with this context
    std::vector<dcl::ComputeNode *> _computeNodes; //!< Compute nodes associated with this context

    cl::Context _context; //!< Native context
    /*!
     * \brief Native command queue for asynchronous read/write
     *
     * This command queue is used to read or write data from any memory object
     * that is associated with this context. This is required for the memory
     * consistency protocol
     */
    cl::CommandQueue _ioCommandQueue;

    std::shared_ptr<dcl::ContextListener> _listener;
};

} /* namespace dcld */

#endif /* CONTEXT_H_ */
