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
 * \file CommunicationManager.cpp
 *
 * \date 2011-10-26
 * \author Philipp Kegel
 */

#include "ComputeNodeCommunicationManagerImpl.h"
#include "HostCommunicationManagerImpl.h"

#include <dcl/CommunicationManager.h>
#include <dcl/DCLException.h>

#include <dcl/util/Logger.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

namespace {

dcl::util::Severity getSeverity() {
    const char *loglevel = getenv("DCL_LOG_LEVEL");

    if (loglevel) {
        if (strcmp(loglevel, "ERROR") == 0) {
            return dcl::util::Severity::Error;
        } else if (strcmp(loglevel, "WARNING") == 0) {
            return dcl::util::Severity::Warning;
        } else if (strcmp(loglevel, "INFO") == 0) {
            return dcl::util::Severity::Info;
        } else if (strcmp(loglevel, "DEBUG") == 0) {
            return dcl::util::Severity::Debug;
        } else if (strcmp(loglevel, "VERBOSE") == 0) {
            return dcl::util::Severity::Verbose;
        }
    }

    // use default log level
#ifndef NDEBUG
    return dcl::util::Severity::Debug;
#else
    return dcl::util::Severity::Info;
#endif
}

} /* unnamed namespace */

/* ****************************************************************************/

namespace dcl {

HostCommunicationManager * HostCommunicationManager::create() {
    // set up dOpenCL logger
    static std::ofstream dclLogFile("dcl_host.log");
    dcl::util::Logger.setOutput(dclLogFile);
    dcl::util::Logger.setLoggingLevel(getSeverity());
    dcl::util::Logger.setDefaultSeverity(dcl::util::Severity::Info);

    return new dclasio::HostCommunicationManagerImpl();
}

/* ****************************************************************************/

ComputeNodeCommunicationManager * ComputeNodeCommunicationManager::create(
        const std::string& url) {
    std::string host;
    dclasio::port_type port = dclasio::CommunicationManagerImpl::DEFAULT_PORT;
    dclasio::CommunicationManagerImpl::resolve_url(url, host, port);
    if (host.empty()) {
        throw dcl::InvalidArgument(DCL_INVALID_NODE, "Missing host name");
    }

    // generate name of dOpenCL log file
    std::string logFileName;
    {
        std::stringstream ss;

        /* TODO Log to system default location of files
         * Log to /var/log/dcld */
        ss << "dcl_" << host << ".log";
        ss >> logFileName;
    }
    // set up dOpenCL logger
    static std::ofstream dclLogFile(logFileName.c_str());
    dcl::util::Logger.setOutput(dclLogFile);
    dcl::util::Logger.setLoggingLevel(getSeverity());
    dcl::util::Logger.setDefaultSeverity(dcl::util::Severity::Info);

    return new dclasio::ComputeNodeCommunicationManagerImpl(host, port);
}

} /* namespace dcl */
