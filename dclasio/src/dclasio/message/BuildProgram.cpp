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
 * \file BuildProgram.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dcl/DCLTypes.h>

#include <dclasio/message/BuildProgram.h>
#include <dclasio/message/Request.h>

#include <string>
#include <vector>

namespace dclasio {
namespace message {

BuildProgram::BuildProgram() :
		_programId(0) {
}

BuildProgram::BuildProgram(
		dcl::object_id programId,
		const std::vector<dcl::object_id>& deviceIds,
		const std::string& options,
		dcl::object_id programBuildId) :
	_programId(programId), _deviceIds(deviceIds), _options(options),
            _programBuildId(programBuildId)
{
}

BuildProgram::BuildProgram(const BuildProgram& rhs) :
	Request(rhs), _programId(rhs._programId), _deviceIds(
			rhs._deviceIds), _options(rhs._options) {
}

dcl::object_id BuildProgram::programId() const {
	return _programId;
}

const std::vector<dcl::object_id>& BuildProgram::deviceIds() const {
	return _deviceIds;
}

const std::string& BuildProgram::options() const {
	return _options;
}

dcl::object_id BuildProgram::programBuildId() const {
    return _programBuildId;
}

} /* namespace message */
} /* namespace dclasio */
