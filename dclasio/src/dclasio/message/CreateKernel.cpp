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
 * \file CreateKernel.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/CreateKernel.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#include <cassert>
#include <string>

namespace dclasio {
namespace message {

CreateKernel::CreateKernel() :
	_kernelId(0), _programId(0) {
}

CreateKernel::CreateKernel(
		dcl::object_id kernelId,
		dcl::object_id programId,
		const char *kernelName) :
	_kernelId(kernelId), _programId(programId), _kernelName(kernelName) {
    assert(kernelName != nullptr && "Kernel name not specified");
}

CreateKernel::CreateKernel(const CreateKernel& rhs) :
	Request(rhs), _kernelId(rhs._kernelId), _programId(rhs._programId),
			_kernelName(rhs._kernelName) {
}

dcl::object_id CreateKernel::kernelId() const {
	return _kernelId;
}

dcl::object_id CreateKernel::programId() const {
	return _programId;
}

const char * CreateKernel::kernelName() const {
	return _kernelName.c_str();
}

} /* namespace message */
} /* namespace dclasio */
