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
 * \file BuildProgram.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef BUILDPROGRAM_H_
#define BUILDPROGRAM_H_

#include "Request.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <vector>
#include <string>

namespace dclasio {
namespace message {

class BuildProgram: public Request {
public:
    BuildProgram();
	BuildProgram(
			dcl::object_id                      programId,
			const std::vector<dcl::object_id>&  deviceIds,
			const std::string&                  options,
            dcl::object_id                      programBuildId);
	BuildProgram(
	        const BuildProgram& rhs);

	dcl::object_id programId() const;
	const std::vector<dcl::object_id>& deviceIds() const;
	const std::string& options() const;
    dcl::object_id programBuildId() const;

    static const class_type TYPE = 100 + BUILD_PROGRAM;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _programId << _deviceIds << _options << _programBuildId;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _programId >> _deviceIds >> _options >> _programBuildId;
    }

private:
	dcl::object_id _programId;
	std::vector<dcl::object_id> _deviceIds;
	std::string _options;
    dcl::object_id _programBuildId;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* BUILDPROGRAM_H_ */
