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
 * \file CreateContext.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/CreateContext.h>
#include <dclasio/message/Request.h>

#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cassert>
#include <cstddef>
#include <vector>

namespace dclasio {

namespace message {

CreateContext::CreateContext() :
		 _contextId(0) {
}

CreateContext::CreateContext(
        dcl::object_id contextId,
      const std::vector<dcl::process_id>& computeNodeIds) :
    _contextId(contextId), _computeNodeIds(computeNodeIds),
    _deviceType(CL_DEVICE_TYPE_ALL) {
}

CreateContext::CreateContext(
        dcl::object_id contextId,
      const std::vector<dcl::process_id>& computeNodeIds,
        cl_device_type deviceType) :
    _contextId(contextId), _computeNodeIds(computeNodeIds),
    _deviceType(deviceType) {
}

CreateContext::CreateContext(
		dcl::object_id contextId,
		const std::vector<dcl::process_id>& computeNodeIds,
		const std::vector<dcl::object_id>& deviceIds) :
	_contextId(contextId), _computeNodeIds(computeNodeIds),
    _deviceType(CL_DEVICE_TYPE_ALL), _deviceIds(deviceIds) {
}

CreateContext::CreateContext(const CreateContext& rhs) :
    Request(rhs), _contextId(rhs._contextId),
    _computeNodeIds(rhs._computeNodeIds),
    _deviceType(rhs._deviceType), _deviceIds(rhs._deviceIds)
{
}

dcl::object_id CreateContext::contextId() const {
	return _contextId;
}

const std::vector<dcl::process_id>& CreateContext::computeNodeIds() const {
    return _computeNodeIds;
}

cl_device_type CreateContext::deviceType() const {
    return _deviceType;
}

const std::vector<dcl::object_id>& CreateContext::deviceIds() const {
	return _deviceIds;
}

} /* namespace message */
} /* namespace dclasio */
