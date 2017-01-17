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
 * \file Platform.cpp
 *
 * Platform test suite
 *
 * \date 2013-02-19
 * \author Philipp Kegel
 */

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define BOOST_TEST_MODULE Platform
#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <cstring>

/* ****************************************************************************
 * Test cases
 ******************************************************************************/

BOOST_AUTO_TEST_CASE( GetPlatformIDs )
{
    cl_platform_id platform[] = {nullptr, nullptr};
    cl_uint num_platforms = 0;
    cl_int err = CL_SUCCESS;

    err = clGetPlatformIDs(1, nullptr, nullptr);
    BOOST_CHECK_EQUAL(err, CL_INVALID_VALUE);

    err = clGetPlatformIDs(0, platform, nullptr);
    BOOST_CHECK_EQUAL(err, CL_INVALID_VALUE);

    err = clGetPlatformIDs(0, nullptr, &num_platforms);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    // there should be only one platform available
    BOOST_CHECK_EQUAL(num_platforms, 1);

    err = clGetPlatformIDs(1, platform, &num_platforms);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    // one and only one platform (dOpenCL) should be returned
    BOOST_CHECK_MESSAGE(platform[0] != nullptr, "clGetPlatformIDs must return a platform");
    BOOST_CHECK_MESSAGE(platform[1] == nullptr, "clGetPlatformIDs must not return more than one platform");
    BOOST_CHECK_EQUAL(num_platforms, 1);
}

#define checkPlatformInfo(platform, param_name, param_value)                                         \
{                                                                                                    \
    size_t size = 0;                                                                                 \
    char value[1024]; /* FIXME does not work for info values larger than 1k */                       \
    cl_int err = clGetPlatformInfo((platform), (param_name), sizeof(value), value, &size);           \
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);                                                              \
    BOOST_CHECK_MESSAGE(strcmp(value, (param_value)) == 0, #param_name " is not '" param_value "'"); \
    BOOST_CHECK_EQUAL(size, strlen(param_value) + 1); /* size includes terminating zero */           \
}

// TODO Create a test case for each info item
BOOST_AUTO_TEST_CASE( GetPlatformInfo )
{
    cl_platform_id platform = nullptr;
    cl_int err = CL_SUCCESS;

    // obtain platform
    err = clGetPlatformIDs(1, &platform, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE(platform != nullptr); // one and only one platform (dOpenCL) should be returned

    /* TODO test error code for invalid platform */

    err = clGetPlatformInfo(platform, 0, 0, nullptr, nullptr);
    BOOST_CHECK_EQUAL(err, CL_INVALID_VALUE); // invalid parameter name should not be accepted

    // use platform 'dOpenCL', if no platform is specified
    checkPlatformInfo(nullptr, CL_PLATFORM_NAME, "dOpenCL");

    // check parameter return values
    checkPlatformInfo(platform, CL_PLATFORM_NAME, "dOpenCL");
    checkPlatformInfo(platform, CL_PLATFORM_VENDOR, "University of Muenster");
    checkPlatformInfo(platform, CL_PLATFORM_PROFILE, "FULL_PROFILE");
    checkPlatformInfo(platform, CL_PLATFORM_VERSION, "OpenCL 1.1");
    checkPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, "cl_wwu_dcl cl_wwu_collective");
}
