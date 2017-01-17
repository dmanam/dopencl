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
 * @file cl_wwu_dcl.h
 *
 * @date 2011-04-12
 * @author Philipp Kegel
 *
 * OpenCL API extension for distributed execution
 *
 * This API extension has three layers:
 * 1) Static and transparent compute node management;
 * 2) Dynamic compute node management;
 * 3) Dynamic device management.
 *
 * The first layer only introduces compute nodes into the OpenCL platform model.
 * Compute nodes can be obtained from a platform, and information on these
 * compute nodes can be queried by the application.
 * Compute nodes cannot be added or removed by the application, but are
 * statically configured using a node file on the host and the compute nodes.
 * This approach is fully transparent and compatible with OpenCL applications.
 *
 * The second layer facilitates dynamic compute node management. An application
 * can create or release compute nodes within a platform. This also implicates
 * that devices can be retained or released by the application such that compute
 * nodes are only removed from the platform if no device that is associated with
 * that compute node will be used by the application anymore.
 *
 * The third layer facilitates dynamic device management. An application can
 * create devices such that a resource manager adds appropriate compute nodes
 * from a cloud to a given platform.
 */

#ifndef CL_WWU_DCL_H_
#define CL_WWU_DCL_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_platform.h>
#else
#include <CL/cl.h>
#include <CL/cl_platform.h>
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define cl_wwu_dcl 1

/* Additional error codes */
#define CL_INVALID_NODE_FILE_WWU -2001
#define CL_INVALID_NODE_NAME_WWU -2002
#define CL_INVALID_NODE_WWU      -2003
#define CL_CONNECTION_ERROR_WWU  -2004
#define CL_IO_ERROR_WWU          -2005
#define CL_PROTOCOL_ERROR_WWU    -2006

/******************************************************************************/

typedef struct _cl_compute_node_WWU * cl_compute_node_WWU;

typedef cl_uint     cl_compute_node_info_WWU;

typedef intptr_t    cl_device_properties_WWU;

/* cl_compute_node_info_WWU */
#define CL_NODE_PLATFORM_WWU                           0x0800
#define CL_NODE_REFERENCE_COUNT_WWU                    0x0801
#define CL_NODE_URL_WWU                                0x0802
#define CL_NODE_PROFILE_WWU                            0x0803
#define CL_NODE_VERSION_WWU                            0x0804
#define CL_NODE_NAME_WWU                               0x0805
#define CL_NODE_VENDOR_WWU                             0x0806
#define CL_NODE_EXTENSIONS_WWU                         0x0807
#define CL_NODE_AVAILABLE_WWU                          0x0808

/* cl_device_info */
#define CL_DEVICE_COMPUTE_NODE_WWU                     0x1040

#ifndef CL_VERSION_1_2
/* cl_program_info */
/* Provide CL_PROGRAM_NUM_KERNELS for forward compatibility of OpenCL 1.1 */
#define CL_PROGRAM_NUM_KERNELS                      0x1167

/* cl_command_type */
/* Provide CL_COMMAND_BARRIER for forward compatibility of OpenCL 1.1 */
#define CL_COMMAND_BARRIER                          0x1205
#endif

/* cl_profiling_info  */
#define CL_PROFILING_COMMAND_RECEIVED_WWU           0x1284

/******************************************************************************/

/* Compute node APIs */
/* TODO Specify compute node semantics
 * a) A compute node is regarded a simple device provider
 * b) A compute node is regarded an access point for a network of device providers
 */

/**
 * @brief Adds a compute node to a platform.
 *
 * @param[in]  platform     the platform the newly created compute node is added to.
 *             Refers to the platform ID returned by clGetPlatformIDs or can be NULL.
 *             If platform is NULL, the behavior is implementation-defined.
 * @param[in]  url          the URL of the compute node to add
 * @param[in]  pfn_notify   A callback function that can be registered by the application.
 *             This callback function will be used by the OpenCL implementation to report information on errors that occur during communication with this compute node.
 *             This callback function may be called asynchronously by the OpenCL implementation.
 *             It is the application's responsibility to ensure that the callback function is thread-safe.
 *             If pfn_notify is NULL, no callback function is registered.
 * @param[in]  user_data
 * @param[out] errcode_ret  error code
 * @return a compute node, or NULL if the compute node URL is invalid
 */
