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
 * @file cl_wwu_dcl.cpp
 *
 * @date 2011-08-11
 * @author Philipp Kegel
 *
 * @brief Implementation of the OpenCL API extension specified for dOpenCL.
 *
 * Function calls of the C API are redirected to corresponding method of a C++
 * implementation. Functions in this file only perform type conversions and
 * related operations, e.g. validating list parameters that are converted into
 * vectors. The C++ methods validate parameters.
 */

#include "CommandQueue.h"
#include "ComputeNode.h"
#include "Context.h"
#include "Memory.h"
#include "Platform.h"
#include "Retainable.h"

#include "dclicd/Error.h"
#include "dclicd/utility.h"

#include "dclicd/detail/ContextProperties.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>


/**
 * @brief Initialization routine.
 */
//void __attribute__ ((constructor)) __dclInit() {
//}

/**
 * @brief Cleans up when dOpenCL is closed or the main program exits, respectively.
 */
//void __attribute__ ((destructor)) __dclFinalize() {
//}

/* Compute node APIs */

cl_compute_node_WWU clCreateComputeNodeWWU(
		cl_platform_id platform,
		const char *url,
		void (*pfn_notify)(
				cl_compute_node_WWU, cl_int, void *),
		void *user_data,
		cl_int *errcode_ret) {
	cl_compute_node_WWU computeNode = nullptr;
	cl_int errcode;

	if (platform) {
		/* TODO Validate platform */
	} else {
		/* Behavior is implementation defined, if platform is NULL */
		platform = _cl_platform_id::dOpenCL();
	}

	try {
		computeNode = platform->createComputeNode(url, pfn_notify, user_data);
		errcode = CL_SUCCESS;
	} catch (const dclicd::Error& err) {
		errcode = err.err();
	} catch (const std::bad_alloc&) {
		errcode = CL_OUT_OF_HOST_MEMORY;
	}

	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return computeNode;
}

cl_int clRetainComputeNodeWWU(
		cl_compute_node_WWU compute_node) {
	if (compute_node) {
		/* TODO Validate compute node */
		compute_node->retain();
	} else {
		return CL_INVALID_NODE_WWU;
	}

	return CL_SUCCESS;
}

cl_int clReleaseComputeNodeWWU(
		cl_compute_node_WWU compute_node) {
	if (compute_node) {
		/* TODO Validate compute node */
		try {
		    dclicd::release(compute_node);
		} catch (const dclicd::Error& err) {
			return err.err();
		}
	} else {
		return CL_INVALID_NODE_WWU;
	}

	return CL_SUCCESS;
}

cl_int clGetComputeNodesWWU(cl_platform_id platform, cl_uint num_entries,
		cl_compute_node_WWU *compute_nodes, cl_uint *num_compute_nodes) {
	if (platform) {
		/* TODO Validate platform */
	} else {
		/* Behavior is implementation defined, if platform is NULL */
		platform = _cl_platform_id::dOpenCL();
	}

	if (compute_nodes) {
		if (num_entries == 0) return CL_INVALID_VALUE;
	} else {
		if (!num_compute_nodes) return CL_INVALID_VALUE;
	}

	try {
		std::vector<cl_compute_node_WWU> computeNodes_;

		platform->getComputeNodes(computeNodes_);

        if (compute_nodes) {
            auto computeNode = std::begin(computeNodes_);
            for (cl_uint i = 0; i < num_entries && computeNode != std::end(computeNodes_); ++i) {
                *compute_nodes++ = *computeNode++;
            }
        }
        if (num_compute_nodes) *num_compute_nodes = computeNodes_.size();
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}

cl_int clGetComputeNodeInfoWWU(
		cl_compute_node_WWU compute_node,
		cl_compute_node_info_WWU param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) {
    /* TODO Validate compute node */
    if (!compute_node) return CL_INVALID_NODE_WWU;

    try {
        compute_node->getInfo(param_name, param_value_size, param_value,
                param_value_size_ret);
    } catch (const dclicd::Error& err) {
        return err.err();
    }

	return CL_SUCCESS;
}

/* Device APIs */

cl_int clGetDeviceIDsFromComputeNodeWWU(cl_compute_node_WWU compute_node,
        cl_device_type device_type, cl_uint num_entries, cl_device_id *devices,
        cl_uint *num_devices) {
    /* TODO Validate compute node */
    if (!compute_node) return CL_INVALID_NODE_WWU;

    if (devices) {
        if (num_entries == 0) return CL_INVALID_VALUE;
    } else {
        if (!num_devices) return CL_INVALID_VALUE;
    }

    try {
        std::vector<cl_device_id> devices_;

        compute_node->getDevices(device_type, devices_);

        if (devices) {
            auto device = std::begin(devices_);
            for (cl_uint i = 0; i < num_entries && device != std::end(devices_); ++i) {
                *devices++ = *device++;
            }
        }
        if (num_devices) *num_devices = devices_.size();
    } catch (const dclicd::Error& err) {
        return err.err();
    }

    return CL_SUCCESS;
}

/* Context APIs */

cl_context clCreateContextFromComputeNodesWWU(
		const cl_context_properties *properties, cl_int num_compute_nodes,
		const cl_compute_node_WWU *compute_nodes, void(*pfn_notify)(
				const char *errinfo, const void *private_info, size_t cb,
				void *user_data), void *user_data, cl_int *errcode_ret) {
	cl_context context = nullptr;
	cl_int errcode = CL_SUCCESS;

    if (num_compute_nodes == 0 || !compute_nodes) {
        errcode = CL_INVALID_VALUE;
    }

    /*
     * Create context
     */
    if (errcode == CL_SUCCESS) {
        try {
            std::unique_ptr<dclicd::detail::ContextProperties> properties_;
            if (properties) properties_.reset(new dclicd::detail::ContextProperties(properties));

            context = new _cl_context(properties_.get(),
                    std::vector<cl_compute_node_WWU>(compute_nodes,
                            compute_nodes + num_compute_nodes),
                    pfn_notify, user_data);
            errcode = CL_SUCCESS;
        } catch (const dclicd::Error& err) {
            errcode = err.err();
        } catch (const std::bad_alloc& err) {
            errcode = CL_OUT_OF_HOST_MEMORY;
        }
    }

	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return context;
}
