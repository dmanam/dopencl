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
 * \date 2012-07-30
 * \author Philipp Kegel
 */

#include "Program.h"

#include "Context.h"
#include "Device.h"
#include "Kernel.h"

#include <dcl/DataTransfer.h>
#include <dcl/Device.h>
#include <dcl/Kernel.h>
#include <dcl/ProgramBuildListener.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <ostream>
#include <vector>

#if USE_PROGRAM_BUILD_LISTENER
namespace dcld {

namespace detail {

class ProgramBuild {
public:
    ProgramBuild(
            Program& program,
            const std::vector<dcl::Device *>& devices,
            const std::shared_ptr<dcl::ProgramBuildListener>& listener) :
        _program(program), _devices(devices), _listener(listener) { }
    virtual ~ProgramBuild() { }

    void onComplete() {
        std::vector<cl_build_status> buildStatus;

        /* Query program build status */
        buildStatus.reserve(_devices.size());
        for (auto device : _devices) {
            auto deviceImpl = dynamic_cast<Device *>(device);
            /* Device has been checked in dcld::Program::build */
            assert(deviceImpl != nullptr);
            buildStatus.push_back(
                    static_cast<cl::Program>(_program).getBuildInfo<CL_PROGRAM_BUILD_STATUS>(*deviceImpl));
        }

        _listener->onComplete(_devices, buildStatus);
    }

private:
    Program& _program;
    std::vector<dcl::Device *> _devices;
    std::shared_ptr<dcl::ProgramBuildListener> _listener;
};

} /* namespace detail */

} /* namespace dcld */

/* ****************************************************************************/

namespace {

/*!
 * \brief Callback for completion of program build
 */
void onProgramBuildComplete(cl_program object_, void *user_data) {
    std::unique_ptr<dcld::detail::ProgramBuild> programBuild(
            static_cast<dcld::detail::ProgramBuild *>(user_data));
    assert(programBuild != nullptr);
    programBuild->onComplete();
}

} /* unnamed namespace */

/* ****************************************************************************/
#endif

namespace dcld {

Program::Program(const std::shared_ptr<Context>& context,
        const char *source, size_t length) :
    _context(context)
{
    if (!context) throw cl::Error(CL_INVALID_CONTEXT);

    cl::Program::Sources sources;
    sources.push_back(std::make_pair(source, length));

    /* Create native program */
    _program = cl::Program(*_context, sources);
}

Program::Program(
        const std::shared_ptr<Context>& context,
        const std::vector<dcl::Device *>& devices,
        const std::vector<size_t>& lengths,
        const unsigned char **binaries,
        VECTOR_CLASS<cl_int> *binary_status) :
    _context(context)
{
    if (!context) throw cl::Error(CL_INVALID_CONTEXT);

    /* TODO Initialize program binaries */
    cl::Program::Binaries binaries_;
    assert(!"Program creation with binaries not implemented");

    /* TODO Use helper function for device conversion */
    /* convert devices */
    VECTOR_CLASS<cl::Device> nativeDevices;
    for (auto device : devices) {
        auto deviceImpl = dynamic_cast<Device *>(device);
        if (!deviceImpl) throw cl::Error(CL_INVALID_DEVICE);
        nativeDevices.push_back(deviceImpl->operator cl::Device());
    }

    /* Create native program */
    _program = cl::Program(*context, nativeDevices, binaries_, binary_status);
}

Program::~Program() { }

Program::operator cl::Program() const {
    return _program;
}

void Program::build(
        const std::vector<dcl::Device *>& devices,
        const char *options,
        const std::shared_ptr<dcl::ProgramBuildListener>& programBuildListener) {
    /* TODO Use helper function for device conversion */
    /* convert devices */
    VECTOR_CLASS<cl::Device> nativeDevices;
    for (auto device : devices) {
        auto deviceImpl = dynamic_cast<Device *>(device);
        if (!deviceImpl) throw cl::Error(CL_INVALID_DEVICE);
        nativeDevices.push_back(deviceImpl->operator cl::Device());
    }

#if USE_PROGRAM_BUILD_LISTENER
    /* start asynchronous program build */
    _program.build(nativeDevices, options, &onProgramBuildComplete,
            new detail::ProgramBuild(*this, devices, programBuildListener));
#else
    _program.build(nativeDevices, options);
#endif
}

void Program::createKernels(
        std::vector<std::shared_ptr<dcl::Kernel>>& kernels) {
    VECTOR_CLASS<cl::Kernel> nativeKernels;

    /* Create kernels */
    _program.createKernels(&nativeKernels);

    /* Wrap kernels */
    kernels.clear();
    kernels.reserve(nativeKernels.size());
    for (auto nativeKernel : nativeKernels) {
        kernels.push_back(std::make_shared<Kernel>(nativeKernel));
    }
}

const std::shared_ptr<Context>& Program::context() const {
    return _context;
}

} /* namespace dcld */
