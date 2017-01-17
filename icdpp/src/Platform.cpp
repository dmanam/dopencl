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
 * @file Platform.cpp
 *
 * @date 2011-04-12
 * @author Philipp Kegel
 */

#include "Platform.h"

#include "ComputeNode.h"
#include "Device.h"

#include "dclicd/Error.h"
#include "dclicd/utility.h"

#include <dcl/CommunicationManager.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <boost/algorithm/string/trim.hpp>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#define DEFAULT_DCL_NODE_FILE "dcl.nodes"

namespace {

void readNodeList(
        std::vector<std::string>& nodeList) {
    const char *filename;

    nodeList.clear();

    /*
     * Determine node file
     */
    filename = getenv("DCL_NODE_FILE");
    if (!filename || strlen(filename) == 0) {
        filename = DEFAULT_DCL_NODE_FILE;
    }

    std::ifstream ifs(filename); // open node file
    if (ifs.good()) {
        /*
         * Parse node file
         */
        do {
            std::string str;
            size_t pos;

            std::getline(ifs, str);
            /* stop reading file on failure, but not if EOF has been reached */
            if (ifs.fail()) {
                /* failbit is also set if EOF has been reached before any
                 * characters have been extracted; this is no error */
                if (ifs.eof()) break;

                /* true error */
                dcl::util::Logger << dcl::util::Error
                        << "Error reading node file '" << filename << '\'' << std::endl;
                break;
            }

            pos = str.find_first_of('#');                  // find comment
            if (pos != std::string::npos) str.resize(pos); // remove comment
            boost::trim(str); // remove leading and trailing white spaces
            if (!str.empty()) nodeList.push_back(str);
        } while (ifs.good());

        ifs.close(); // close node file
    } else {
        dcl::util::Logger << dcl::util::Warning
                << "Node file '" << filename << "' not found" << std::endl;
    }
}

} /* unnamed namespace */

/******************************************************************************/

void _cl_platform_id::get(
		std::vector<cl_platform_id>& platforms) {
	platforms.assign(1, dOpenCL());
}

_cl_platform_id::_cl_platform_id(
		const std::string& profile,
		const std::string& version,
		const std::string& name,
		const std::string& vendor,
		const std::string& extensions) :
		_profile(profile), _version(version), _name(name),
			_vendor(vendor), _extensions(extensions),
			_computeNodesInitialized(false) {
	_communicationManager.reset(dcl::HostCommunicationManager::create());
	_communicationManager->start();
}

_cl_platform_id::~_cl_platform_id() {
	/*
	 * Release all compute nodes
	 */
	{
		std::lock_guard<std::mutex> lock(_computeNodesMutex);
		for (auto computeNode : _computeNodes) {
			delete computeNode;
		}
		_computeNodes.clear();
	}

	/* shutdown connection manager */
	_communicationManager->stop();
}

cl_compute_node_WWU _cl_platform_id::createComputeNode(
		const std::string& url,
		void (*pfn_notify)(
				cl_compute_node_WWU, cl_int, void *),
		void *user_data) {
	/* TODO Detect redundant connection
	 * Ensure that no redundant instance of _cl_compute_node_WWU is created for
	 * the same dcl::ComputeNode */
	auto remote = _communicationManager->createComputeNode(url);
	auto computeNode = new _cl_compute_node_WWU(
	        this, *remote, pfn_notify, user_data);

	/*
	 * Add compute node to list
	 */
	{
		std::lock_guard<std::mutex> lock(_computeNodesMutex);
		bool inserted = _computeNodes.insert(computeNode).second;
		assert(inserted);
	}

	return computeNode;
}

void _cl_platform_id::destroyComputeNode(cl_compute_node_WWU computeNode) {
	auto remote = &computeNode->remote();

	/*
	 * Remove compute node from list
	 */
	{
		std::lock_guard<std::mutex> lock(_computeNodesMutex);
		std::set<cl_compute_node_WWU>::size_type numErased;
		numErased = _computeNodes.erase(computeNode);
		assert(numErased == 1);
	}

	_communicationManager->destroyComputeNode(remote);

	delete computeNode;
}