extern CL_API_ENTRY cl_compute_node_WWU CL_API_CALL
clCreateComputeNodeWWU(
		cl_platform_id  /* platform */,
		const char *    /* url */,
        void (*         /* pfn_notify */)(
                cl_compute_node_WWU /* compute_node */,
                cl_int              /* connection_status */,
                void *              /* user_data */),
        void *          /* user_data */,
        cl_int *        /* errcode_ret */) CL_EXT_SUFFIX__VERSION_1_1;

/**
 * @brief Increments a compute node's reference counter.
 *
 * @param[in]  compute_node compute node
 * @return error code
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clRetainComputeNodeWWU(
        cl_compute_node_WWU	/* compute_node */) CL_EXT_SUFFIX__VERSION_1_1;

/**
 * @brief Decrements a compute node's reference counter, removing the compute node from its platform if the counter becomes 0.
 *
 * @param[in]  compute_node compute node
 * @return error code
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseComputeNodeWWU(
        cl_compute_node_WWU /* compute_node */) CL_EXT_SUFFIX__VERSION_1_1;

/**
 * @brief Returns a list of compute nodes.
 *
 * @param[in]  platform
 * @param[in]  num_entries
 * @param[out] compute_nodes
 * @param[out] num_compute_nodes
 * @return
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clGetComputeNodesWWU(
		cl_platform_id          /* platform */,
		cl_uint                 /* num_entries */,
		cl_compute_node_WWU*    /* compute_nodes */,
		cl_uint *               /* num_compute_nodes */) CL_EXT_SUFFIX__VERSION_1_1;

/**
 * @brief Returns the specified information on a given compute node.
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clGetComputeNodeInfoWWU(
        cl_compute_node_WWU         /* compute_node */,
        cl_compute_node_info_WWU	/* param_name */,
        size_t                      /* param_value_size */,
        void *                      /* param_value */,
        size_t *                    /* param_value_size_ret */) CL_EXT_SUFFIX__VERSION_1_1;

/* Context APIs */

extern CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromComputeNodesWWU(
		const cl_context_properties *   /* properties */,
		cl_int                          /* num_compute_nodes */,
		const cl_compute_node_WWU *     /* compute_nodes */,
		void (CL_CALLBACK *             /* pfn_notify*/ )(
				const char *, const void *, size_t, void *),
		void *                          /* user_data */,
		cl_int *                        /* errcode_ret */) CL_API_SUFFIX__VERSION_1_1;

/* Extended device APIs */

/**
 * @brief Obtain the list of devices available on a particular compute node.
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDsFromComputeNodeWWU(
        cl_compute_node_WWU /* compute_node */,
        cl_device_type      /* device_type */,
        cl_uint             /* num_entries */,
        cl_device_id *      /* devices */,
        cl_uint *           /* num_devices */) CL_EXT_SUFFIX__VERSION_1_1;

/**
 * @brief Allocates devices from the resource manager's device pool.
 *
 * CL_INVALID_PROPERTY if device property name in properties is not a supported
 * property name, if the value specified for a supported property name is not
 * valid, or if the same property name is specified more than once.
 *
 * @param[in]  platform     the platform the newly created compute node is added to.
 *             Refers to the platform ID returned by clGetPlatformIDs or can be NULL.
 *             If platform is NULL, the behavior is implementation-defined.
 * @param[in]  properties   Specifies a list of device property names and their
 *             corresponding values. Each property name is immediately followed
 *             by the corresponding desired value. The list is terminated with 0.
 *             properties can be NULL in which case the device that is returned
 *             is implementation-defined. The list of supported properties is
 *             described in the table below.
 * @param[in]  num_devices
 * @param[in]  devices
 * @param[out] num_devices_ret
 * @return error code
 */
