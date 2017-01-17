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
 * @file ComputeNode.cpp
 *
 * @date 2011-04-06
 * @author Philipp Kegel
 */

#include "ComputeNode.h"

#include "Device.h"
#include "Platform.h"

#include "dclicd/Error.h"
#include "dclicd/utility.h"

#include <dcl/ComputeNode.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>


_cl_compute_node_WWU::_cl_compute_node_WWU(
		cl_platform_id platform,
		dcl::ComputeNode& remote,
		void (*pfn_notify)(
				cl_compute_node_WWU, cl_int, void *),
		void *user_data) :
	_platform(platform), _pfnNotify(pfn_notify), _userData(user_data),
            _remote(remote)
{
	assert(platform != nullptr);
    assert(pfn_notify == nullptr && "Compute node callback not implemented");

	if (user_data != nullptr && pfn_notify == nullptr) {
		throw dclicd::Error(CL_INVALID_VALUE);
	}

    initDeviceList();
}

_cl_compute_node_WWU::~_cl_compute_node_WWU() {
}

void _cl_compute_node_WWU::destroy() {
	assert(_ref_count == 0);
	/* TODO Avoid 'delete this' in _cl_compute_node_WWU::destroy */
	_platform->destroyComputeNode(this);
}

void _cl_compute_node_WWU::getDevices(
		cl_device_type type,
		std::vector<cl_device_id>& devices) const {
    switch (type) {
    case CL_DEVICE_TYPE_DEFAULT:
    case CL_DEVICE_TYPE_CPU:
    case CL_DEVICE_TYPE_GPU:
    case CL_DEVICE_TYPE_ACCELERATOR:
#if defined(CL_VERSION_1_2)
    case CL_DEVICE_TYPE_CUSTOM:
#endif // #if defined(CL_VERSION_1_2)
        /* device type is valid; return all devices of specified type */
        devices.clear(); // clear output list

        for (const auto& device : _devices) {
            cl_device_type deviceType;

            device->getInfo(CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceType, nullptr);
            if ((deviceType & type)) {
                devices.push_back(device.get());
            }
        }

        break;
    case CL_DEVICE_TYPE_ALL:
        devices.clear(); // clear output list
        /* just return all devices */
        for (const auto& device : _devices) {
            devices.push_back(device.get());
        }
        break;
    default:
        throw dclicd::Error(CL_INVALID_DEVICE_TYPE);
    }

	if (devices.empty()) {
		throw dclicd::Error(CL_DEVICE_NOT_FOUND);
	}
}

void _cl_compute_node_WWU::getInfo(
		cl_compute_node_info_WWU param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) const {
	switch (param_name) {
    case CL_NODE_AVAILABLE_WWU:
        /* TODO Return availability status of compute node
         * Compute nodes become unavailable if they loose connection (and so do
         * their devices). */
        assert(!"_cl_compute_node_WWU::getInfo not fully implemented yet");
        break;
	case CL_NODE_PLATFORM_WWU:
		dclicd::copy_info(_platform, param_value_size, param_value,
				param_value_size_ret);
		break;
	case CL_NODE_REFERENCE_COUNT_WWU:
		dclicd::copy_info(_ref_count, param_value_size, param_value,
				param_value_size_ret);
		break;
	case CL_NODE_URL_WWU:
        dclicd::copy_info(_remote.url(), param_value_size, param_value,
                param_value_size_ret);
	    break;
	/* TODO Return platform info of compute node's platform */
	case CL_NODE_PROFILE_WWU:
	case CL_NODE_VERSION_WWU:
	case CL_NODE_NAME_WWU:
	case CL_NODE_VENDOR_WWU:
	case CL_NODE_EXTENSIONS_WWU:
	    assert(!"_cl_compute_node_WWU::getInfo not fully implemented yet");
	    break;
	default:
    	throw dclicd::Error(CL_INVALID_VALUE);
	}
}

dcl::ComputeNode& _cl_compute_node_WWU::remote() const {
	return _remote;
}

void _cl_compute_node_WWU::initDeviceList() {
    std::vector<dcl::Device *> devices;

    _remote.getDevices(devices);

    _devices.clear();
    for (auto device : devices) {
        _devices.push_back(std::unique_ptr<_cl_device_id>(
                new _cl_device_id(this, *device)));
    }
}
