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
 * \file Context.cpp
 *
 * \date 2011-04-06
 * \author Philipp Kegel
 */

#include "Context.h"

#include "ComputeNode.h"
#include "Device.h"
#include "Platform.h"

#include "dclicd/Error.h"
#include "dclicd/utility.h"

#include "dclicd/detail/ContextProperties.h"

#include <dclasio/message/CreateContext.h>
#include <dclasio/message/DeleteContext.h>
#include <dclasio/message/ErrorResponse.h>
#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>

#include <dcl/CLError.h>
#include <dcl/CLObjectRegistry.h>
#include <dcl/ComputeNode.h>
#include <dcl/ContextListener.h>
#include <dcl/DCLTypes.h>
#include <dcl/DCLException.h>
#include <dcl/Remote.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <ostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>


void _cl_context::init(
		const std::vector<cl_device_id>& devices) {
	std::map<dcl::ComputeNode *, std::vector<dcl::object_id>> computeNodeDeviceIds;

	assert(!devices.empty()); // device list must not be empty

	/*
	 * Obtain compute nodes' lists of device IDs
	 */
    for (auto device : devices) {
    	if (!device) throw dclicd::Error(CL_INVALID_DEVICE);
        /* Availability of device is checked by the compute node's OpenCL implementation */

    	computeNodeDeviceIds[&(device->remote().getComputeNode())].push_back(device->remote().getId());
    }

    try {
        std::vector<dcl::process_id> computeNodeIds;
        std::vector<std::pair<dcl::ComputeNode *, dclasio::message::CreateContext>> requests;

        /*
         * TODO Create list of compute node IDs
        computeNodeIds.reserve(computeNodeDeviceIds.size());
        for (auto deviceIds : computeNodeDeviceIds) {
            computeNodeIds.push_back(deviceIds.first->getId());
        }
         */

		/*
		 * Create and send requests
		 */
		for (auto deviceIds : computeNodeDeviceIds) {
			dcl::ComputeNode *computeNode = deviceIds.first;
			/* TODO Add context properties to request
			 * However, the platform (dOpenCL) must be stripped from the
			 * transmitted properties because it is not portable and should be
			 * replaced by the native platform selected on the compute node. */
			/* TODO Avoid copying 'create context' requests */
			dclasio::message::CreateContext request(_id, computeNodeIds, deviceIds.second);
			computeNode->sendRequest(request);

			requests.push_back(std::make_pair(computeNode, request)); // save pending request
		}

		/*
		 * Await responses from all compute nodes
		 */
		for (auto request : requests) {
			dcl::ComputeNode *computeNode = request.first;
			computeNode->awaitResponse(request.second);
			_computeNodes.push_back(computeNode); // update compute node list

			/* TODO Receive responses from *all* compute nodes, i.e., do not stop receipt on first failure */
		}

	    /* Register context as context listener */
	    getPlatform()->remote().objectRegistry().bind<dcl::ContextListener>(_id, *this);

		dcl::util::Logger << dcl::util::Info
				<< "Context created (ID=" << _id << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}

	_devices = devices;
}

cl_platform_id _cl_context::getPlatform() const {
    cl_platform_id platform;

    /*
     * Select platform
     */
    if (_properties) {
        platform = _properties->property<CL_CONTEXT_PLATFORM, cl_platform_id>();
    } else {
        /* Behavior is implementation defined, if properties is NULL */
        platform = _cl_platform_id::dOpenCL();
    }

    return platform;
}

_cl_context::_cl_context(
		const dclicd::detail::ContextProperties *properties,
		const std::vector<cl_device_id>& devices,
		void (*pfn_notify)(
				const char *errinfo,
				const void *private_info,
				size_t cb,
				void *user_data),
		void *user_data) :
        _properties(properties ? new dclicd::detail::ContextProperties(*properties) : nullptr),
        _pfnNotify(pfn_notify), _userData(user_data)
{
    if (devices.empty()) throw dclicd::Error(CL_INVALID_VALUE);
    if (pfn_notify == nullptr && user_data != nullptr) throw dclicd::Error(CL_INVALID_VALUE);

	/* Ensure that device are from platform dOpenCL */
    for (auto device : devices) {
        cl_platform_id devicePlatform;

        device->getInfo(CL_DEVICE_PLATFORM, sizeof(devicePlatform), &devicePlatform, nullptr);
        if (devicePlatform != getPlatform()) throw dclicd::Error(CL_INVALID_DEVICE);
    }

    init(devices);
}

_cl_context::_cl_context(
		const dclicd::detail::ContextProperties *properties,
		cl_device_type device_type,
		void (*pfn_notify)(
				const char *errinfo,
				const void *private_info,
				size_t cb,
				void *user_data),
		void *user_data) :
        _properties(properties ? new dclicd::detail::ContextProperties(*properties) : nullptr),
        _pfnNotify(pfn_notify), _userData(user_data)
{
	std::vector<cl_device_id> devices;
	
    if (pfn_notify == nullptr && user_data != nullptr) throw dclicd::Error(CL_INVALID_VALUE);

	/* obtain device list */
    getPlatform()->getDevices(device_type, devices);

    init(devices);
}

