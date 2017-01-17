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
 * \date 2013-02-15
 * \author Philipp Kegel
 */

#include "Device.h"

#include <dcl/Binary.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <cstddef>
#include <memory>

namespace dcld {

Device::Device(
        const cl::Device& device) :
        _device(device) {
}

Device::~Device() {
}

Device::operator cl::Device() const {
    return _device;
}

void Device::getInfo(
        cl_device_info param_name,
        dcl::Binary& param) const {
    size_t param_value_size;
    /* Obtain device info using OpenCL C API to avoid unnecessary type
     * conversions. */
    cl_int err = ::clGetDeviceInfo(_device(), param_name, 0, nullptr, &param_value_size);
    if (err == CL_SUCCESS) {
        try {
            std::unique_ptr<char[]> param_value(new char[param_value_size]);
            err = ::clGetDeviceInfo(_device(), param_name, param_value_size,
                    param_value.get(), nullptr);
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

} /* namespace dcld */
