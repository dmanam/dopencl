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
 * \file InfoResponse.cpp
 *
 * \date 2011-05-19
 * \author Philipp Kegel
 */

#include <dclasio/message/InfoResponse.h>
#include <dclasio/message/Response.h>
#include <dclasio/message/Request.h>

#include <dcl/Binary.h>

#include <cstddef>

namespace dclasio {
namespace message {

InfoResponse::InfoResponse() {
}

InfoResponse::InfoResponse(
		const Request& request,
		size_t size,
		const void *value) :
		DefaultResponse(request), _param(size, value) {
}

InfoResponse::InfoResponse(
		const InfoResponse& rhs) :
		DefaultResponse(rhs), _param(rhs._param) {
}

InfoResponse::~InfoResponse() { }

dcl::Binary InfoResponse::param() const {
    return _param;
}

} /* namespace message */
} /* namespace dclasio */
