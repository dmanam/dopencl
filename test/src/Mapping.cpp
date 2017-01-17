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
 * \file Mapping.cpp
 *
 * Memory mapping test suite (part of the memory test module)
 *
 * \date 2014-01-11
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
#include <cstdlib>
#include <vector>

namespace {

struct Context {
    cl_context context;
    cl_command_queue commandQueue;
    cl_mem buffer;
    size_t vecSize;
    size_t cb;

    Context() :
            vecSize(1024 * 1024) /* #vector elements */,
            cb(vecSize * sizeof(cl_int)) /* buffer size */
    {
        cl_platform_id platform = dcltest::getPlatform();
        cl_device_id device = dcltest::getDevice(platform);

        context = dcltest::createContext(1, &device);
        commandQueue = dcltest::createCommandQueue(context, device);
        buffer = dcltest::createRWBuffer(context, cb);

        BOOST_TEST_MESSAGE("Set up fixture");
    }

    ~Context() {
        // clean up
        clReleaseMemObject(buffer);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);

        BOOST_TEST_MESSAGE("Teared down fixture");
    }
};

} // anonymous namespace

/* ****************************************************************************/

BOOST_FIXTURE_TEST_SUITE( Mapping, Context )

/* ****************************************************************************
 * Test cases
 ******************************************************************************/

BOOST_AUTO_TEST_CASE( MapWrite )
{
    cl_event unmap = nullptr;
    std::vector<cl_int> vecIn(vecSize, 0), vecOut(vecSize, 1);
    cl_int err = CL_SUCCESS;

    dcltest::fillVector(vecIn, 1, 1); // initialize input data

    // map buffer
    void *ptr = clEnqueueMapBuffer(
            commandQueue, buffer, CL_TRUE, CL_MAP_WRITE, 0, cb, 0, nullptr, nullptr, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    memcpy(ptr, &vecIn.front(), cb); // copy data

    // unmap buffer, i.e., upload data to device
    err = clEnqueueUnmapMemObject(commandQueue, buffer, ptr, 0, nullptr, &unmap);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // download data from device on second compute node
    err = clEnqueueReadBuffer(commandQueue, buffer, CL_TRUE, 0, cb,
            &vecOut.front(), 1, &unmap, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    BOOST_CHECK_MESSAGE(vecIn == vecOut, "Input and output buffers differ"); // compare input and output data

    // clean up
    clReleaseEvent(unmap);
}


BOOST_AUTO_TEST_CASE( MapRead )
{
    cl_event unmap = nullptr;
    std::vector<cl_int> vec(vecSize, 0);
    cl_int err = CL_SUCCESS;

    dcltest::fillVector(vec, 1, 1); // initialize input data

    // update data to device
    err = clEnqueueWriteBuffer(commandQueue, buffer, CL_FALSE, 0, cb,
            &vec.front(), 0, nullptr, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // map buffer for reading (implicit download)
    void *ptr = clEnqueueMapBuffer(
            commandQueue, buffer, CL_TRUE, CL_MAP_READ, 0, cb, 0, nullptr, nullptr, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    // compare buffer data and mapped data
    BOOST_CHECK_MESSAGE(memcmp(&vec.front(), ptr, cb) == 0, "Input data and mapped data differ");

    // unmap buffer
    err = clEnqueueUnmapMemObject(commandQueue, buffer, ptr, 0, nullptr, &unmap);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    err = clFinish(commandQueue);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // clean up
    clReleaseEvent(unmap);
}

BOOST_AUTO_TEST_SUITE_END()
