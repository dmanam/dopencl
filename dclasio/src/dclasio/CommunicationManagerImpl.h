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
 * \file CommunicationManagerImpl.h
 *
 * \date 2011-10-26
 * \author Philipp Kegel
 */

#ifndef COMMUNICATIONMANAGERIMPL_H_
#define COMMUNICATIONMANAGERIMPL_H_

#include "DCLAsioTypes.h"

#include "comm/ConnectionListener.h"
#include "comm/DataDispatcher.h"
#include "comm/MessageDispatcher.h"
#include "comm/MessageListener.h"

#include <dcl/CommunicationManager.h>
#include <dcl/DCLTypes.h>

#include <boost/asio/io_service.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace dclasio {

class ComputeNodeImpl;
class ProcessImpl;

namespace comm {

class CLEventProcessor;
class DataStream;
class message_queue;

} /* namespace comm */

/* ****************************************************************************/

/*!
 * \brief An abstract base class for managing communication.
 *
 * Override methods {host|resourceManager|computeNode}Connection{Established|Lost|Approved}
 * to define behavior in derived class.
 */
class CommunicationManagerImpl:
        public virtual dcl::CommunicationManager,
        public comm::connection_listener,
        public comm::message_listener {
public:
    //! default connection timeout is 3 seconds
    static const std::chrono::seconds DEFAULT_CONNECTION_TIMEOUT;

    static const port_type DEFAULT_PORT = 25025;

    /*!
     * \brief Extracts host name and port from a string.
     *
     * \param[in]     url       the URL to resolve
     * \param[out]    hostName  a host name
     * \param[in,out] port      default port.
     * If the URL contains a port number, it is returned by this parameter.
     */
    static void resolve_url(
            const std::string&  url,
            std::string&        hostName,
            port_type&          port);

    static dcl::process_id create_process_id(
            const std::string& hostName,
            port_type port);


    CommunicationManagerImpl();
    CommunicationManagerImpl(
            const std::string&  host,
            port_type           port);
    virtual ~CommunicationManagerImpl();

    void bind(
            const std::string&  host,
            port_type           port);

    void start();
    void stop();

    /*!
     * \brief Creates compute nodes from the given URLs.
     *
     * WARNING: Compute nodes returned by this method may are not connected
     * and not owned by this communication manager yet.
     * The compute nodes have to be connected in order to pass ownership to the
     * communication manager.
     *
     * \param[in]  urls         remote endpoints
     * \param[out] computeNodes compute nodes
     */
    void createComputeNodes(
            const std::vector<std::string>& urls,
            std::vector<ComputeNodeImpl *>& computeNodes);

    void destroyComputeNode(
            ComputeNodeImpl *computeNode);

    /*!
     * \brief Returns the dOpenCL process that is associated with the specified ID.
     *
     * Subclasses may override this method in order to introduce additional
     * process types.
     *
     * \param[in]  pid  the process ID
     * \return the dOpenCL process that is associated with the specified process ID,
     *         or \c nullptr if no dOpenCL process is associated with the ID.
     */
    virtual ProcessImpl * get_process(
            dcl::process_id pid) const;

    ComputeNodeImpl * get_compute_node(
            dcl::process_id pid) const;

    void get_compute_nodes(
            const std::vector<dcl::process_id>& pids,
            std::vector<ComputeNodeImpl *>& computeNodes) const;

    /*
     * Connection listener API
     */
    bool approve_message_queue(
            ProcessImpl::Type process_type,
            dcl::process_id pid);

    void message_queue_connected(
            comm::message_queue& msgq,
            ProcessImpl::Type process_type,
            dcl::process_id pid);

    void message_queue_disconnected(
            comm::message_queue& msgq);

    bool approve_data_stream(
            dcl::process_id pid);

    void data_stream_connected(
            comm::DataStream& data_stream,
            dcl::process_id pid);

    /*
     * Message listener API
     */
    void message_received(
            comm::message_queue& msgq,
            const message::Message& message);

protected:
    /* Connection managers must be non-copyable */
    CommunicationManagerImpl(
            const CommunicationManagerImpl&) = delete;
    CommunicationManagerImpl& operator=(
            const CommunicationManagerImpl&) = delete;

    boost::asio::io_service _io_service;
    dcl::process_id _pid; //!< local process ID
    comm::MessageDispatcher _messageDispatcher;
    comm::DataDispatcher _dataDispatcher;

    std::unique_ptr<comm::CLEventProcessor> _clEventProcessor; //!< Processor for dOpenCL messages

    std::unordered_map<dcl::process_id, std::unique_ptr<ComputeNodeImpl>> _computeNodes;
    mutable std::recursive_mutex _connectionsMutex;
};

} /* namespace dclasio */

#endif /* COMMUNICATIONMANAGERIMPL_H_ */
