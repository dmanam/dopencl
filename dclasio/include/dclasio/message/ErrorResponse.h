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
 * \file ErrorResponse.h
 *
 * \date 2014-04-04
 * \author Philipp Kegel
 */

#ifndef ERRORRESPONSE_H_
#define ERRORRESPONSE_H_

#include "Response.h"

#include <dcl/ByteBuffer.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclasio {
namespace message {

class Request;

/* ****************************************************************************/

class ErrorResponse: public Response {
public:
    ErrorResponse();
    ErrorResponse(const Request& request, cl_int errcode);
    ErrorResponse(const ErrorResponse& rhs);
    virtual ~ErrorResponse();

    cl_int get_errcode() const;

    /* TODO Replace message ID */
    static const class_type TYPE = 299;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Response::pack(buf);
        buf << _errcode;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Response::unpack(buf);
        buf >> _errcode;
    }

private:
    cl_int _errcode;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* ERRORRESPONSE_H_ */
