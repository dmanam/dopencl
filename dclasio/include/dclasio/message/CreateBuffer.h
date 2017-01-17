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
 * \file CreateBuffer.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef CREATEBUFFER_H_
#define CREATEBUFFER_H_

#include "Request.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>

namespace dclasio {
namespace message {

class CreateBuffer : public Request {
public:
    CreateBuffer();
	CreateBuffer(
			dcl::object_id bufferId,
			dcl::object_id contextId,
			cl_mem_flags flags,
			size_t size);
	CreateBuffer(
	        const CreateBuffer& rhs);

	dcl::object_id bufferId() const;
	dcl::object_id contextId() const;
	cl_mem_flags flags() const;
	size_t size() const;

    static const class_type TYPE = 100 + CREATE_BUFFER;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _bufferId << _contextId << _flags << _size;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _bufferId >> _contextId >> _flags >> _size;
    }

private:
	dcl::object_id _bufferId;
	dcl::object_id _contextId;
	cl_mem_flags _flags;
	size_t _size;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* CREATEBUFFER_H_ */
