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
 * \file utility.cpp
 *
 * \date 2013-12-08
 * \author Philipp Kegel
 */

#include <boost/test/unit_test.hpp>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cstddef>
#include <cstring>

namespace dcltest {

const char *source =
        "__kernel void add(__global float *a, __global float *b, __global float *c) {"
        "   size_t idx = get_global_id(0)"
        "   c[idx] = a[idx] + b[idx]; }";
const char *source1 =
        "__kernel void incr(__global float *x) {"
        "   ++x[get_global_id(0)]; }"
        "__kernel void scale(float a, __global float *x) {"
        "   x[get_global_id(0)] *= a; }";
const char *source2 =
        "void sgemm(         float  alpha"
        "                    float  beta,"
        "           __global float *Ad,"
        "           __global float *Bd,"
        "           __global float *Cd,"
        "                    int    width) {"
        "   int col = get_global_id(0);"
        "   int row = get_global_id(1);"
        "   float sum = 0;"
        "   for (int k = 0; k < width; k+=1)"
        "       sum += Ad[row * width + k] * Bd[k * width + col];"
        "   Cd[row * width + col] = alpha * sum + beta * Cd[row * width + col];"
        "}";

cl_platform_id getPlatform() {
    cl_platform_id platform = nullptr;
    cl_uint num_platforms = 0;
    size_t param_value_size = 0;
    cl_int err = CL_SUCCESS;

    err = clGetPlatformIDs(1, &platform, &num_platforms);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE_MESSAGE(num_platforms >= 1, "No platform found");

    /* platform should be 'dOpenCL' */
    err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, nullptr,
            &param_value_size);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    char platform_name[param_value_size];
    err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, param_value_size,
            platform_name, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_MESSAGE(strcmp(platform_name, "dOpenCL") == 0, "First platform must be 'dOpenCL'");

    return platform;
}

void getComputeNodes(cl_platform_id platform, cl_uint num_nodes, cl_compute_node_WWU *nodes) {
    cl_uint num_nodes_ret = 0;

    cl_int err = clGetComputeNodesWWU(platform, num_nodes, nodes, &num_nodes_ret);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE_MESSAGE(num_nodes_ret >= num_nodes, "No enough compute nodes");
}

void getDevices(cl_platform_id platform, cl_device_type type, cl_uint num_devices, cl_device_id *devices) {
    cl_uint num_devices_ret = 0;

    cl_int err = clGetDeviceIDs(platform, type, num_devices, devices,
            &num_devices_ret);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE_MESSAGE(num_devices_ret >= num_devices, "No enough devices");
}

cl_device_id getDevice(cl_platform_id platform) {
    cl_device_id device = nullptr;
    cl_uint num_devices = 0;

    cl_int err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device,
            &num_devices);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE(device != nullptr);
    BOOST_REQUIRE_GE(num_devices, 1);

    return device;
}

cl_device_id getDevice(cl_compute_node_WWU node) {
    cl_device_id device = nullptr;
    cl_uint num_devices = 0;

    cl_int err = clGetDeviceIDsFromComputeNodeWWU(node, CL_DEVICE_TYPE_ALL, 1, &device,
            &num_devices);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE(device != nullptr);
    BOOST_REQUIRE_GE(num_devices, 1);

    return device;
}

cl_context createContext(cl_uint num_devices, cl_device_id *devices) {
    cl_int err = CL_SUCCESS;

    BOOST_REQUIRE_GE(num_devices, 1);
    BOOST_REQUIRE(devices != nullptr);

    cl_context context = clCreateContext(0, num_devices, devices, 0, 0, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE(context != nullptr);

    return context;
}

cl_command_queue createCommandQueue(cl_context context, cl_device_id device,
        cl_command_queue_properties properties) {
    cl_int err = CL_SUCCESS;

    cl_command_queue commandQueue = clCreateCommandQueue(context, device, properties, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE(commandQueue != nullptr);

    return commandQueue;
}

cl_program createProgramWithSource(
        cl_context context,
        cl_uint num_sources,
        const char **sources,
        size_t *lengths) {
    cl_int err = CL_SUCCESS;

    cl_program program = clCreateProgramWithSource(context, num_sources, sources, lengths, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE(program != nullptr);

    return program;
}

cl_mem createRWBuffer(cl_context context, size_t cb) {
    cl_int err = CL_SUCCESS;

    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, cb, nullptr, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE(buffer != nullptr);

    return buffer;
}

} // namespace dcltest