extern CL_API_ENTRY cl_int CL_API_CALL
clCreateDevicesWWU(
		cl_platform_id              /* platform */,
        cl_device_properties_WWU    /* properties */,
        size_t                      /* num_devices */,
        cl_device_id *              /* devices */,
        size_t                      /* num_device_ret */) CL_EXT_SUFFIX__VERSION_1_1;

/**
 * @brief Increments a device's reference counter.
 *
 * @param[in]  device   device
 * @return error code
 */
/* TODO Merge clRetainDeviceWWU with clRetainDevice (OpenCL 1.2) */
extern CL_API_ENTRY cl_int CL_API_CALL
clRetainDeviceWWU(
        cl_device_id /* device */) CL_EXT_SUFFIX__VERSION_1_1;

/**
 * @brief Decrements a device's reference counter, removing it from its platform if the counter becomes 0.
 *
 * The returned device will no longer be available to the application, i.e. any
 * reference to this device will be regarded invalid.
 *
 * @param[in]  device   the device to return
 * @return error code
 */
/* TODO Merge clRetainDeviceWWU with clRetainDevice (OpenCL 1.2) */
extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseDeviceWWU(
        cl_device_id /* device */) CL_EXT_SUFFIX__VERSION_1_1;

/******************************************************************************/

typedef CL_API_ENTRY cl_compute_node_WWU (CL_API_CALL *clCreateComputeNodeWWU_fn)(
        const char *    /* url */,
        void (*         /* pfn_notify */)(
                cl_compute_node_WWU /* compute_node */,
                cl_int              /* connection_status */,
                void *              /* user_data */),
        void *          /* user_data */,
        cl_int *        /* errcode_ret */);
typedef CL_API_ENTRY cl_int (CL_API_CALL *clRetainComputeNodeWWU_fn)(
        cl_compute_node_WWU /* compute_node */);
typedef CL_API_ENTRY cl_int (CL_API_CALL *clReleaseComputeNodeWWU_fn)(
        cl_compute_node_WWU /* compute_node */);
typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetComputeNodesWWU_fn)(
		cl_platform_id          /* platform */,
		cl_uint                 /* num_entries */,
		cl_compute_node_WWU *   /* compute_nodes */,
		cl_uint *               /* num_compute_nodes */);
typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetComputeNodeInfoWWU_fn)(
        cl_compute_node_WWU         /* compute_node */,
        cl_compute_node_info_WWU    /* param_name */,
        size_t                      /* param_value_size */,
        void *                      /* param_value */,
        size_t *                    /* param_value_size_ret */);

typedef CL_API_ENTRY cl_context (CL_API_CALL *clCreateContextFromComputeNodesWWU_fn)(
		const cl_context_properties *   /* properties */,
		cl_int                          /* num_compute_nodes */,
		const cl_compute_node_WWU *     /* compute_nodes */,
		void (CL_CALLBACK *             /* pfn_notify*/ )(
				const char *, const void *, size_t, void *),
		void *                          /* user_data */,
		cl_int *                        /* errcode_ret */);

typedef CL_API_ENTRY cl_int (CL_API_CALL *clCreateDevicesWWU_fn)(
		cl_platform_id              /* platform */,
        cl_device_properties_WWU    /* properties */,
        size_t                      /* num_devices */,
        cl_device_id *              /* devices */,
        size_t                      /* num_device_ret */);
typedef CL_API_ENTRY cl_int (CL_API_CALL *clRetainDeviceWWU_fn)(
        cl_device_id /* device */);
typedef CL_API_ENTRY cl_int (CL_API_CALL *clReleaseDeviceWWU_fn)(
        cl_device_id /* device */);

#ifdef __cplusplus
}
#endif

#endif /* CL_WWU_DCL_H_ */
