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
 * \file CreateProgramWithBinary.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef CREATEPROGRAMWITHBINARY_H_
#define CREATEPROGRAMWITHBINARY_H_

#include "Request.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <cstddef>
#include <vector>

namespace dclasio {
namespace message {

class CreateProgramWithBinary: public Request {
public:
	CreateProgramWithBinary(
			const dcl::object_id                programId,
			const dcl::object_id                contextId,
			const std::vector<dcl::object_id>&  deviceIds,
			const std::vector<size_t>&          lengths);
	CreateProgramWithBinary(
	        const CreateProgramWithBinary& rhs);
	virtual ~CreateProgramWithBinary();

	dcl::object_id programId() const;
	dcl::object_id contextId() const;
	const std::vector<dcl::object_id>& deviceIds() const;
	const std::vector<size_t>& lengths() const;

    static const class_type TYPE = 100 + CREATE_PROGRAM_WITH_BINARY;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _programId << _contextId << _deviceIds << _lengths;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _programId >> _contextId >> _deviceIds >> _lengths;
    }

private:
	dcl::object_id _programId;
	dcl::object_id _contextId;
	std::vector<dcl::object_id> _deviceIds;
	std::vector<size_t> _lengths;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* CREATEPROGRAMWITHBINARY_H_ */
