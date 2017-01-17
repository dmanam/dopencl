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
 * \file    Device.h
 *
 * \date    2011-11-12
 * \author  Philipp Kegel
 */

#ifndef DCL_DEVICE_H_
#define DCL_DEVICE_H_

#include "DCLTypes.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dcl {

class Binary;
class ComputeNode;

/* ****************************************************************************/

/*!
 * \brief Remote interface for a device
 */
class Device {
public:
    virtual ~Device() { }

    virtual void getInfo(
            cl_device_info param_name,
            Binary&        param) const = 0;

    /* TODO Delete dcl::Device::getId when all remote ICD objects are accessible through interface classes. */
    virtual object_id getId() const = 0;
    /* TODO Delete dcl::Device::getComputeNode when all remote ICD objects are accessible through interface classes. */
    virtual ComputeNode& getComputeNode() const = 0;
};

} /* namespace dcl */

#endif /* DCL_DEVICE_H_ */
