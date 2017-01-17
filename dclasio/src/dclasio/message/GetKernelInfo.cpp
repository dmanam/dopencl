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
 * \file GetKernelInfo.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/GetKernelInfo.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

/* ****************************************************************************
 * Get kernel info
 ******************************************************************************/

namespace dclasio {
namespace message {

GetKernelInfo::GetKernelInfo() {
}

GetKernelInfo::GetKernelInfo(dcl::object_id kernelId, cl_kernel_info paramName) :
        _kernelId(kernelId), _paramName(paramName) {
}

GetKernelInfo::GetKernelInfo(const GetKernelInfo& rhs) :
        Request(rhs), _kernelId(rhs._kernelId), _paramName(rhs._paramName) {
}

GetKernelInfo::~GetKernelInfo() {
}

dcl::object_id GetKernelInfo::kernelId() const {
    return _kernelId;
}

cl_kernel_info GetKernelInfo::paramName() const {
    return _paramName;
}

} /* namespace message */
} /* namespace dclasio */

/* ****************************************************************************
 * Get kernel work group info
 ******************************************************************************/

namespace dclasio {
namespace message {

GetKernelWorkGroupInfo::GetKernelWorkGroupInfo() {
}

GetKernelWorkGroupInfo::GetKernelWorkGroupInfo(
        dcl::object_id kernelId,
        dcl::object_id deviceId,
        cl_kernel_work_group_info paramName) :
        _kernelId(kernelId), _deviceId(deviceId), _paramName(paramName) {
}

GetKernelWorkGroupInfo::GetKernelWorkGroupInfo(
        const GetKernelWorkGroupInfo& rhs) :
        Request(rhs), _kernelId(rhs._kernelId), _deviceId(rhs._deviceId), _paramName(
                rhs._paramName) {
}

GetKernelWorkGroupInfo::~GetKernelWorkGroupInfo() {
}

dcl::object_id GetKernelWorkGroupInfo::kernelId() const {
    return _kernelId;
}

dcl::object_id GetKernelWorkGroupInfo::deviceId() const {
    return _deviceId;
}

cl_kernel_info GetKernelWorkGroupInfo::paramName() const {
    return _paramName;
}

} /* namespace message */
} /* namespace dclasio */
