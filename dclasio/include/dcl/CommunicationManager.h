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
 * \file CommunicationManager.h
 *
 * \date 2011-08-07
 * \author Philipp Kegel
 *
 * C++ API declarations for dOpenCL communication layer
 */

#ifndef COMMUNICATIONMANAGER_H_
#define COMMUNICATIONMANAGER_H_

#include <string>
#include <vector>

namespace dcl {

class CLObjectRegistry;
class ComputeNode;
class ConnectionListener;
class Daemon;

/* ****************************************************************************/

/*!
 * \brief Virtual base class for communication managers.
 */
class CommunicationManager {
public:
    virtual ~CommunicationManager() {
    }

    /*!
     * \brief Starts the communication infrastructure
     */
    virtual void start() = 0;

    /*!
     * \brief Shuts down the communication infrastructure
     */
    virtual void stop() = 0;
};

/* ****************************************************************************/

/*!
 * \brief Communication manager for host processes.
 */
class HostCommunicationManager: public virtual CommunicationManager {
public:
    /*!
     * \brief Creates a new communication manager which is used for outgoing connections only.
     *
     * \return a communication manager instance
     */
    static HostCommunicationManager * create();

    virtual ~HostCommunicationManager() { }

    /* TODO Do not expose object registry in public API */
    virtual CLObjectRegistry& objectRegistry() = 0;

    /*!
     * \brief Creates a new compute node.
     *
     * A connection to this compute is created automatically.
     *
     * \param[in]  url  the compute node's URL
     * \return a compute node instance
     */
    virtual ComputeNode * createComputeNode(
            const std::string& url) = 0;

    /*!
     * \brief Creates multiple compute nodes concurrently.
     *
     * This method is more efficient for creating multiple compute nodes than
     * creating each compute node separately using createComputeNode as the
     * creation process is parallelized.
     *
     * \param[in]  urls         the compute nodes' URLs
     * \param[out] computeNodes compute node instances
     */
    virtual void createComputeNodes(
            const std::vector<std::string>& urls,
            std::vector<ComputeNode *>&     computeNodes) = 0;

    /*!
     * \brief Destroys a compute node.
     *
     * \param[in]  computeNode  the compute node to destroy
     */
    virtual void destroyComputeNode(
            ComputeNode *computeNode) = 0;
};

/* ****************************************************************************/

/*!
 * \brief Communication manager for compute node processes.
 */
class ComputeNodeCommunicationManager: public virtual CommunicationManager {
public:
    /*!
     * \brief Creates a new communication manager which is accessible via a given URL.
     *
     * \param[in]  url  URL of the communication manager
     * \return a communication manager instance
     */
    static ComputeNodeCommunicationManager * create(
            const std::string& url);

    virtual ~ComputeNodeCommunicationManager() { }

    virtual void setDaemon(
            Daemon *daemon = nullptr) = 0;

    /* TODO Connections should be established transparently in dOpenCL.
     * No connection listener should be required in ICD or daemon */
    virtual bool addConnectionListener(
            ConnectionListener& listener) = 0;
    virtual bool removeConnectionListener(
            ConnectionListener& listener) = 0;
};

} /* namespace dcl */

#endif /* COMMUNICATIONMANAGER_H_ */
