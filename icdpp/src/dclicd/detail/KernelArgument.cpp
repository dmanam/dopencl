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
 * \file    KernelArgument.cpp
 *
 * \date    2013-10-26
 * \author  Philipp Kegel
 */

#include "KernelArgument.h"

#include "../../Memory.h"

#include <dcl/Binary.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cassert>
#include <cstddef>

namespace dclicd {

namespace detail {

KernelArgument::KernelArgument(
        size_t size_) : type(dcl::kernel_arg_type::MEMORY), size(size_) { }
KernelArgument::KernelArgument(
        cl_mem mem) : type(dcl::kernel_arg_type::MEMORY), size(sizeof(cl_mem)) {
    assert(mem != nullptr);
    dcl::object_id memId = mem->remoteId();
    value.assign(sizeof(mem), &memId);
}
KernelArgument::KernelArgument(size_t size_, const void *value_) :
        type(dcl::kernel_arg_type::BINARY), size(size_), value(size_, value_) { }

} /* namespace detail */

} /* namespace dclicd */
