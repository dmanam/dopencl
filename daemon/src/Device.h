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
 * \file Device.h
 *
 * \date 2013-02-15
 * \author Philipp Kegel
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <dcl/Binary.h>
#include <dcl/ComputeNode.h>
#include <dcl/DCLTypes.h>
#include <dcl/Device.h>
#include <dcl/Remote.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <stdexcept>

namespace dcld {

class Device: public dcl::Remote, public dcl::Device {
public:
    Device(
            const cl::Device& device);
    virtual ~Device();

    /*!
     * \brief Returns the wrapped OpenCL device.
     */
    operator cl::Device() const;

    void getInfo(
            cl_device_info param_name,
            dcl::Binary&   param) const;

    /* TODO Remove dcld::Device::getId */
    dcl::object_id getId() const { return _id; }
    /* TODO Remove dcld::Device::getComputeNode */
    dcl::ComputeNode& getComputeNode() const {
        throw std::runtime_error("dcl::Device::getComputeNode must not be called on compute node");
    }

private:
    cl::Device _device;
};

} /* namespace dcld */

#endif /* DEVICE_H_ */
