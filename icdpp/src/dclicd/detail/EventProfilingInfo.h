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
 * @file EventProfilingInfo.h
 *
 * @date 2013-03-01
 * @author Philipp Kegel
 */

#ifndef EVENTPROFILINGINFO_H_
#define EVENTPROFILINGINFO_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclicd {

namespace detail {

class EventProfilingInfo {
public:
    /**
     * @brief Creates an event profiling info set.
     *
     * The input times are provided by two different clocks (compute node and
     * device clock). Internally the skew of these clocks is computed and added
     * to all values. Thus, the values returned by this event profiling info are
     * adjusted to the compute node clock.
     *
     * @param[in]  received time of receiving/enqueuing (compute node clock) on
     *                      the compute node
     * @param[in]  queued   time of enqueuing the command (device clock)
     * @param[in]  submit   time of submitting the command (device clock)
     * @param[in]  start    time of starting the command (device clock)
     * @param[in]  end      time of finishing the command (device clock)
     */
    EventProfilingInfo(
            cl_ulong received,
            cl_ulong queued,
            cl_ulong submit,
            cl_ulong start,
            cl_ulong end);

    /**
     * @brief Skew of the compute node clock and device clock.
     *
     * By subtracting this value from the times returned by this event
     * profiling info, these values are adjusted to the device clock.
     *
     * @return the clock skew in nanoseconds
     */
    cl_ulong clockSkew() const;

    cl_ulong received() const;
    cl_ulong submit() const;
    cl_ulong start() const;
    cl_ulong end() const;

private:
    cl_ulong _clockSkew; /**< Skew of compute node clock and device clock */
    cl_ulong _received;
    cl_ulong _submit;
    cl_ulong _start;
    cl_ulong _end;
};

} /* namespace detail */

} /* namespace dclicd */

#endif /* EVENTPROFILINGINFO_H_ */
