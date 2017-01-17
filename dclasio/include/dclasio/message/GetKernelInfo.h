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
 * \file GetKernelInfo.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef GETKERNELINFO_H_
#define GETKERNELINFO_H_

#include "Request.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclasio {
namespace message {

class GetKernelInfo: public Request {
public:
    GetKernelInfo();
    GetKernelInfo(dcl::object_id kernelId, cl_kernel_info paramName);
    GetKernelInfo(const GetKernelInfo& rhs);
    virtual ~GetKernelInfo();

    dcl::object_id kernelId() const;
    cl_kernel_info paramName() const;

    static const class_type TYPE = 100 + GET_KERNEL_INFO;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _kernelId << _paramName;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _kernelId >> _paramName;
    }

private:
    dcl::object_id _kernelId;
    cl_kernel_info _paramName;
};

/* ****************************************************************************/

class GetKernelWorkGroupInfo: public Request {
public:
    GetKernelWorkGroupInfo();
    GetKernelWorkGroupInfo(
            dcl::object_id kernelId,
            dcl::object_id deviceId,
            cl_kernel_work_group_info paramName);
    GetKernelWorkGroupInfo(const GetKernelWorkGroupInfo& rhs);
    virtual ~GetKernelWorkGroupInfo();

    dcl::object_id kernelId() const;
    dcl::object_id deviceId() const;
    cl_kernel_work_group_info paramName() const;

    static const class_type TYPE = 100 + GET_KERNEL_WORK_GROUP_INFO;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _kernelId << _deviceId << _paramName;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _kernelId >> _deviceId >> _paramName;
    }
private:
    dcl::object_id _kernelId;
    dcl::object_id _deviceId;
    cl_kernel_work_group_info _paramName;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* GETKERNELINFO_H_ */
