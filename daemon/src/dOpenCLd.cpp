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
 * \file dOpenCLd.cpp
 *
 * \date 2011-01-16
 * \author Philipp Kegel
 *
 * Implementation of dOpenCL daemon
 */

#include "dOpenCLd.h"

#include "Device.h"
#include "Session.h"

#include <dcl/CommunicationManager.h>
#include <dcl/ComputeNode.h>
#include <dcl/Device.h>
#include <dcl/Host.h>
#include <dcl/Session.h>

#include <dcl/util/Logger.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <condition_variable>
#include <functional>
#include <iterator>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace {

/*!
 * \brief Extracts version information from an OpenCL version string.
 * The OpenCL version string is given as
 * OpenCL<space><miajor.major version<space><platform-specific information>
 *
 * \param[in]  version  OpenCL version string
 * \param[out] major    major version number
 * \param[out] minor    minor version number
 * \param[out] info     platform-specific information
 */
void getOpenCLVersion(
        const std::string& version,
        unsigned int& major,
        unsigned int& minor,
        std::string& info) {
    std::istringstream iss(version);
    std::string token;

    /*
     * Validate prefix
     */
    iss >> token; // read first token
    if (token.compare("OpenCL")) {
        /* version string does not start with 'OpenCL' */
        throw std::invalid_argument("Invalid OpenCL version string");
    }

    /*
     * Extract version numbers
     */
    if (iss.eof() || iss.get() != ' ') { // skip space
        throw std::invalid_argument("No version number found");
    }
    major = 0;
    minor = 0;
    iss >> major; // read major version number
    iss.ignore(); // skip period
    iss >> minor; // read minor version number

    if (iss.good()) {
        /*
         * Extract platform-specific information
         */
        if (iss.get() != ' ') { // skip space
            throw std::invalid_argument("Invalid version number");
        }
        std::getline(iss, info); // read platform-specific information (all remaining characters)
    }
}

cl::Platform getPlatform(const std::string *platformName) {
    VECTOR_CLASS<cl::Platform> platforms;

    /* The number of platform may be zero without throwing an error.
     * If an ICD loader is used, CL_PLATFORM_NOT_FOUND_KHR will be thrown. */
    cl::Platform::get(&platforms);

    /*
     * Select platform
     */
    auto platform = std::begin(platforms);
    while (platform != std::end(platforms)) {
        std::string version;
        unsigned int major, minor;
        std::string info;

        /*
         * Obtain platform version
         * dOpenCL daemon requires OpenCL version 1.1
         */
        platform->getInfo(CL_PLATFORM_VERSION, &version);
        getOpenCLVersion(version, major, minor, info);

        if (platformName) {
            std::string name;

            /*
             * select platform by name
             */
            platform->getInfo(CL_PLATFORM_NAME, &name);
            if ((name.find(*platformName) != std::string::npos)) {
                if (major < 1 || (major == 1 && minor < 1)) {
                    dcl::util::Logger << dcl::util::Warning
                            << "Platform '" << name << "' (version "
                            << version << ") does not support OpenCL 1.1 or higher."
                            << std::endl;
                    platform = std::end(platforms);
                }
                break;
            }
        } else {
            /*
             * select first appropriate platform
             */
            if ((major == 1 && minor >= 1) || major > 1) {
                break;
            }
        }

        ++platform;
    }

    if (platform == std::end(platforms)) {
        if (!platforms.empty()) {
            dcl::util::Logger << dcl::util::Error
                    << "No OpenCL 1.1 compliant platform found." << std::endl;
        }
        throw cl::Error(CL_PLATFORM_NOT_FOUND_KHR);
    }

    return *platform;
}

} /* unnamed namespace */

/* ****************************************************************************/

