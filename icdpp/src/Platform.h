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
 * @file Platform.h
 *
 * @date 2011-04-12
 * @author Philipp Kegel
 */

#ifndef CL_PLATFORM_H_
#define CL_PLATFORM_H_

#include <dcl/CommunicationManager.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cstddef>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

class _cl_platform_id {
public:
	inline static cl_platform_id dOpenCL() {
		static _cl_platform_id dOpenCL(
		        "FULL_PROFILE",
		        "OpenCL 1.1",
		        "dOpenCL",
		        "University of Muenster",
		        "cl_wwu_dcl cl_wwu_collective");
		return &dOpenCL;
	}

	static void get(
	        std::vector<cl_platform_id>& platforms);


	virtual ~_cl_platform_id();

	/**
	 * @brief Creates a compute node.
	 *
	 * The compute node is added to this platform and will be returned by
	 * getComputeNodes.
	 *
	 * @param[in]  url          the compute node's URL
	 * @param[in]  pfn_notify   A callback function that can be registered by the application.
	 *             This callback function will be used by the OpenCL implementation to report information on errors that occur during communication with this compute node.
	 *             This callback function may be called asynchronously by the OpenCL implementation.
	 *             It is the application's responsibility to ensure that the callback function is thread-safe.
	 *             If pfn_notify is NULL, no callback function is registered.
	 * @param[in]  user_data
	 * @return a compute node
	 */
	cl_compute_node_WWU createComputeNode(
    		const std::string&  url,
            void (*pfn_notify)(
                    cl_compute_node_WWU compute_node,
                    cl_int              errcode,
                    void *              user_data) = 0,
            void *user_data = nullptr);

    /**
     * @brief Destroys a compute node.
     *
     * The compute node is removed from the platform. It will not be returned by
     * a subsequent call of getComputeNodes.
     *
     * This method is reserved for internal use. Compute nodes should be
     * released by an application using _cl_compute_node::release.
     *
     * @param[in]  computeNode  the compute node that should be remove from the platform
     */
    void destroyComputeNode(
            cl_compute_node_WWU computeNodes);

	/**
	 * @brief Obtain a list of all compute nodes.
	 *
	 * @param[out] computeNodes a list of compute nodes
	 */
	void getComputeNodes(
			std::vector<cl_compute_node_WWU>& computeNodes);
	void getDevices(
			cl_device_type              deviceType,
			std::vector<cl_device_id>&  devices);
	void getInfo(
			cl_platform_info    param_name,
			size_t              param_value_size,
			void *              param_value,
			size_t *            param_value_size_ret) const;

	void unloadCompiler() const;

    /**
     * @brief Returns a reference to the remote platform instance.
     */
    dcl::HostCommunicationManager& remote() const;

private:
	_cl_platform_id(
			const std::string&  profile,
			const std::string&  version,
			const std::string&  name,
			const std::string&  vendor,
			const std::string&  extensions);

	/* Platforms must be non-copyable */
	_cl_platform_id(
	        const _cl_platform_id& rhs) = delete;
	_cl_platform_id& operator=(
	        const _cl_platform_id& rhs) = delete;

	/**
	 * @brief Initializes static compute nodes.
	 *
	 * This method must only be called while _computeNodesMutex is locked.
	 */
	void initComputeNodes();


	std::string _profile;
	std::string _version;
	std::string _name;
	std::string _vendor;
	std::string _extensions;

    std::unique_ptr<dcl::HostCommunicationManager> _communicationManager;

    /** Indicates if static compute nodes have been added to this platform */
    bool _computeNodesInitialized;
    /* TODO Use std::set<std::unique_ptr<_cl_compute_node_WWU>> to store a platform's compute nodes */
	/** The compute nodes that are managed by this platform. */
	std::set<cl_compute_node_WWU> _computeNodes;
	mutable std::mutex _computeNodesMutex;
};

#endif /* CL_PLATFORM_H_ */
