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
 * \file DeviceIDsResponse.h
 *
 * \date 2014-04-04
 * \author Philipp Kegel
 */

#ifndef DEVICEIDSRESPONSE_H_
#define DEVICEIDSRESPONSE_H_

#include <dclasio/message/Response.h>
#include <dclasio/message/Request.h>

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <vector>

namespace dclasio {

namespace message {

/*!
 * \brief Response message containing a list of device IDs
 */
class DeviceIDsResponse: public DefaultResponse {
public:
    DeviceIDsResponse() {
    }
    DeviceIDsResponse(
            const Request& request,
            const std::vector<dcl::object_id>& deviceIds_) :
            DefaultResponse(request), deviceIds(deviceIds_) {
    }
    DeviceIDsResponse(const DeviceIDsResponse& rhs) :
            DefaultResponse(rhs), deviceIds(rhs.deviceIds) {
    }
    ~DeviceIDsResponse() {
    }

    std::vector<dcl::object_id> deviceIds;

    static const class_type TYPE = 200 + Request::GET_DEVICE_IDS;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        DefaultResponse::pack(buf);
        buf << deviceIds;
    }

    void unpack(dcl::ByteBuffer& buf) {
        DefaultResponse::unpack(buf);
        buf >> deviceIds;
    }
};

} /* namespace message */

} /* namespace dclasio */

#endif /* DEVICEIDSRESPONSE_H_ */
