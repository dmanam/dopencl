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
 * \file    DeviceInfosResponse.h
 *
 * \date    2011-05-19
 * \author  Philipp Kegel
 */

#ifndef DEVICEINFOSRESPONSE_H_
#define DEVICEINFOSRESPONSE_H_

#include <dclasio/message/Response.h>
#include <dclasio/message/Request.h>

#include <dcl/Binary.h>
#include <dcl/ByteBuffer.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <map>

namespace dclasio {

namespace message {

/*!
 * \brief A response message containing a set of device information.
 *
 * Unlike InfoResponse, this class is designed to provide a set of information
 * items rather than a single item.
 *
 * !!! This message class is currently not used. !!!
 */
class DeviceInfosResponse: public DefaultResponse {
public:
    DeviceInfosResponse();
    DeviceInfosResponse(
            const Request& request,
            const std::map<cl_device_info, const dcl::Binary>& params);
    DeviceInfosResponse(
            const Request& request,
            cl_device_info paramName,
            size_t size,
            const void * value);
    DeviceInfosResponse(
            const DeviceInfosResponse& rhs);
    virtual ~DeviceInfosResponse();

    dcl::Binary get_param(cl_device_info paramName) const;
    const std::map<cl_device_info, const dcl::Binary>& get_params() const;

    static const class_type TYPE = 200 + Request::GET_DEVICE_INFO;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        DefaultResponse::pack(buf);
        buf << _params;
    }

    void unpack(dcl::ByteBuffer& buf) {
        DefaultResponse::unpack(buf);
        buf >> _params;
    }

private:
    std::map<cl_device_info, const dcl::Binary> _params;
};

} /* namespace message */

} /* namespace dclasio */

#endif /* DEVICEINFOSRESPONSE_H_ */
