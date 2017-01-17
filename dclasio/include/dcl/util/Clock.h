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
 * \file Clock.h
 *
 * \date 2013-02-27
 * \author Philipp Kegel
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <chrono>

namespace dcl {

namespace util {

/*!
 * \brief A simple class for creating OpenCL-like time stamps from a global clock
 *
 * This clock should return monotonic time stamps which are valid on all nodes
 * of a distributed system used by dOpenCL. Thus, time stamps from different
 * nodes are comparable to each other, in order to, e.g., profile runtime of
 * commands or data transfers.
 */
class Clock {
public:
    Clock();
    virtual ~Clock();

    /*!
     * \brief Returns the number of elapsed nanoseconds since an arbitrary but fixed time point.
     *
     * \return a time point
     */
    cl_ulong getTime();

private:
    /*!
     * \brief Synchronizes this clock with a global clock
     *
     * This methods computes the clock skew of the system clock and a global
     * clock.
     */
    void sync();

    cl_ulong _clockSkew; //!< clock skew in nanoseconds
    std::chrono::time_point<std::chrono::high_resolution_clock> _start; //!< fixed time point to compute differences with
};

/* ****************************************************************************/

extern Clock clock;

} // namespace util

} // namespace dcl

#endif /* CLOCK_H_ */
