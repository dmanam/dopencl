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
 * @file EventProfilingInfo.cpp
 *
 * @date 2013-03-01
 * @author Philipp Kegel
 */

#include "EventProfilingInfo.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclicd {

namespace detail {

EventProfilingInfo::EventProfilingInfo(
        cl_ulong received,
        cl_ulong queued, cl_ulong submit, cl_ulong start, cl_ulong end) :
	_received(received), _submit(submit), _start(start), _end(end) {
    /*
     * Determine skew of compute node clock and device clock:
     * we assume that the time of receiving a command on the compute node
     * (CL_PROFILING_COMMAND_RECEIVED, compute node clock) is equal to the time
     * of enqueuing the command on the compute node (CL_PROFILING_COMMAND_QUEUED,
     * device clock). The difference of these time points (in nanoseconds) is
     * considered the skew of the compute node clock and the device clock.
     *
     * In the current implementation, the time of sending and receipt is not
     * exactly measured when sending or receiving a command, but rather when an
     * event is created. However, if the host's and compute nodes' clocks are in
     * sync (e.g., when using ntpd) the following invariants hold:
     * - QUEUED (host) < RECEIVED (compute node),
     * - RECEIVED (compute node) == QUEUED < SUBMIT < START < END (device).
     */
    _clockSkew = received - queued;
}

cl_ulong EventProfilingInfo::clockSkew() const {
    return _clockSkew;
}

cl_ulong EventProfilingInfo::received() const {
    return _received;
}

cl_ulong EventProfilingInfo::submit() const {
	return _submit + _clockSkew;
}

cl_ulong EventProfilingInfo::start() const {
	return _start + _clockSkew;
}

cl_ulong EventProfilingInfo::end() const {
	return _end + _clockSkew;
}

} /* namespace detail */

} /* namespace dclicd */
