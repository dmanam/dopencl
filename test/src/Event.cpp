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
 * \file Event.cpp
 *
 * Event test suite
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

#define BOOST_TEST_MODULE Event
#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <iostream>
#include <vector>

namespace {

void eventCallback(cl_event event, cl_int execution_status,
        void *user_data) {
    std::cout << "Called event callback with status " << execution_status
            << std::endl;
}

/* ****************************************************************************/

struct Context {
    Context() {
        cl_platform_id platform = dcltest::getPlatform();
        cl_device_id device = dcltest::getDevice(platform);

        context = dcltest::createContext(1, &device);
        commandQueue = dcltest::createCommandQueue(context, device);

        BOOST_TEST_MESSAGE("Set up fixture");
    }

    ~Context() {
        // clean up
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);

        BOOST_TEST_MESSAGE("Teared down fixture");
    }

    cl_context context;
    cl_command_queue commandQueue;
};

/* ****************************************************************************/

template<cl_event_info Param>
struct EventInfo;

#define EventInfo(param, type)                \
template<> struct EventInfo<param> {          \
    typedef type Type;                        \
    static const char Name[]; };              \
const char EventInfo<param>::Name[] = #param;

EventInfo(CL_EVENT_CONTEXT, cl_context);
EventInfo(CL_EVENT_COMMAND_QUEUE, cl_command_queue);
EventInfo(CL_EVENT_COMMAND_TYPE, cl_uint);
EventInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, cl_int);

template<cl_command_queue_info Param>
void checkEventInfo(cl_event event, const typename EventInfo<Param>::Type param_value) {
    typename EventInfo<Param>::Type value;
    size_t size;

    cl_int err = clGetEventInfo(event, Param,
            sizeof(typename EventInfo<Param>::Type), &value, &size);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_EQUAL(value, param_value);
    BOOST_CHECK_EQUAL(size, sizeof(typename EventInfo<Param>::Type));
}

} // anonymous namespace

/* ****************************************************************************/

BOOST_FIXTURE_TEST_SUITE( UserEvent, Context )

/* ****************************************************************************
 * Test cases
 ******************************************************************************/

BOOST_AUTO_TEST_CASE( CreateUserEvent )
{
    cl_event event = nullptr;
    cl_int err = CL_SUCCESS;

    event = clCreateUserEvent(nullptr, &err);
    BOOST_REQUIRE_EQUAL(err, CL_INVALID_CONTEXT);

    event = clCreateUserEvent(context, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // clean up
    clSetUserEventStatus(event, CL_COMPLETE); // event status must be CL_COMPLETE to release the event
    clReleaseEvent(event);
}

// TODO Create a test case for each info item
BOOST_AUTO_TEST_CASE( GetUserEventInfo )
{
    cl_context event_context = nullptr;
    cl_command_queue event_queue = nullptr;
    cl_command_type command_type = 0u;
    size_t size = 0;
    cl_int execution_status;
    cl_int err = CL_SUCCESS;

    cl_event event = clCreateUserEvent(context, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // validate user event infos
    checkEventInfo<CL_EVENT_CONTEXT>(event, context);
    checkEventInfo<CL_EVENT_COMMAND_QUEUE>(event, nullptr);
    checkEventInfo<CL_EVENT_COMMAND_TYPE>(event, CL_COMMAND_USER);
    checkEventInfo<CL_EVENT_COMMAND_EXECUTION_STATUS>(event, CL_SUBMITTED);

    // clean up
    clSetUserEventStatus(event, CL_COMPLETE); // event status must be CL_COMPLETE to release the event
    clReleaseEvent(event);
}

BOOST_AUTO_TEST_CASE( SetUserEventStatus )
{
    cl_int execution_status = CL_SUBMITTED;
    cl_int err = CL_SUCCESS;

    cl_event event = clCreateUserEvent(context, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    err = clSetUserEventStatus(event, CL_COMPLETE);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    err = clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
            sizeof(cl_int), &execution_status, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_EQUAL(execution_status, CL_COMPLETE);

    // clean up
    clReleaseEvent(event);
}

BOOST_AUTO_TEST_CASE( Callback )
{
    const size_t SIZE = 1024;
    std::vector<cl_int> vec(SIZE, 0);
    cl_event start = nullptr;
    cl_event upload = nullptr;
    cl_int err = CL_SUCCESS;

    // create buffer
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, SIZE * sizeof(cl_int), nullptr, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // create user event
    start = clCreateUserEvent(context, &err);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    // upload data to device
    err = clEnqueueWriteBuffer(
            commandQueue,
            buffer,
            CL_FALSE,
            0, SIZE * sizeof(cl_int), &vec.front(),
            1, &start, &upload);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);

    /* register event callbacks */
    err = clSetEventCallback(start, CL_COMPLETE, &eventCallback, nullptr);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    err = clSetEventCallback(upload, CL_COMPLETE, &eventCallback, nullptr);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);

    // start upload
    err = clSetUserEventStatus(start, CL_COMPLETE);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    err = clFlush(commandQueue);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);

//    sleep(2);

    // finish command queue
    err = clFinish(commandQueue);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);

    // clean up
    clReleaseEvent(start);
    clReleaseEvent(upload);
    clReleaseMemObject(buffer);
}

BOOST_AUTO_TEST_SUITE_END()
