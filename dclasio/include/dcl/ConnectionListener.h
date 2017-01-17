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
 * \file ConnectionListener.h
 *
 * \date 2011-08-07
 * \author Philipp Kegel
 *
 * C++ API declarations for dOpenCL communication layer
 */

#ifndef DCL_CONNECTIONLISTENER_H_
#define DCL_CONNECTIONLISTENER_H_

namespace dcl {

class Host;
class ComputeNode;

/* ****************************************************************************/

class ConnectionListener {
public:
    virtual ~ConnectionListener() { }

    /*!
     * \brief Callback for incoming host connections.
     *
     * \param[in]  host an incoming host connection
     * \return \c true, if the host connection has been accepted, otherwise \c false
     */
    virtual bool connected(
            Host& host) = 0;

    /*!
     * \brief Callback for closed host connections.
     *
     * \param[in]  host a closed host connection
     */
    virtual void disconnected(
            Host& host) = 0;

    /*!
     * \brief Callback for incoming compute node connections.
     *
     * \param[in]  compute node an incoming compute node connection
     * \return \c true, if the compute node connection has been accepted, otherwise \c false
     */
    virtual bool connected(
            ComputeNode& computeNode) = 0;

    /*!
     * \brief Callback for closed compute node connections.
     *
     * \param[in]  compute node a closed compute node connection
     */
    virtual void disconnected(
            ComputeNode& computeNode) = 0;
};

} /* namespace dcl */

#endif /* DCL_CONNECTIONLISTENER_H_ */
