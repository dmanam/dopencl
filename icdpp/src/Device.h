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
 * \file Device.h
 *
 * \date 2011-04-06
 * \author Philipp Kegel
 */

#ifndef CL_DEVICE_H_
#define CL_DEVICE_H_

#include <dcl/Binary.h>
#include <dcl/Device.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cstddef>
#include <map>
#include <mutex>


class _cl_device_id {
public:
	_cl_device_id(
			cl_compute_node_WWU computeNode,
			dcl::Device&        device);
	virtual ~_cl_device_id();

	void getInfo(
            cl_device_info	param_name,
            size_t			param_value_size,
            void * 			param_value,
            size_t *		param_value_size_ret) const;

    /*!
     * \brief Returns a reference to the remote device instance.
     */
	dcl::Device& remote() const;

private:
	cl_compute_node_WWU _computeNode;

	mutable std::map<cl_device_info, const dcl::Binary> _infoCache;
	mutable std::mutex _infoCacheMutex;

	dcl::Device& _device;
};

#endif /* CL_DEVICE_H_ */
