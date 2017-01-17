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
 * \file Memory.h
 *
 * \date 2011-09-01
 * \author Philipp Kegel
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <dcl/DCLTypes.h>
#include <dcl/Memory.h>
#include <dcl/Process.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <cstddef>
#include <memory>

namespace dcld {

class Context;

/* ****************************************************************************/

/*!
 * \brief A decorator for a native memory object.
 *
 * This wrapper is used to implement memory consistency across nodes in dOpenCL.
 */
class Memory: public virtual dcl::Memory {
public:
    virtual ~Memory();

    virtual operator cl::Memory() const = 0;

    size_t size() const;

    /*!
     * \brief Checks is this memory is used as input.
     *
     * isOutput and isInput are NOT mutually exclusive.
     *
     * \return \c true, if this memory is used as input, otherwise \c false.
     * \see isOutput
     */
    bool isInput() const;

    /*!
     * \brief Checks is this memory is used as output.
     *
     * isOutput and isInput are NOT mutually exclusive.
     *
     * \return \c true, if this memory is used as output, otherwise \c false.
     * \see isInput
     */
    bool isOutput() const;

    /*!
     * \brief Acquires the changes to this memory object associated with \c releaseEvent
     *
     * \param[in]  process      the process from which the changes should be acquired
     * \param[in]  commandQueue command queue for uploading the received data
     * \param[in]  releaseEvent event that releases the changes to be acquired
     * \param[out] acquireEvent event associated with this acquire operation
     */
    virtual void acquire(
            dcl::Process&           process,
            const cl::CommandQueue& commandQueue,
            const cl::Event&        releaseEvent,
            cl::Event *             acquireEvent) = 0;

    /*!
     * \brief Releases the changes to this memory object associated with \c releaseEvent
     *
     * The updated data of this memory object are sent to the requesting \c process.
     * \c commandQueue is used to obtain the data from the local OpenCL implementation.
     *
     * \param[in]  process      the process that requested the acquire operation
     * \param[in]  commandQueue command queue for downloading data before
     *                          sending
     * \param[in]  releaseEvent event associated with the operation that
     *                          releases the changes to be acquired
     */
    virtual void release(
            dcl::Process&           process,
            const cl::CommandQueue& commandQueue,
            const cl::Event&        releaseEvent) const = 0;

protected:
    Memory(
            const std::shared_ptr<Context>& context);

    /* Memory objects must be non-copyable */
    Memory(
            const Memory& rhs) = delete;
    Memory& operator=(
            const Memory&) = delete;

    std::shared_ptr<Context> _context; //!< Context associated with this memory object
};

/* ****************************************************************************/

class Buffer: public dcl::Buffer, public Memory {
public:
    /*!
     * \brief Creates an uninitialized buffer.
     */
    Buffer(
            const std::shared_ptr<Context>& context,
            cl_mem_flags                    flags,
            size_t                          size,
            void *                          ptr);
    virtual ~Buffer();

    operator cl::Memory() const;
    operator cl::Buffer() const;

    void acquire(
            dcl::Process&           process,
            const cl::CommandQueue& commandQueue,
            const cl::Event&        releaseEvent,
            cl::Event *             acquireEvent);

    void release(
            dcl::Process&           process,
            const cl::CommandQueue& commandQueue,
            const cl::Event&        releaseEvent) const;

private:
    cl::Buffer _buffer; //!< Native buffer
};

} /* namespace dcld */

#endif /* MEMORY_H_ */
