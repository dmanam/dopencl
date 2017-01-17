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
 * \file ComputeNodeCommunicationManagerImpl.h
 *
 * \date 2011-10-26
 * \author Philipp Kegel
 */

#ifndef COMPUTENODECOMMUNICATIONMANAGERIMPL_H_
#define COMPUTENODECOMMUNICATIONMANAGERIMPL_H_

#include "CommunicationManagerImpl.h"
#include "DCLAsioTypes.h"
#include "ProcessImpl.h"
#include "SmartCLObjectRegistry.h"

#include <dcl/CommunicationManager.h>
#include <dcl/ComputeNode.h>
#include <dcl/ConnectionListener.h>
#include <dcl/Daemon.h>
#include <dcl/DCLTypes.h>

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dclasio {

class ComputeNodeImpl;
class HostImpl;

namespace comm {

class CLRequestProcessor;
class message_queue;

} /* namespace comm */

/* ****************************************************************************/

class ComputeNodeCommunicationManagerImpl:
        public CommunicationManagerImpl,              // extend CommunicationManagerImpl
        public dcl::ComputeNodeCommunicationManager { // implement ComputeNodeCommunicationManager
public:
    ComputeNodeCommunicationManagerImpl(
            const std::string& host, port_type port);
    virtual ~ComputeNodeCommunicationManagerImpl();

    /*!
     * \brief Returns this communication manager's application object registry.
     *
     * \return an application object registry
     */
    SmartCLObjectRegistry& objectRegistry();

    void connectComputeNodes(
            const std::vector<ComputeNodeImpl *>& computeNodes);

    void setDaemon(
            dcl::Daemon *daemon = nullptr);
    dcl::Daemon * getDaemon() const;

    bool addConnectionListener(
            dcl::ConnectionListener& listener);
    bool removeConnectionListener(
            dcl::ConnectionListener& listener);

    /*!
     * \brief Returns the dOpenCL process that is associated with a specified process ID.
     *
     * This method overrides CommunicationManagerImpl::getProcess in order to also
     * search the list of hosts.
     */
    ProcessImpl * get_process(
            dcl::process_id pid) const;

    HostImpl * get_host(
            dcl::process_id pid) const;

    /*
     * Connection listener API
     */
    bool approve_message_queue(
            ProcessImpl::Type process_type,
            dcl::process_id process_id);

    void message_queue_connected(
            comm::message_queue& msgq,
            ProcessImpl::Type process_type,
            dcl::process_id process_id);

    void message_queue_disconnected(
            comm::message_queue& msgq);

    /*
     * Message listener API
     */
    void message_received(
            comm::message_queue& msgq,
            const message::Message& message);

private:
    void host_connected(
            comm::message_queue& msgq,
            dcl::process_id process_id);
    void compute_node_connected(
            comm::message_queue& msgq,
            dcl::process_id process_id);

    SmartCLObjectRegistry _objectRegistry; //!< Registry for application objects

    std::unique_ptr<comm::CLRequestProcessor> _clRequestProcessor; //!< Processor for command requests

    dcl::Daemon *_daemon;
    std::set<dcl::ConnectionListener *> _connectionListeners;
    std::mutex _connectionListenersMutex;

    std::unordered_map<dcl::process_id, std::unique_ptr<HostImpl>> _hosts;
};

} /* namespace dclasio */

#endif /* COMPUTENODECOMMUNICATIONMANAGERIMPL_H_ */