void _cl_platform_id::getComputeNodes(
		std::vector<cl_compute_node_WWU>& computeNodes) {
    std::lock_guard<std::mutex> lock(_computeNodesMutex);

	/* ensure that compute nodes from configuration file have been added to
	 * compute node list */
	initComputeNodes();

	/* copy compute node list */
	computeNodes.assign(std::begin(_computeNodes), std::end(_computeNodes));
}

void _cl_platform_id::getDevices(
		cl_device_type type,
		std::vector<cl_device_id>& devices) {
	std::vector<cl_compute_node_WWU> computeNodes;

	/* Device type is validated in _cl_compute_node_WWU::getDevices */

	getComputeNodes(computeNodes);

	devices.clear(); /* clear output list */

	for (auto computeNode : computeNodes) {
		std::vector<cl_device_id> nodeDevices;

		try {
			computeNode->getDevices(type, nodeDevices);
			devices.insert(std::end(devices), std::begin(nodeDevices), std::end(nodeDevices));
		} catch (dclicd::Error& err) {
			if (err.err() == CL_DEVICE_NOT_FOUND) {
				continue; /* ignore CL_DEVICE_NOT_FOUND */
			}

			throw; // rethrow error
		}
	}

	if (devices.empty()) throw dclicd::Error(CL_DEVICE_NOT_FOUND);
}

void _cl_platform_id::getInfo(
		cl_platform_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) const {
    switch (param_name) {
    case CL_PLATFORM_PROFILE:
    	dclicd::copy_info(_profile, param_value_size, param_value, param_value_size_ret);
        break;
    case CL_PLATFORM_VERSION:
    	dclicd::copy_info(_version, param_value_size, param_value, param_value_size_ret);
        break;
    case CL_PLATFORM_NAME:
    	dclicd::copy_info(_name, param_value_size, param_value, param_value_size_ret);
        break;
    case CL_PLATFORM_VENDOR:
    	dclicd::copy_info(_vendor, param_value_size, param_value, param_value_size_ret);
        break;
    case CL_PLATFORM_EXTENSIONS:
    	dclicd::copy_info(_extensions, param_value_size, param_value, param_value_size_ret);
        break;
    default:
    	throw dclicd::Error(CL_INVALID_VALUE);
    }
}

void _cl_platform_id::unloadCompiler() const {
	/* TODO Implement _cl_platform_id::unloadCompiler */
}

dcl::HostCommunicationManager& _cl_platform_id::remote() const {
    return *_communicationManager;
}

void _cl_platform_id::initComputeNodes() {
    std::vector<std::string> urls;

    if (_computeNodesInitialized) return; // only process node file once

	readNodeList(urls);

	if (!urls.empty()) {
        std::vector<dcl::ComputeNode *> computeNodes;

		try {
			/* Create compute node proxies */
			_communicationManager->createComputeNodes(urls, computeNodes);

            /*
             * Create compute nodes and add to list
             */
            for (auto computeNode : computeNodes) {
                /* TODO Discard redundant connections
                 * Ensure that no redundant instance of _cl_compute_node_WWU is created for
                 * the same dcl::ComputeNode */
                bool inserted = _computeNodes.insert(new _cl_compute_node_WWU(this, *computeNode)).second;
                assert(inserted);
            }

			/* TODO Delete compute node proxies in case of an exception */
		} catch (const std::bad_alloc& err) {
			dcl::util::Logger << dcl::util::Error
					<< "Out of memory" << std::endl;
		} catch (const dcl::DCLException& err) {
			dcl::util::Logger << dcl::util::Error
					<< "dOpenCL error: " << err.what() << std::endl;
		}
	}

	_computeNodesInitialized = true;
}
