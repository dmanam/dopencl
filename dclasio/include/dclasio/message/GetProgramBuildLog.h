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
 * \file GetProgramBuildLog.h
 *
 * \date 2014-04-06
 * \author Philipp Kegel
 */

#ifndef GETPROGRAMBUILDLOG_H_
#define GETPROGRAMBUILDLOG_H_

#include "Request.h"

#include <dcl/DCLTypes.h>

#include <cstddef>

namespace dclasio {
namespace message {

/*!
 * \brief Query a program's build log.
 * Other build info is provided locally.
 */
class GetProgramBuildLog: public Request {
public:
	/*!
	 * \brief Creates a request for a program's build log
	 *
	 * \param[in]  programId
	 * \param[in]  deviceId
	 * \param[in]  size         maximum size of the build log to return
	 * An error will be returned by the compute node if the build log is longer this size.
	 * A value of 0 indicates that no build log should be returned.
	 */
	GetProgramBuildLog(
			const dcl::object_id    programId,
			const dcl::object_id    deviceId,
			size_t                  size);
	GetProgramBuildLog(
	        const GetProgramBuildLog& rhs);
	virtual ~GetProgramBuildLog();

	dcl::object_id programId() const;
	dcl::object_id deviceId() const;
	size_t size() const;

    static const value_type TYPE = 100 + GET_PROGRAM_BUILD_LOG;

    value_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _programId << _deviceId << _size;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _programId >> _deviceId >> _size;
    }

private:
	dcl::object_id _programId;
	dcl::object_id _deviceId;
	size_t _size;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* GETPROGRAMBUILDLOG_H_ */
