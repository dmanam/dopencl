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
 * \file DeviceImpl.cpp
 *
 * \date 2011-11-12
 * \author Philipp Kegel
 */

#include "DeviceImpl.h"

#include "ComputeNodeImpl.h"

//#include "message/DeviceInfosResponse.h"
#include "message/GetDeviceInfo.h"

#include <dclasio/message/InfoResponse.h>

#include <dcl/Binary.h>
#include <dcl/ComputeNode.h>
#include <dcl/DCLTypes.h>
#include <dcl/Remote.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <memory>

namespace dclasio {

DeviceImpl::DeviceImpl(
		dcl::object_id id,
		ComputeNodeImpl& computeNode) :
	dcl::Remote(id), _computeNode(computeNode) {
}

DeviceImpl::~DeviceImpl() { }

void DeviceImpl::getInfo(cl_device_info param_name, dcl::Binary& param) const {
    message::GetDeviceInfo request(_id, param_name);
	std::unique_ptr<message::InfoResponse> response(
			static_cast<message::InfoResponse *>(
					_computeNode.executeCommand(request, message::InfoResponse::TYPE).release()));
	param = response->param();

    dcl::util::Logger << dcl::util::Info
            << "Got device info (ID=" << _id
            << ')' << std::endl;
}

ComputeNodeImpl& DeviceImpl::computeNode() const {
    return _computeNode;
}

} /* namespace dclasio */