_cl_context::_cl_context(
		const dclicd::detail::ContextProperties *properties,
		const std::vector<cl_compute_node_WWU>& computeNodes,
		void (*pfn_notify)(
				const char *errinfo,
				const void *private_info,
				size_t cb,
				void *user_data),
		void *user_data) :
		_properties(properties ? new dclicd::detail::ContextProperties(*properties) : nullptr),
		_pfnNotify(pfn_notify), _userData(user_data)
{
	std::vector<cl_device_id> devices;

    if (computeNodes.empty()) { throw dclicd::Error(CL_INVALID_VALUE); }
    if (pfn_notify == nullptr && user_data != nullptr) { throw dclicd::Error(CL_INVALID_VALUE); }

	/* obtain device list */
	for (auto computeNode : computeNodes) {
		cl_platform_id nodePlatform;

		/* Ensure that compute node is associated with selected platform */
		computeNode->getInfo(CL_NODE_PLATFORM_WWU, sizeof(nodePlatform), &nodePlatform, nullptr);
		if (nodePlatform != getPlatform()) throw dclicd::Error(CL_INVALID_NODE_WWU);

		try {
	        std::vector<cl_device_id> nodeDevices;

			computeNode->getDevices(CL_DEVICE_TYPE_ALL, nodeDevices);
			devices.insert(std::end(devices), std::begin(nodeDevices), std::end(nodeDevices));
		} catch (dclicd::Error& err) {
			if (err.err() == CL_DEVICE_NOT_FOUND) {
				continue; // ignore CL_DEVICE_NOT_FOUND
			}

			throw; // rethrow error
		}
	}

    if (devices.empty()) throw dclicd::Error(CL_DEVICE_NOT_FOUND);

    init(devices);
}

_cl_context::~_cl_context() {
}

void _cl_context::destroy() {
	assert(_ref_count == 0);

	try {
		dclasio::message::DeleteContext deleteContext(_id);
		dcl::executeCommand(_computeNodes, deleteContext);

	    /* Remove this context from list of context listeners */
		getPlatform()->remote().objectRegistry().unbind<dcl::ContextListener>(_id);

		dcl::util::Logger << dcl::util::Info
				<< "Context deleted (ID=" << _id << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

const std::vector<dcl::ComputeNode *>& _cl_context::computeNodes() const {
	return _computeNodes;
}

const std::vector<cl_device_id>& _cl_context::devices() const {
	return _devices;
}

void _cl_context::getInfo(
		cl_context_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) const {
	switch (param_name) {
	case CL_CONTEXT_REFERENCE_COUNT:
		dclicd::copy_info(_ref_count, param_value_size,
				param_value, param_value_size_ret);
		break;
	case CL_CONTEXT_NUM_DEVICES:
		dclicd::copy_info(static_cast<cl_uint>(_devices.size()),
				param_value_size, param_value, param_value_size_ret);
		break;
	case CL_CONTEXT_DEVICES:
		/* deep copy device list */
		dclicd::copy_info(_devices, param_value_size, param_value,
		        param_value_size_ret);
		break;
	case CL_CONTEXT_PROPERTIES:
	    if (_properties) {
            /* deep copy properties list */
            dclicd::copy_info(
                    _properties->size() * sizeof(cl_context_properties), _properties->data(),
                    param_value_size, param_value, param_value_size_ret);
	    } else {
	        dclicd::copy_info(0, nullptr, param_value_size, param_value, param_value_size_ret);
	    }
		break;
//    case CL_CONTEXT_D3D10_PREFER_SHARED_RESOURCES_KHR:
//        assert(!"cl_khr_d3d10_sharing not supported by dOpenCL");
//        break;
	default:
		throw dclicd::Error(CL_INVALID_VALUE);
	}
}

void _cl_context::getSupportedImageFormats(
        cl_mem_flags flags,
        cl_mem_object_type image_type,
        std::vector<cl_image_format>& imageFormats) const {
    /* TODO Implement _cl_context::getSupportedImageFormats */
//    assert(!"_cl_context::getSupportedImageFormats not implemented yet");
    /* Do nothing in order to return an empty list */
    imageFormats.clear();
}

bool _cl_context::hasDevice(cl_device_id device) const {
	return (std::find(std::begin(_devices), std::end(_devices), device) != std::end(_devices));
}

/*
 * Context listener APIs
 */

void _cl_context::onError(const char *errorInfo,
		const void *private_info, size_t cb) {
	if (_pfnNotify) {
		_pfnNotify(errorInfo, private_info, cb, _userData);
	}
}
