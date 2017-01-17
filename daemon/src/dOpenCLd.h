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
 * \file dOpenCLd.h
 *
 * \date 2011-01-16
 * \author Philipp Kegel
 *
 * dOpenCL daemon
 */

#ifndef DOPENCLD_H_
#define DOPENCLD_H_

#include <dcl/CommunicationManager.h>
#include <dcl/ComputeNode.h>
#include <dcl/ConnectionListener.h>
#include <dcl/Daemon.h>
#include <dcl/Device.h>
#include <dcl/Host.h>
#include <dcl/Session.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <condition_variable>
#include <map>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

namespace dcld {

class Device;
class Session;

/* ****************************************************************************/

class dOpenCLd: public dcl::Daemon, public dcl::ConnectionListener {
public:
    /*!
     * \brief Creates a daemon.
     *
     * \param[in]  url          URL which the daemon should bind to
     * \param[in]  platformName name of the platform which the daemon should attach to
     *             If platformName is \c NULL, the first platform available will be used.
     */
	dOpenCLd(
			const std::string& url,
			const std::string *platform = nullptr);
	virtual ~dOpenCLd();

	/*!
	 * \brief Does what the daemon does.
	 *
	 * This is a blocking method. Use terminate to return from this call.
	 */
	void run();

	/*!
	 * \brief Terminates the daemon.
	 *
	 * This method signals the daemon to stop gracefully.
	 */
	void terminate();

	/*
	 * compute node/daemon API
	 */
	void getDevices(
	        std::vector<dcl::Device *>& devices) const;

	dcl::Session * getSession(
	        const dcl::Host& host) const;

	/*
	 * dOpenCL connection listener API
	 */
	bool connected(
	        dcl::Host& host);
	void disconnected(
	        dcl::Host& host);
	bool connected(
	        dcl::ComputeNode& computeNode);
	void disconnected(
	        dcl::ComputeNode& computeNode);

private:
	/* Daemons must be non-copyable */
	dOpenCLd(
	        const dOpenCLd& rhs) = delete;
	dOpenCLd& operator=(
	        const dOpenCLd&) = delete;

	void initializeDevices();

	std::unique_ptr<dcl::ComputeNodeCommunicationManager> _communicationManager;

    cl::Platform _platform; //!< Selected platform; default is first platform
    std::vector<std::unique_ptr<Device>> _devices; //!< Device list

	bool _interrupt;
	std::mutex _interruptMutex;
	std::condition_variable_any _interrupted;

	std::map<const dcl::Host *, std::unique_ptr<Session>> _sessions; //!< Sessions
	mutable std::mutex _sessionsMutex;
};

} /* namespace dcld */

#endif /* DOPENCLD_H_ */
