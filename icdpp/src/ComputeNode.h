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

/**
 * @file ComputeNode.h
 *
 * @date 2011-04-06
 * @author Philipp Kegel
 */

#ifndef CL_COMPUTENODE_H_
#define CL_COMPUTENODE_H_

#include "Platform.h"
#include "Retainable.h"

#include <dcl/ComputeNode.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cstddef>
#include <memory>
#include <vector>


class _cl_compute_node_WWU :
	public _cl_retainable {
	friend class _cl_platform_id;

public:
	void getDevices(
			cl_device_type,
			std::vector<cl_device_id>&) const;
	void getInfo(
			cl_compute_node_info_WWU    param_name,
			size_t                      param_value_size,
			void *                      param_value,
			size_t *                    param_value_size_ret) const;

    /**
     * @brief Returns a reference to the remote compute node instance.
     */
    /* TODO Hide method _cl_compute_node_id_WWU::remote to not break API design */
    dcl::ComputeNode& remote() const;

protected:
    void destroy();

private:
	/**
     * @brief Create a compute node.
     *
     * This method must not be called directly.
     * Use \code ComputeNodeManager::createComputeNode instead.
     *
     * @param[in]  platform
     * @param[in]  remote       the remote compute node instance
	 * @param[in]  pfn_notify   A callback function that can be registered by the application.
	 *             This callback function will be used by the OpenCL implementation to report information on errors that occur during communication with this compute node.
	 *             This callback function may be called asynchronously by the OpenCL implementation.
	 *             It is the application's responsibility to ensure that the callback function is thread-safe.
	 *             If pfn_notify is NULL, no callback function is registered.
	 * @param[in]  user_data
     *
     * @see ComputeNodeManager::createComputeNode
     */
    _cl_compute_node_WWU(
    		cl_platform_id              platform,
    		dcl::ComputeNode&           remote,
            void (*                     pfn_notify)(
                    cl_compute_node_WWU compute_node,
                    cl_int              errcode,
                    void *              user_data) = nullptr,
            void *user_data = nullptr);
	virtual ~_cl_compute_node_WWU();

	/* Compute nodes must be non-copyable */
	_cl_compute_node_WWU(
	        const _cl_compute_node_WWU&) = delete;
	_cl_compute_node_WWU& operator=(
	        const _cl_compute_node_WWU&) = delete;

	/**
	 * @brief Initializes this compute node's device list.
	 *
	 * Queries valid devices from the compute node proxies and creates
	 * application level device objects for each proxy.
	 *
	 * This method must only be called once.
	 */
	void initDeviceList();

	cl_platform_id _platform;
	void (*_pfnNotify)(
					cl_compute_node_WWU compute_node,
					cl_int              errcode,
				 	void *              user_data);
	void *_userData;
	std::vector<std::unique_ptr<struct _cl_device_id>> _devices;

	dcl::ComputeNode& _remote; /**< remote compute node instance */
};

#endif /* CL_COMPUTENODE_H_ */
