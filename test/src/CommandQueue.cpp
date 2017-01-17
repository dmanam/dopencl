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
 * \file Command queue.cpp
 *
 * Command queue test suite
 *
 * \date 2013-02-18
 * \author Philipp Kegel
 */

#include "utility.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define BOOST_TEST_MODULE Command queue
#include <boost/test/unit_test.hpp>

namespace {

struct Context {
    Context() {
        cl_platform_id platform = dcltest::getPlatform();

        device = dcltest::getDevice(platform);
        context = dcltest::createContext(1, &device);

        BOOST_TEST_MESSAGE("Set up fixture");
    }

    ~Context() {
        // clean up
        clReleaseContext(context);

        BOOST_TEST_MESSAGE("Teared down fixture");
    }

    cl_device_id device;
    cl_context context;
};

/* ****************************************************************************/

template<cl_command_queue_info Param>
struct CommandQueueInfo;

#define CommandQueueInfo(param, type)                \
template<> struct CommandQueueInfo<param> {          \
    typedef type Type;                               \
    static const char Name[]; };                     \
const char CommandQueueInfo<param>::Name[] = #param;

CommandQueueInfo(CL_QUEUE_CONTEXT, cl_context);
CommandQueueInfo(CL_QUEUE_DEVICE, cl_device_id);
CommandQueueInfo(CL_QUEUE_PROPERTIES, cl_command_queue_properties);
CommandQueueInfo(CL_QUEUE_REFERENCE_COUNT, cl_uint);

template<cl_command_queue_info Param>
void checkCommandQueueInfo(cl_command_queue command_queue,
        const typename CommandQueueInfo<Param>::Type param_value) {
    typename CommandQueueInfo<Param>::Type value;
    size_t size;

    cl_int err = clGetCommandQueueInfo(command_queue, Param,
            sizeof(typename CommandQueueInfo<Param>::Type), &value, &size);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_EQUAL(value, param_value);
    BOOST_CHECK_EQUAL(size, sizeof(typename CommandQueueInfo<Param>::Type));
}

} // anonymous namespace

/* ****************************************************************************
 * Test cases
 ******************************************************************************/

BOOST_FIXTURE_TEST_CASE( CreateCommandQueue, Context )
{
    cl_int err = CL_SUCCESS;

    cl_command_queue command_queue = clCreateCommandQueue(context, device, 0, &err);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK(command_queue != nullptr);

    // clean up
    clReleaseCommandQueue(command_queue);
}

// TODO Create a test case for each info item
BOOST_FIXTURE_TEST_CASE( GetCommandQueueInfo, Context )
{
    cl_command_queue command_queue = dcltest::createCommandQueue(context, device);

    // validate command queue infos
    checkCommandQueueInfo<CL_QUEUE_CONTEXT>(command_queue, context);
    checkCommandQueueInfo<CL_QUEUE_DEVICE>(command_queue, device);
    checkCommandQueueInfo<CL_QUEUE_PROPERTIES>(command_queue, 0);
    checkCommandQueueInfo<CL_QUEUE_REFERENCE_COUNT>(command_queue, 1);

    // clean up
    clReleaseCommandQueue(command_queue);
}

BOOST_FIXTURE_TEST_CASE( RetainCommandQueue, Context )
{
    cl_command_queue command_queue = dcltest::createCommandQueue(context, device);
    cl_uint ref_count = 0;
    cl_int err = CL_SUCCESS;

    // check preconditions
    err = clGetCommandQueueInfo(command_queue, CL_QUEUE_REFERENCE_COUNT, sizeof(cl_uint), &ref_count, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE_EQUAL(ref_count, 1);

    // validate retain command
    err = clRetainCommandQueue(command_queue);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);

    err = clGetCommandQueueInfo(command_queue, CL_QUEUE_REFERENCE_COUNT, sizeof(cl_uint), &ref_count, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_EQUAL(ref_count, 2);

    // clean up
    clReleaseCommandQueue(command_queue);
    clReleaseCommandQueue(command_queue);
}

BOOST_FIXTURE_TEST_CASE( ReleaseCommandQueue, Context )
{
    cl_command_queue command_queue = dcltest::createCommandQueue(context, device);
    cl_uint ref_count = 0;
    cl_int err = CL_SUCCESS;

    // check preconditions
    err = clRetainCommandQueue(command_queue);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    err = clGetCommandQueueInfo(command_queue, CL_QUEUE_REFERENCE_COUNT, sizeof(cl_uint), &ref_count, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE_EQUAL(ref_count, 2);

    // validate release command
    err = clReleaseCommandQueue(command_queue);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    err = clGetCommandQueueInfo(command_queue, CL_QUEUE_REFERENCE_COUNT, sizeof(cl_uint), &ref_count, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_EQUAL(ref_count, 1);

    /* TODO Release should invalidate command queue when reference count becomes zero
    err = clReleaseCommandQueue(command_queue);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    err = clReleaseCommandQueue(command_queue);
    BOOST_CHECK_EQUAL(err, CL_INVALID_COMMAND_QUEUE);
    err = clRetainCommandQueue(command_queue);
    BOOST_CHECK_EQUAL(err, CL_INVALID_COMMAND_QUEUE);
    err = clGetCommandQueueInfo(command_queue, CL_QUEUE_REFERENCE_COUNT, sizeof(cl_uint), &ref_count, nullptr);
    BOOST_CHECK_EQUAL(err, CL_INVALID_COMMAND_QUEUE);
     */

    // clean up
    clReleaseCommandQueue(command_queue);
}
