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
 * \file CreateBuffer.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/CreateBuffer.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>

namespace dclasio {
namespace message {

CreateBuffer::CreateBuffer() {
}

CreateBuffer::CreateBuffer(
		dcl::object_id bufferId,
		dcl::object_id contextId,
		cl_mem_flags flags,
		size_t size) :
	_bufferId(bufferId), _contextId(contextId), _flags(flags), _size(size) {
}

CreateBuffer::CreateBuffer(const CreateBuffer& rhs) :
	Request(rhs), _bufferId(rhs._bufferId), _contextId(rhs._contextId),
			_flags(rhs._flags), _size(rhs._size) {
}

dcl::object_id CreateBuffer::bufferId() const {
	return _bufferId;
}

dcl::object_id CreateBuffer::contextId() const	{
	return _contextId;
}

cl_mem_flags CreateBuffer::flags() const {
	return _flags;
}

size_t CreateBuffer::size() const {
	return _size;
}

} /* namespace message */
} /* namespace dclasio */
