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
 * \file Program.h
 *
 * \date 2012-07-30
 * \author Philipp Kegel
 */

#ifndef PROGRAM_H_
#define PROGRAM_H_

#include <dcl/Device.h>
#include <dcl/Kernel.h>
#include <dcl/Program.h>
#include <dcl/ProgramBuildListener.h>

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

class Context;

/* ****************************************************************************/

/*!
 * \brief A decorator for a native program.
 *
 * This wrapper is required to receive program source or binaries from a host
 * and for notifying program build listeners about completed programs builds.
 */
class Program: public dcl::Program {
public:
    Program(
            const std::shared_ptr<Context>& context,
            const char *                    source,
            size_t                          length);
    Program(
            const std::shared_ptr<Context>&     context,
            const std::vector<dcl::Device *>&   devices,
            const std::vector<size_t>&          lengths,
            const unsigned char **              binaries,
            VECTOR_CLASS<cl_int> *              binary_status);
    virtual ~Program();

    operator cl::Program() const;

    void build(
            const std::vector<dcl::Device *>&                   devices,
            const char *                                        options,
            const std::shared_ptr<dcl::ProgramBuildListener>&   programBuildListener);

    void createKernels(
            std::vector<std::shared_ptr<dcl::Kernel>>& kernels);

    const std::shared_ptr<Context>& context() const;

private:
    /* Programs must be non-copyable */
    Program(
            const Program& rhs) = delete;
    Program& operator=(
            const Program& rhs) = delete;

    std::shared_ptr<Context> _context; //!< Context associated with program

    cl::Program _program; //!< Native program
};

} /* namespace dcld */

#endif /* PROGRAM_H_ */
