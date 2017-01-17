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
 * \file Device.cpp
 *
 * Device test suite
 *
 * \date 2013-12-15
 * \author Philipp Kegel
 */

#include "utility.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define BOOST_TEST_MODULE Device
#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <cstring>

namespace {

template<cl_device_info Param>
struct DeviceInfo;

#define DeviceInfo(param, type)                \
template<> struct DeviceInfo<param> {          \
    typedef type Type;                         \
    static const char Name[]; };               \
const char DeviceInfo<param>::Name[] = #param;

DeviceInfo(CL_DEVICE_PLATFORM, cl_platform_id);
DeviceInfo(CL_DEVICE_EXECUTION_CAPABILITIES, cl_device_exec_capabilities);
#ifdef CL_VERSION_1_2
DeviceInfo(CL_DEVICE_REFERENCE_COUNT, cl_uint);
#endif

template<cl_command_queue_info Param>
void checkDeviceInfo(cl_device_id device,
        const typename DeviceInfo<Param>::Type param_value) {
    typename DeviceInfo<Param>::Type value;
    size_t size;

    cl_int err = clGetDeviceInfo(device, Param,
            sizeof(typename DeviceInfo<Param>::Type), &value, &size);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_EQUAL(value, param_value);
    BOOST_CHECK_EQUAL(size, sizeof(typename DeviceInfo<Param>::Type));
}

} // anonymous namespace

/* ****************************************************************************
 * Test cases
 ******************************************************************************/

BOOST_AUTO_TEST_CASE( GetDeviceIDs )
{
    cl_platform_id platform = dcltest::getPlatform();
    cl_uint num_devices = 0;
    cl_device_id device = nullptr;
    cl_int err = CL_SUCCESS;

    /* TODO Test CL_INVALID_PLATFORM */

    // test CL_INVALID_VALUE
    err = clGetDeviceIDs(nullptr, CL_DEVICE_TYPE_ALL, 0, nullptr, nullptr);
    BOOST_CHECK_EQUAL(err, CL_INVALID_VALUE); // devices or num_device must not both be NULL
    err = clGetDeviceIDs(nullptr, CL_DEVICE_TYPE_ALL, 0, &device, &num_devices);
    BOOST_CHECK_EQUAL(err, CL_INVALID_VALUE); // devices must be NULL if num_entries is 0

    /* TODO Test getting devices */
}

// TODO Create a test case for each info item
BOOST_AUTO_TEST_CASE( GetDeviceInfo )
{
    cl_platform_id platform = dcltest::getPlatform();
    cl_device_id device = dcltest::getDevice(platform);
    cl_device_exec_capabilities exec_capabilities = 0;
    size_t size_ret = 0;
    cl_int err = CL_SUCCESS;

    checkDeviceInfo<CL_DEVICE_PLATFORM>(device, platform);

    err = clGetDeviceInfo(device, CL_DEVICE_EXECUTION_CAPABILITIES,
            sizeof(cl_device_exec_capabilities), &exec_capabilities, &size_ret);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_EQUAL(exec_capabilities & CL_EXEC_NATIVE_KERNEL, 0);
    BOOST_CHECK_EQUAL(size_ret, sizeof(cl_device_exec_capabilities));
#ifdef CL_VERSION_1_2
    checkDeviceInfo<CL_DEVICE_REFERENCE_COUNT>(device, 1);
#endif
}