namespace dcld {

dOpenCLd::dOpenCLd(const std::string& url, const std::string *platform) :
	_communicationManager(dcl::ComputeNodeCommunicationManager::create(url)),
    _platform(getPlatform(platform)) {
    initializeDevices();
}

dOpenCLd::~dOpenCLd() {
}

void dOpenCLd::run() {
	std::lock_guard<std::mutex> lock(_interruptMutex);

    /* attach to connection manager */
    _communicationManager->setDaemon(this);
    _communicationManager->addConnectionListener(*this);

	_communicationManager->start();

    /* TODO Process connections in run method rather than in the callback methods */

	/* Suspend the calling (main) thread to prevent the daemon from exiting */
	_interrupt = false;
	while (!_interrupt) _interrupted.wait(_interruptMutex);

	_communicationManager->stop();

    /* detach from connection manager */
    _communicationManager->setDaemon();
    _communicationManager->removeConnectionListener(*this);

	dcl::util::Logger << dcl::util::Info
	        << "Shutting down dOpenCL daemon ..." << std::endl;
}

void dOpenCLd::terminate() {
	std::lock_guard<std::mutex> lock(_interruptMutex);
	/* Resume the main thread which will terminate the daemon */
	_interrupt = true;
	_interrupted.notify_all();
}

void dOpenCLd::initializeDevices() {
    VECTOR_CLASS<cl::Device> devices;

    /*
     * Initialize device list
     */
    _platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    dcl::util::Logger << dcl::util::Info
            << "Using platform '" << _platform.getInfo<CL_PLATFORM_NAME>() << "'\n"
            << "\tfound " << devices.size() << " device(s):\n";
    for (auto device : devices) {
        dcl::util::Logger << dcl::util::Info
            << "\t\t" << device.getInfo<CL_DEVICE_NAME>() << '\n';
        _devices.push_back(std::unique_ptr<Device>(new Device(device)));
    }
    dcl::util::Logger.flush();
}

/* ****************************************************************************
 * dOpenCL compute node/daemon API
 ******************************************************************************/

void dOpenCLd::getDevices(
        std::vector<dcl::Device *>& devices) const {
    devices.reserve(_devices.size());
    for (const auto& device : _devices) {
        devices.push_back(device.get());
    }
}

dcl::Session * dOpenCLd::getSession(
        const dcl::Host& host) const {
    auto i = _sessions.find(&host);
    return (i == std::end(_sessions)) ? nullptr : i->second.get();
}

/* ****************************************************************************
 * dOpenCL connection listener API
 ******************************************************************************/

bool dOpenCLd::connected(dcl::Host& host) {
	std::lock_guard<std::mutex> lock(_sessionsMutex);

	auto i = _sessions.find(&host);
	if (i == std::end(_sessions)) {
		/* create new session in list */
		bool created = _sessions.emplace(
		        &host, std::unique_ptr<Session>(new Session(_platform))).second;
		if (created) {
            dcl::util::Logger << dcl::util::Info
                    << "Session created (host='" << host.url() << "')" << std::endl;
		}
		return created;
	} else {
		/* TODO Handle reestablished connection */
	    return true;
	}
}

void dOpenCLd::disconnected(dcl::Host& host) {
	std::lock_guard<std::mutex> lock(_sessionsMutex);

	/* TODO Retain session for future reconnect
	 * Do not drop a session if its process has lost connection. Rather preserve
	 * the session to allow the process to reconnect and continue the session.
	 * Only drop sessions on request, or after a certain timeout has expired. */

	auto i = _sessions.find(&host);
	if (i != std::end(_sessions)) {
	    /* FIXME Handle incomplete events when destroying a session */
	    /* If a user event status is not set to completed the daemon hangs
	     * permanently within clReleaseContext when destroying the session.
	     * Presumably because the native OpenCL implementation waits for the
	     * event status to become CL_COMPLETE, which never happens for user
	     * events after the application on the client has been terminated. */
		_sessions.erase(i); // remove session from list

		dcl::util::Logger << dcl::util::Info
				<< "Session destroyed (host='" << host.url() << "')" << std::endl;
	}

	/* TODO Delete host when disconnected
	 * The ownership of the host must be clarified, as the host may be used by
	 * multiple connection listeners.
	 * Possible solution: return bool (see connected) to indicate if host should
	 * be deleted. If all listeners agree on deleting the host, it is deleted
	 * by the communication manager. */
}

bool dOpenCLd::connected(dcl::ComputeNode& computeNode) {
	/* TODO Implement dOpenCLd::connected for compute nodes */
	return false;
}

void dOpenCLd::disconnected(dcl::ComputeNode& computeNode) {
	/* TODO Implement dOpenCLd::disconnected for compute nodes */
}

} /* namespace dcld */
