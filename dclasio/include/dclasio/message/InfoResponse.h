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
 * \file InfoResponse.h
 *
 * \date 2011-05-19
 * \author Philipp Kegel
 */

#ifndef INFORESPONSE_H_
#define INFORESPONSE_H_

#include "Response.h"

#include <dcl/Binary.h>
#include <dcl/ByteBuffer.h>

#include <cstddef>

namespace dclasio {
namespace message {

class Request;

/* ****************************************************************************/

/*!
 * \brief A response message containing a single piece of information on any type of OpenCL object.
 */
class InfoResponse: public DefaultResponse {
public:
    InfoResponse();
	InfoResponse(
			const Request& request,
			size_t         size,
			const void *   value);
	InfoResponse(
	        const InfoResponse& rhs);
	virtual ~InfoResponse();

    dcl::Binary param() const;

    static const class_type TYPE = 298;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        DefaultResponse::pack(buf);
        buf << _param;
    }

    void unpack(dcl::ByteBuffer& buf) {
        DefaultResponse::unpack(buf);
        buf >> _param;
    }

private:
    dcl::Binary _param;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* INFORESPONSE_H_ */
