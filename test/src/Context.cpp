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
 * \file Buffer.cpp
 *
 * Context test suite
 *
 * \date 2013-04-07
 * \author Philipp Kegel
 */

#include "utility.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#define BOOST_TEST_MODULE Context
#include <boost/test/unit_test.hpp>

/* ****************************************************************************
 * Test cases
 ******************************************************************************/

BOOST_AUTO_TEST_CASE( CreateContext )
{
    cl_platform_id platform = dcltest::getPlatform();
    cl_device_id device = nullptr;
    cl_context context = nullptr;
    cl_int err = CL_SUCCESS;

    context = clCreateContext(0, 1, 0, 0, 0, &err);
    BOOST_REQUIRE_EQUAL(err, CL_INVALID_VALUE);
    context = clCreateContext(0, 0, &device, 0, 0, &err);
    BOOST_REQUIRE_EQUAL(err, CL_INVALID_VALUE);
    context = clCreateContext(0, 1, &device, 0, &err, &err);
    BOOST_REQUIRE_EQUAL(err, CL_INVALID_VALUE);

    dcltest::getDevices(platform, CL_DEVICE_TYPE_ALL, 1, &device);

    // create context
    context = clCreateContext(0, 1, &device, 0, 0, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // clean up
    clReleaseContext(context);
}

BOOST_AUTO_TEST_CASE( CreateContextFromComputeNode )
{
    cl_platform_id platform = dcltest::getPlatform();
    cl_compute_node_WWU compute_node = nullptr;
    cl_context context = nullptr;
    cl_int err = CL_SUCCESS;

    context = clCreateContextFromComputeNodesWWU(0, 1, 0, 0, 0, &err);
    BOOST_REQUIRE_EQUAL(err, CL_INVALID_VALUE);
    context = clCreateContextFromComputeNodesWWU(0, 0, &compute_node, 0, 0, &err);
    BOOST_REQUIRE_EQUAL(err, CL_INVALID_VALUE);
    context = clCreateContextFromComputeNodesWWU(0, 0, &compute_node, 0, &err, &err);
    BOOST_REQUIRE_EQUAL(err, CL_INVALID_VALUE);

    dcltest::getComputeNodes(platform, 1, &compute_node);

    // create context from compute node
    context = clCreateContextFromComputeNodesWWU(0, 1, &compute_node, 0, 0, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // verify context
    cl_uint num_devices = 0;
    cl_uint num_devices_ctx = 0;

    err = clGetDeviceIDsFromComputeNodeWWU(compute_node, CL_DEVICE_TYPE_ALL, 0, 0, &num_devices);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    err = clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, sizeof(num_devices), &num_devices_ctx, 0);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE_EQUAL(num_devices_ctx, num_devices);

    cl_device_id devices[num_devices];
    cl_device_id devices_ctx[num_devices_ctx];

    err = clGetDeviceIDsFromComputeNodeWWU(compute_node, CL_DEVICE_TYPE_ALL, num_devices, devices, 0);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    err = clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(devices), &devices_ctx, 0);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    for (int i = 0; i < num_devices; ++i) {
        BOOST_REQUIRE_EQUAL(devices_ctx[i], devices[i]);
    }

    // clean up
    clReleaseContext(context);
}

BOOST_AUTO_TEST_CASE( CreateContextFromType )
{
    cl_platform_id platform = dcltest::getPlatform();
    cl_context context = nullptr;
    cl_int err = CL_SUCCESS;

    context = clCreateContextFromType(0, CL_DEVICE_TYPE_ALL, 0, &err, &err);
    BOOST_REQUIRE_EQUAL(err, CL_INVALID_VALUE);

    // create context from type
    context = clCreateContextFromType(0, CL_DEVICE_TYPE_ALL, 0, 0, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // clean up
    clReleaseContext(context);
}
