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
 * \file Program.cpp
 *
 * Program test suite
 *
 * \date 2013-12-13
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

#define BOOST_TEST_MODULE Program
#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <memory>

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

template<cl_program_info Param>
struct ProgramInfo;

#define ProgramInfo(param, type)                \
template<> struct ProgramInfo<param> {          \
    typedef type Type;                               \
    static const char Name[]; };                     \
const char ProgramInfo<param>::Name[] = #param;

ProgramInfo(CL_PROGRAM_CONTEXT, cl_context);
ProgramInfo(CL_PROGRAM_NUM_DEVICES, cl_uint);
ProgramInfo(CL_PROGRAM_DEVICES, cl_device_id *);
ProgramInfo(CL_PROGRAM_SOURCE, char *);
ProgramInfo(CL_PROGRAM_BINARIES, unsigned char **);
ProgramInfo(CL_PROGRAM_BINARY_SIZES, size_t *);
ProgramInfo(CL_PROGRAM_REFERENCE_COUNT, cl_uint);
#ifdef CL_VERSION_1_2
ProgramInfo(CL_PROGRAM_NUM_KERNELS, cl_uint);
ProgramInfo(CL_PROGRAM_KERNEL_NAMES, char *);
#endif

template<cl_program_info Param>
void checkProgramInfo(cl_program program,
        const typename ProgramInfo<Param>::Type param_value) {
    typename ProgramInfo<Param>::Type value;
    size_t size;

    // FIXME Allocate memory for pointer-based values
    cl_int err = clGetProgramInfo(program, Param,
            sizeof(typename ProgramInfo<Param>::Type), &value, &size);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_EQUAL(value, param_value);
    BOOST_CHECK_EQUAL(size, sizeof(typename ProgramInfo<Param>::Type));
}

} // anonymous namespace

/* ****************************************************************************
 * Test cases
 ******************************************************************************/

BOOST_FIXTURE_TEST_CASE( CreateProgramWithSource, Context )
{
    cl_int err = CL_SUCCESS;
    cl_uint num_sources = 3u;
    const char *sources[] = {dcltest::source, dcltest::source1, dcltest::source2};
    size_t lengths[] = {strlen(dcltest::source), strlen(dcltest::source1), strlen(dcltest::source2)};

    cl_program program = clCreateProgramWithSource(context, num_sources, sources, lengths, &err);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK(program != nullptr);

    // clean up
    clReleaseProgram(program);
}

/* TODO Test clCreateProgramWithBinary */

// TODO Create a test case for each info item
BOOST_FIXTURE_TEST_CASE( GetProgramInfo, Context )
{
    cl_program program = dcltest::createProgramWithSource(context, 1, &dcltest::source);
    cl_uint num_devices = 0;
    std::unique_ptr<cl_device_id[]> devices;
    cl_int err = CL_SUCCESS;

    // get device list from context
    err = clGetContextInfo(context, CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &num_devices, nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    BOOST_REQUIRE_GE(num_devices, 1);
    devices.reset(new cl_device_id[num_devices]);
    err = clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(devices.get()), devices.get(), nullptr);
    BOOST_REQUIRE_EQUAL(err, CL_SUCCESS);
    for (cl_uint i = 0; i < num_devices; ++i) {
        BOOST_REQUIRE(devices[i] != nullptr);
    }

    // validate program infos/
    checkProgramInfo<CL_PROGRAM_CONTEXT>(program, context);
    checkProgramInfo<CL_PROGRAM_NUM_DEVICES>(program, num_devices);
    checkProgramInfo<CL_PROGRAM_REFERENCE_COUNT>(program, 1);

    // check program devices
    cl_device_id devices_ret[num_devices];
    size_t size_ret;
    err = clGetProgramInfo(program, CL_PROGRAM_DEVICES,
            sizeof(cl_device_id) * num_devices, &devices_ret, &size_ret);
    BOOST_CHECK_EQUAL(err, CL_SUCCESS);
    BOOST_CHECK_EQUAL(devices_ret[0], device);
    BOOST_CHECK_EQUAL(size_ret, sizeof(cl_device_id) * num_devices);

    // TODO Check program source
    // TODO Check program binaries
    // TODO Check binary sizes
#ifdef CL_VERSION_1_2
    /* TODO Check CL_PROGRAM_NUM_KERNELS, CL_PROGRAM_KERNEL_NAMES (must fail) */
#endif

    // clean up
    clReleaseProgram(program);
}
