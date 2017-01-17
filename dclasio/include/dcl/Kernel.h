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
 * \file    Kernel.h
 *
 * \date    2012-08-05
 * \author  Philipp Kegel
 *
 * dOpenCL kernel API
 */

#ifndef DCL_KERNEL_H_
#define DCL_KERNEL_H_

#include <cstddef>
#include <memory>

namespace dcl {

class Binary;
class Device;
class Memory;

/* ****************************************************************************/

/*!
 * \brief Remote interface of a kernel.
 */
class Kernel {
public:
    virtual ~Kernel() { }

    virtual void getInfo(
            cl_kernel_info  param_name,
            Binary&         param_value) const = 0;
    virtual void getWorkGroupInfo(
            const Device *              device,
            cl_kernel_work_group_info   param_name,
            Binary&                     param_value) const = 0;

    virtual void setArg(
            cl_uint                         index,
            const std::shared_ptr<Memory>&  memoryObject) = 0;
    virtual void setArg(
            cl_uint     index,
            size_t      size,
            const void *argPtr = nullptr) = 0;

};

} /* namespace dcl */

#endif /* DCL_KERNEL_H_ */
