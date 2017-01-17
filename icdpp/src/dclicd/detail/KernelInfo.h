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
 * \file KernelInfo.h
 *
 * \date 2013-10-26
 * \author Philipp Kegel
 */

#ifndef KERNELINFO_H_
#define KERNELINFO_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <string>

namespace dclicd {

namespace detail {

class KernelInfo {
public:
    KernelInfo(
            const std::string& functionName_,
            cl_uint            numArgs_,
            const std::string *_attributes = nullptr);

    const std::string& functionName;
    cl_uint numArgs;

    /* OpenCL 1.2 kernel info */
    const std::string * const attributes;
};

/******************************************************************************/

class KernelWorkGroupInfo {
public:
    KernelWorkGroupInfo(
            size_t       workGroupSize_,
            const size_t compileworkGroupSize_[],
            cl_ulong     localMemSize_,
            size_t       preferredWorkGroupSizeMultiple_,
            cl_ulong     privateMemSize_,
            const size_t globalWorkSize_[] = nullptr);

private:
    size_t workGroupSize;
    const size_t *compileworkGroupSize;
    cl_ulong localMemSize;
    size_t preferredWorkGroupSizeMultiple;
    cl_ulong privateMemSize;

    /* OpenCL 1.2 kernel work-group info */
    const size_t *globalWorkSize;
};

} /* namespace detail */

} /* namespace dclicd */

#endif /* KERNELINFO_H_ */
