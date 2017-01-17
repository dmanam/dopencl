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
 * \file DeviceImpl.h
 *
 * \date 2011-11-12
 * \author Philipp Kegel
 */

#ifndef DEVICEIMPL_H_
#define DEVICEIMPL_H_

#include "ComputeNodeImpl.h"

#include <dcl/Binary.h>
#include <dcl/ComputeNode.h>
#include <dcl/DCLTypes.h>
#include <dcl/Device.h>
#include <dcl/Remote.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclasio {

class DeviceImpl: public dcl::Device, public dcl::Remote {
public:
	DeviceImpl(
            dcl::object_id      id,
            ComputeNodeImpl&    computeNode);
	virtual ~DeviceImpl();

	void getInfo(
            cl_device_info  param_name,
            dcl::Binary&    param) const;

    /* TODO Remove dclasio::DeviceImpl::getId */
    dcl::object_id getId() const { return _id; }
    /* TODO Remove dclasio::DeviceImpl::getComputeNode */
    dcl::ComputeNode& getComputeNode() const { return _computeNode; }

	/*!
	 * \brief Returns the compute node associated with this device.
	 *
	 * \return a compute node
	 */
	ComputeNodeImpl& computeNode() const;

private:
	ComputeNodeImpl& _computeNode;
};

} /* namespace dclasio */

#endif /* DEVICEIMPL_H_ */
