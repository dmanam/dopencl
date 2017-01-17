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

/**
 * @file Kernel.cpp
 *
 * @date 2011-12-09
 * @author Philipp Kegel
 */

#include "Kernel.h"

#include "Device.h"
#include "Memory.h"
#include "Program.h"

#include <dcl/Binary.h>
#include <dcl/Device.h>
#include <dcl/Memory.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <set>
#include <vector>

namespace dcld {

Kernel::Kernel(const std::shared_ptr<Program>& program, const char *name) {
    if (!program) throw cl::Error(CL_INVALID_PROGRAM);

    _kernel = cl::Kernel(*program, name);
    _writeMemoryObjects.resize(_kernel.getInfo<CL_KERNEL_NUM_ARGS>());
}

Kernel::Kernel(const cl::Kernel& kernel) : _kernel(kernel) { }

Kernel::~Kernel() { }

Kernel::operator cl::Kernel() const {
	return _kernel;
}

void Kernel::getInfo(
		cl_kernel_info param_name,
		dcl::Binary& param) const {
	size_t param_value_size;
	/* Obtain kernel info using OpenCL C API to avoid unnecessary type
	 * conversions. */
	cl_int err = ::clGetKernelInfo(_kernel(), param_name, 0, nullptr, &param_value_size);
	if (err == CL_SUCCESS) {
        try {
            std::unique_ptr<char[]> param_value(new char[param_value_size]);
            err = ::clGetKernelInfo(_kernel(),
                    param_name,
                    param_value_size, param_value.get(),
                    nullptr);
            if (err != CL_SUCCESS) throw cl::Error(err);
            /* TODO Move pointer into dcl::Binary */
//            param.assign(param_value_size, std::move(param_value));
            param.assign(param_value_size, param_value.get());
        } catch (std::bad_alloc&) {
            throw cl::Error(CL_OUT_OF_RESOURCES);
        }
	} else {
		throw cl::Error(err);
	}
}

void Kernel::getWorkGroupInfo(
        const dcl::Device *device,
        cl_kernel_work_group_info param_name,
        dcl::Binary& param) const {
    auto deviceImpl = dynamic_cast<const Device *>(device);
    if (!deviceImpl) throw cl::Error(CL_INVALID_DEVICE);

    size_t param_value_size;
    /* Obtain kernel work group info using OpenCL C API to avoid unnecessary
     * type conversions. */
    cl_int err = ::clGetKernelWorkGroupInfo(
            _kernel(), deviceImpl->operator cl::Device()(), param_name, 0, nullptr, &param_value_size);
    if (err == CL_SUCCESS) {
        try {
            std::unique_ptr<char[]> param_value(new char[param_value_size]);
            err = ::clGetKernelWorkGroupInfo(
                    _kernel(),
                    deviceImpl->operator cl::Device()(),
                    param_name,
                    param_value_size, param_value.get(),
                    nullptr);
            if (err != CL_SUCCESS) throw cl::Error(err);
            /* TODO Move pointer into dcl::Binary */
//            param.assign(param_value_size, std::move(param_value));
            param.assign(param_value_size, param_value.get());
        } catch (std::bad_alloc&) {
            throw cl::Error(CL_OUT_OF_RESOURCES);
        }
    } else {
        throw cl::Error(err);
    }
}

void Kernel::setArg(cl_uint index,
        const std::shared_ptr<dcl::Memory>& memory) {
    auto memoryImpl = std::dynamic_pointer_cast<Memory>(memory);
    if (!memoryImpl) throw cl::Error(CL_INVALID_MEM_OBJECT);

    _kernel.setArg(index, memoryImpl->operator cl::Memory()());

    if (memoryImpl->isOutput()) { // memory object will be modified by kernel
        /* If a writable (CL_MEM_WRITE_ONLY, CL_MEM_READ_WRITE)
         * memory object is set as kernel argument, it is assumed
         * that the kernel will modify this memory object. */
        assert(index < _writeMemoryObjects.size());
        if (_writeMemoryObjects.size() <= index) {
            _writeMemoryObjects.resize(index + 1);
        }
        _writeMemoryObjects[index] = memoryImpl;
    }
}

void Kernel::setArg(cl_uint index, const cl::Sampler& sampler) {
	_kernel.setArg(index, sampler);
}

void Kernel::setArg(cl_uint index, size_t size, const void *argPtr) {
	/* Const-ness has to be stripped from the argument value, as the OpenCL C++
	 * binding requires a non-const argument value. Though, this argument is
	 * directly passed to the C API which argument is const. */
	_kernel.setArg(index, size, const_cast<void *>(argPtr));
}

std::vector<std::shared_ptr<Memory>> Kernel::writeMemoryObjects() {
    std::set<std::shared_ptr<Memory>> writeMemoryObjects;

    /* copy memory objects from argument list to set to remove duplicates */
    for (auto memoryObject : _writeMemoryObjects) {
        /* ignore empty (NULL) entries */
        if (memoryObject) writeMemoryObjects.insert(memoryObject);
    }

    /* return value optimization should kick-in here to avoid copying */
    return std::vector<std::shared_ptr<Memory>>(std::begin(writeMemoryObjects),
            std::end(writeMemoryObjects));
}

} /* namespace dcld */
