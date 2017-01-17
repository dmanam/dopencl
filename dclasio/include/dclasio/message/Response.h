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
 * \file Response.h
 *
 * \date 2011-04-08
 * \author Philipp Kegel
 */

#ifndef RESPONSE_H_
#define RESPONSE_H_

#include "Message.h"
#include "Request.h"

#include <dcl/ByteBuffer.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclasio {

namespace message {

/*!
 * \brief Abstract response message
 */
class Response : public Message {
public:
    Response();
    Response(const Request& request);
    Response(const Response& rhs);
    virtual ~Response();

    Request::id_type get_request_id() const;

    virtual cl_int get_errcode() const = 0;

    void pack(dcl::ByteBuffer& buf) const {
        buf << _requestId;
    }

    void unpack(dcl::ByteBuffer& buf) {
        buf >> _requestId;
    }

private:
    Request::id_type _requestId;
};

/* ****************************************************************************/

/*!
 * \brief The default response type for successfully executed requests
 */
class DefaultResponse : public Response {
public:
    DefaultResponse();
    DefaultResponse(
            const Request& request);
    DefaultResponse(
            const DefaultResponse& rhs);
    virtual ~DefaultResponse();

    cl_int get_errcode() const;

    static const class_type TYPE = 200;

    class_type get_type() const {
        return TYPE;
    }
};

} /* namespace message */

} /* namespace dclasio */

#endif /* RESPONSE_H_ */
