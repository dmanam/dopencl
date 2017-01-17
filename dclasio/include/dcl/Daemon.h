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
 * \file Daemon
 *
 * \date 2013-10-18
 * \author Philipp Kegel
 *
 * C++ API declarations for dOpenCL communication layer
 */

#ifndef DCL_DAEMON_H_
#define DCL_DAEMON_H_

#include <vector>

namespace dcl {

class Device;
class Host;
class Session;

/* ****************************************************************************/

/*!
 * An interface class to access a compute node's devices and sessions.
 *
 * This interface is a tentative solution to avoid implementing the current
 * ComputeNode interface on the compute node side as the interface contains some
 * unwanted compatibility methods (e.g., sendRequest, etc.).
 *
 * This interface should be integrated into the ComputeNode interface as the
 * daemon actually is a representation of a compute node.
 */
class Daemon {
public:
    virtual ~Daemon() { }

    /*!
     * \brief Returns a list of compute nodes available on the compute node
     *
     * \param[out] devices  a list of available compute nodes
     */
    virtual void getDevices(
            std::vector<Device *>& devices) const = 0;

    /*!
     * \brief Looks up the session that is associated with \c host
     *
     * \param[in]  host the host which session should be returned
     * \return the host's session, or \c NULL if no session is associated with \c host
     */
    virtual Session * getSession(
            const Host& host) const = 0;
};

} /* namespace dcl */

#endif /* DCL_DAEMON_H_ */
