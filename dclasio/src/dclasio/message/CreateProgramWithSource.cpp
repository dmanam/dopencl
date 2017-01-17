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
 * \file CreateProgramWithSource.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/CreateProgramWithSource.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#include <cstddef>

namespace dclasio {
namespace message {

CreateProgramWithSource::CreateProgramWithSource(
		dcl::object_id programId,
		dcl::object_id contextId,
		size_t length) :
	_programId(programId), _contextId(contextId), _length(length) {
}

CreateProgramWithSource::CreateProgramWithSource(
		const CreateProgramWithSource& rhs) :
	Request(rhs), _programId(rhs._programId), _contextId(rhs._contextId),
			_length(rhs._length) {
}

CreateProgramWithSource::CreateProgramWithSource() :
	_programId(0), _contextId(0), _length(0) {
}

CreateProgramWithSource::~CreateProgramWithSource() { }

dcl::object_id CreateProgramWithSource::programId() const {
	return _programId;
}

dcl::object_id CreateProgramWithSource::contextId() const {
	return _contextId;
}

size_t CreateProgramWithSource::length() const {
	return _length;
}

} /* namespace message */
} /* namespace dclasio */
