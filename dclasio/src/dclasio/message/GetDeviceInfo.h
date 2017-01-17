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
 * \file GetDeviceInfo.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef GETDEVICEINFO_H_
#define GETDEVICEINFO_H_

#include <dclasio/message/Request.h>

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclasio {

namespace message {

/*!
 * \brief A message for requesting a single piece of device information.
 */
class GetDeviceInfo: public Request {
public:
    GetDeviceInfo() :
            deviceId(0), paramName(CL_DEVICE_NAME) {
    }
    GetDeviceInfo(dcl::object_id deviceId_, cl_device_info paramName_) :
            deviceId(deviceId_), paramName(paramName_) {
    }
    GetDeviceInfo(const GetDeviceInfo& rhs) :
            Request(rhs), deviceId(rhs.deviceId), paramName(rhs.paramName) {
    }
    virtual ~GetDeviceInfo() {
    }

    dcl::object_id deviceId;
    cl_device_info paramName;

    static const class_type TYPE = 100 + GET_DEVICE_INFO;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << deviceId << paramName;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> deviceId >> paramName;
    }
};

} /* namespace message */

} /* namespace dclasio */

#endif /* GETDEVICEINFO_H_ */
