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
 * \file MemoryConsistency.cpp
 *
 * Memory consistency test suite (part of the memory test module)
 *
 * \date 2013-02-18
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

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <vector>

namespace {

struct MultiDeviceContext {
    MultiDeviceContext() :
            vecSize(1024 * 1024) /* #vector elements */,
            cb(vecSize * sizeof(cl_int)) /* buffer size */
    {
        cl_platform_id platform = dcltest::getPlatform();
        cl_compute_node_WWU nodes[2];

        dcltest::getComputeNodes(platform, 2, nodes);
        devices[0] = dcltest::getDevice(nodes[0]);
        devices[1] = dcltest::getDevice(nodes[1]);

        context = dcltest::createContext(2, devices);
        commandQueues[0] = dcltest::createCommandQueue(context, devices[0]);
        commandQueues[1] = dcltest::createCommandQueue(context, devices[1]);
        buffer = dcltest::createRWBuffer(context, cb);

        BOOST_TEST_MESSAGE("Setup fixture");
    }

    ~MultiDeviceContext() {
        // clean up
        clReleaseMemObject(buffer);
        clReleaseCommandQueue(commandQueues[0]);
        clReleaseCommandQueue(commandQueues[1]);
        clReleaseContext(context);

        BOOST_TEST_MESSAGE("Teared down fixture");
    }

    cl_device_id devices[2];
    cl_command_queue commandQueues[2];
    cl_context context;
    cl_mem buffer;
    size_t vecSize;
    size_t cb;
};

} // anonymous namespace

/* ****************************************************************************/

BOOST_FIXTURE_TEST_SUITE( MemoryConsistency, MultiDeviceContext )

/* ****************************************************************************
 * Test cases
 ******************************************************************************/

BOOST_AUTO_TEST_CASE( WriteRead )
{
    cl_event write = nullptr;
    std::vector<cl_int> vecIn(vecSize, 0), vecOut(vecSize, 1);
    cl_int err = CL_SUCCESS;

    dcltest::fillVector(vecIn, 1, 1); // initialize input data

    // upload data to device on first compute node
    err = clEnqueueWriteBuffer(commandQueues[0], buffer, CL_FALSE, 0, cb,
            &vecIn.front(), 0, nullptr, &write);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    // ensure start of data upload to first compute node
    err = clFlush(commandQueues[0]);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    // download data from device on second compute node
    err = clEnqueueReadBuffer(commandQueues[1], buffer, CL_TRUE, 0, cb,
            &vecOut.front(), 1, &write, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    BOOST_CHECK_MESSAGE(vecIn == vecOut, "Input and output buffers differ"); // compare input and output data

    // clean up
    clReleaseEvent(write);
}

BOOST_AUTO_TEST_CASE( NDRangeKernelRead )
{
    const char *source = "\
__kernel void init(__global int *v) {       \
    v[get_global_id(0)] = get_global_id(0); \
}";
    cl_event init = nullptr;
    std::vector<cl_int> hVec(vecSize, 0), dVec(vecSize, 1);
    cl_int err = CL_SUCCESS;

    dcltest::fillVector(hVec, 0, 1); // initialize host vector

    cl_program program = clCreateProgramWithSource(context, 1, &source, nullptr, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    err = clBuildProgram(program, 2, devices, nullptr, nullptr, nullptr);
    cl_kernel kernel = clCreateKernel(program, "init", &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // submit kernel to device on first compute node
    err = clEnqueueNDRangeKernel(commandQueues[0], kernel,
            1, nullptr, &vecSize, nullptr, 0, nullptr, &init);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    // ensure start of kernel execution on first compute node
    err = clFlush(commandQueues[0]);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    // download data from device on second compute node
    err = clEnqueueReadBuffer(commandQueues[1], buffer, CL_TRUE, 0, cb,
            &dVec.front(), 1, &init, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    BOOST_CHECK_MESSAGE(hVec == dVec, "Host and device buffers differ"); // compare host and device buffer

    // clean up
    clReleaseEvent(init);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
}

/*!
 * \brief Test cross-over exchange of two memory objects
 */
BOOST_AUTO_TEST_CASE( ConcurrentWriteRead )
{
    cl_event write[2] = {nullptr, nullptr};
    cl_event read[2] = {nullptr, nullptr};
    std::vector<cl_int> vecIn0(vecSize, 1), vecIn1(vecSize, 2);
    std::vector<cl_int> vecOut0(vecSize, 0), vecOut1(vecSize, 0);
    cl_int err = CL_SUCCESS;

    cl_mem buffer1 = dcltest::createRWBuffer(context, cb);

    // upload data to device on first compute node
    err = clEnqueueWriteBuffer(commandQueues[0], buffer, CL_FALSE, 0, cb,
            &vecIn0.front(), 0, nullptr, &write[0]);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    // upload data to device on second compute node
    err = clEnqueueWriteBuffer(commandQueues[1], buffer1, CL_FALSE, 0, cb,
            &vecIn1.front(), 0, nullptr, &write[1]);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // download data from device on first compute node
    err = clEnqueueReadBuffer(commandQueues[0], buffer1, CL_FALSE, 0, cb,
            &vecOut0.front(), 1, &write[1], &read[0]);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    // download data from device on second compute node/
    err = clEnqueueReadBuffer(commandQueues[1], buffer, CL_FALSE, 0, cb,
            &vecOut1.front(), 1, &write[0], &read[1]);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // await completion of data transfers/
    err = clWaitForEvents(2, read);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    BOOST_CHECK_MESSAGE(vecIn0 == vecOut1, "Input and output buffers differ"); // compare input and output data
    BOOST_CHECK_MESSAGE(vecIn1 == vecOut0, "Input and output buffers differ"); // compare input and output data

    // clean up
    clReleaseEvent(write[0]);
    clReleaseEvent(write[1]);
    clReleaseEvent(read[0]);
    clReleaseEvent(read[1]);
    clReleaseMemObject(buffer1);
}

BOOST_AUTO_TEST_SUITE_END()
