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
 * \file Kernel.h
 *
 * \date 2011-12-09
 * \author Philipp Kegel
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include <dcl/Binary.h>
#include <dcl/Device.h>
#include <dcl/Kernel.h>
#include <dcl/Memory.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <cstddef>
#include <memory>
#include <vector>

namespace dcld {

class Memory;
class Program;

/* ****************************************************************************/

/*!
 * \brief A decorator for a native kernel.
 *
 * This wrapper is required to store memory objects which are passed to a
 * kernel as arguments.
 */
class Kernel: public dcl::Kernel {
public:
    Kernel(
            const std::shared_ptr<Program>& program,
            const char *                    name);
    Kernel(
            const cl::Kernel& kernel);
    virtual ~Kernel();

    /*!
     * \brief Returns the wrapped OpenCL kernel.
     */
    operator cl::Kernel() const;

    void getInfo(
            cl_kernel_info  param_name,
            dcl::Binary&    param) const;
    void getWorkGroupInfo(
            const dcl::Device *         device,
            cl_kernel_work_group_info   param_name,
            dcl::Binary&                param) const;

    void setArg(
            cl_uint                             index,
            const std::shared_ptr<dcl::Memory>& memoryObject);
    void setArg(
            cl_uint            index,
            const cl::Sampler& sampler);
    void setArg(
            cl_uint     index,
            size_t      size,
            const void *argPtr = nullptr);

    /*!
     * \brief Returns the memory objects (possibly) written to with this kernel
     *
     * \return a list of memory objects
     */
    std::vector<std::shared_ptr<Memory>> writeMemoryObjects();

private:
    /* Kernels must be non-copyable */
    Kernel(
            const Kernel& rhs) = delete;
    Kernel& operator=(
            const Kernel& rhs) = delete;

    cl::Kernel _kernel; //!< Native kernel

    std::vector<std::shared_ptr<Memory>> _writeMemoryObjects; //!< Memory objects used by this kernel
};

} /* namespace dcld */

#endif /* KERNEL_H_ */
