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
 * \date 2011-04-06
 * \author Philipp Kegel
 */

#include "Device.h"

#include "ComputeNode.h"
#include "Platform.h"

#include "dclicd/Error.h"
#include "dclicd/utility.h"

#include <dcl/Binary.h>
#include <dcl/CLError.h>
#include <dcl/DCLException.h>
#include <dcl/Device.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cstddef>
#include <cstring>
#include <iterator>
#include <map>
#include <mutex>
#include <utility>


_cl_device_id::_cl_device_id(
        cl_compute_node_WWU computeNode,
        dcl::Device& device) :
    _computeNode(computeNode), _device(device) {
    /* TODO Increase reference count of associated compute node */
}

_cl_device_id::~_cl_device_id() {
    /* TODO Decrease reference count of associated compute node */
}

void _cl_device_id::getInfo(
		cl_device_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) const {
	switch (param_name) {
	case CL_DEVICE_COMPUTE_NODE_WWU:
		dclicd::copy_info(_computeNode, param_value_size, param_value,
		        param_value_size_ret);
		break;
	case CL_DEVICE_PLATFORM:
		_computeNode->getInfo(CL_NODE_PLATFORM_WWU, param_value_size,
		        param_value, param_value_size_ret);
		break;
	case CL_DEVICE_EXECUTION_CAPABILITIES:
	    /* Devices must not report CL_EXEC_NATIVE_KERNEL capability as native
	     * kernels cannot be executed by remote devices. */
	    /* TODO Unset CL_EXEC_NATIVE_KERNEL in value returned by remote device */
        dclicd::copy_info<cl_device_exec_capabilities>(CL_EXEC_KERNEL,
                param_value_size, param_value, param_value_size_ret);
	    break;
	case CL_DEVICE_AVAILABLE:
	    /* Device available is a transient property and thus cannot be cached */
		/* TODO Return false (device unavailable) if its compute node is not connected */
        try {
            dcl::Binary param;

            _device.getInfo(param_name, param);

            dclicd::copy_info(param, param_value_size,
                    param_value, param_value_size_ret);
        } catch (const dcl::CLError& err) {
            throw dclicd::Error(err);
        } catch (const dcl::IOException& err) {
            throw dclicd::Error(err);
        } catch (const dcl::ProtocolException& err) {
            throw dclicd::Error(err);
        }
        break;
	default:
	{
	    std::lock_guard<std::mutex> lock(_infoCacheMutex);

        auto i = _infoCache.find(param_name); // search device info in cache
        if (i == std::end(_infoCache)) {
            std::pair<cl_device_info, dcl::Binary> entry;

            /*
             * Cache miss: query device info from compute node
             */
            try {
                entry.first = param_name;
                _device.getInfo(param_name, entry.second);
            } catch (const dcl::CLError& err) {
                throw dclicd::Error(err);
            } catch (const dcl::IOException& err) {
                throw dclicd::Error(err);
            } catch (const dcl::ProtocolException& err) {
                throw dclicd::Error(err);
            }

            switch (param_name) {
            case CL_DEVICE_EXTENSIONS:
                /* TODO Devices must not report support for CL/GL or CL/DirectX interop */
                break;
            }

            i = _infoCache.insert(std::move(entry)).first;
        }

        dclicd::copy_info(i->second, param_value_size,
                param_value, param_value_size_ret);
	}
	    /* no break */
	}
}

dcl::Device& _cl_device_id::remote() const {
	return _device;
}
