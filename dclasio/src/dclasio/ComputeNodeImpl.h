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
 * \file ComputeNodeImpl.h
 *
 * \date 2011-10-30
 * \author Philipp Kegel
 */

#ifndef COMPUTENODEIMPL_H_
#define COMPUTENODEIMPL_H_

#include "ProcessImpl.h"

#include "DCLAsioTypes.h"

#include "comm/DataStream.h"
#include "comm/MessageDispatcher.h"
#include "comm/MessageQueue.h"
#include "comm/ResponseBuffer.h"

#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>

#include <dcl/Binary.h>
#include <dcl/ComputeNode.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>
#include <dcl/Device.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl_wwu_dcl.h>
#endif

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace dclasio {

class DeviceImpl;

namespace comm {

class DataDispatcher;
// FIXME MessageDispatcher header required by start_read_message hack
//class MessageDispatcher;

} // namespace comm

/* ****************************************************************************/

class ComputeNodeImpl: public dcl::ComputeNode, public ProcessImpl {
public:
    /*!
     * \brief Connects to multiple compute nodes concurrently.
     *
     * This method updates the compute nodes' device lists.
     * This method works on a best-effort base: failed connection attempts are
     * ignored.
     *
     * \param[in]  computeNodes     the compute nodes to connect to
     * \param[in]  localProcessType type of the local process that is establishing the connection
     * \param[in]  pid              ID of the local process that is establishing the connection
     */
    static void connect(
            const std::vector<ComputeNodeImpl *>&   computeNodes,
            Type                                    localProcessType,
            dcl::process_id                         pid);

    /*!
     * \brief Awaits for multiple compute nodes to connect.
     *
     * \param[in]  computeNodes the compute nodes which connection to wait for
     */
    static void awaitConnection(
            const std::vector<ComputeNodeImpl *>& computeNodes);

    /*!
     * \brief Updates the device lists of multiple compute nodes.
     *
     * The device IDs are queried from multiple remote compute nodes.
     * This method is more efficient than \c updateDevices.
     *
     * \param[in]  computeNode  the compute node to query the device IDs from
     */
    static void updateDevices(
            const std::vector<ComputeNodeImpl *>& computeNodes);


    /*!
     * \brief Creates a compute node instance from a message queue connection
     *
     * \param[in]  id                   the compute node's process ID
     * \param[in]  messageDispatcher    an associated message dispatcher
     * \param[in]  dataDispatcher       an associated data dispatcher
     * \param[in]  messageQueue         an associated message queue (owned by message dispatcher)
     */
    ComputeNodeImpl(
            dcl::process_id             id,
            comm::MessageDispatcher&    messageDispatcher,
            comm::DataDispatcher&       dataDispatcher,
            comm::message_queue&        messageQueue);
    /*!
     * \brief Creates a compute node instance for a remote endpoint.
     * This compute node is not connected and has no valid process ID.
     *
     * \param[in]  messageDispatcher    an associated message dispatcher
     * \param[in]  dataDispatcher       an associated data dispatcher
     * \param[in]  endpoint             the remote endpoint associated with this process
     */
    ComputeNodeImpl(
            comm::MessageDispatcher&    messageDispatcher,
            comm::DataDispatcher&       dataDispatcher,
            const endpoint_type&        endpoint);
    virtual ~ComputeNodeImpl();

    /*!
     * \brief Connects to the compute node
     *
     * A connection is established in four steps:
     * 1. connect message queue
     * 2. connect data stream
     * 3. update device list
     *
     * \param[in]  localProcessType type of the local process that is establishing the connection
     * \param[in]  pid              ID of the local process that is establishing the connection
     */
    /* TODO Allow for asynchronous connect using a callback */
    void connect(
            Type localProcessType,
            dcl::process_id pid);

    void getDevices(
            std::vector<dcl::Device *>& devices);

    void getInfo(
            cl_compute_node_info_WWU    param_name,
            dcl::Binary&                param) const;

    void sendRequest(
            message::Request& request) const;

    std::unique_ptr<message::Response> awaitResponse(
            const message::Request&         request,
            message::Response::class_type   responseType);
    void awaitResponse(
            const message::Request& request);

    std::unique_ptr<message::Response> executeCommand(
            message::Request&               request,
            message::Response::class_type   responseType);
    void executeCommand(
            message::Request& request);

    comm::ResponseBuffer& responseBuffer();

private:
    /*!
     * \brief Connects to the compute node's message queue.
     *
     * \param[in]  localProcessType type of the local process that is establishing the connection
     * \param[in]  pid              ID of the local process that is establishing the connection
     * \param[in]  timeout          timeout for connection process; may be a duration or time point
     */
    template<typename timeout_type>
    void connectMessageQueue(
            Type localProcessType,
            dcl::process_id pid,
            const timeout_type& timeout) {
        std::lock_guard<std::recursive_mutex> lock(_connectionStatusMutex);
        // connect message queue to remote process
        _pid = _messageQueue.connect(localProcessType, pid);
        // TODO Connection asynchronously
        if (_pid != 0) {
            // FIXME Start reading messages automatically
            _messageDispatcher.start_read_message(_messageQueue);
            _connectionStatus = ConnectionStatus::MESSAGE_QUEUE_CONNECTED;
            _connectionStatusChanged.notify_all();
        } else {
            throw dcl::ConnectionException("Compute node '" + url() + "' refused connection");
        }
    }

    /*!
     * \brief Connects the compute node's data stream.
     *
     * \param[in]  pid      ID of the local process that is establishing the connection
     * \param[in]  timeout  a timeout; may be a duration or time point
     */
    template<typename timeout_type>
    void connectDataStream(
            dcl::process_id pid,
            const timeout_type& timeout) {
        bool connected = awaitConnectionStatus(ConnectionStatus::MESSAGE_QUEUE_CONNECTED, timeout);
        if (!connected) { /* session creation timed out */
            throw dcl::ConnectionException("Could not connect to compute node '" + url() + '\'');
        }

        {
            std::lock_guard<std::recursive_mutex> lock(_connectionStatusMutex);
            assert(_dataStream && "No data stream");
            if (_dataStream) {
                if (_dataStream->connect(pid) != 0) {
                    _connectionStatus = ConnectionStatus::CONNECTED;
                    _connectionStatusChanged.notify_all();
                } else {
                    throw dcl::ConnectionException("Compute node '" + url() + "' refused connection");
                }
            }
        }
    }

    /*!
     * \brief Updates the compute nodes device list.
     * The device IDs are queried from the remote compute node.
     */
    void updateDevices();
    /*!
     * \brief Updates the compute node's device list using a list of device IDs.
     *
     * \param[in]  deviceIds    the device IDs that should be used for the update
     */
    void updateDevices(
            const std::vector<dcl::object_id>& deviceIds);


    comm::ResponseBuffer _responseBuffer; //!< Response buffer

    std::unique_ptr<std::vector<std::unique_ptr<DeviceImpl>>> _devices; //!< Device list
    /*!
     * \brief A mutex associated with this compute node's devices list.
     *
     * This mutex must be recursive as getDeviceIDs locks it and will attempt to
     * lock it again when the device IDs have to be queried in updateDevices.
     */
    std::recursive_mutex _devicesMutex;
};

/* ****************************************************************************/

/*!
 * \brief Sends a message to multiple compute nodes.
 *
 * \param[in]  computeNodes a list of compute nodes to send a message to
 * \param[in]  message      the message to send
 */
void sendMessage(
        const std::vector<ComputeNodeImpl *>&   computeNodes,
        message::Message&                       message);

/*!
 * \brief Sends a request to multiple compute nodes.
 *
 * \param[in]  computeNodes a list of compute nodes to send a request to
 * \param[in]  request      the request to send
 */
void sendRequest(
        const std::vector<ComputeNodeImpl *>&   computeNodes,
        message::Request&                       request);

/*!
 * \brief Executes a command on multiple compute nodes.
 *
 * \param[in]  computeNodes a list of compute node to execute the command on
 * \param[in]  request      the request to send
 * \param[in]  responseType the expected type of responses
 * \param[out] responses    the responses returned by the compute nodes
 */
void executeCommand(
        const std::vector<ComputeNodeImpl *>&               computeNodes,
        message::Request&                                   request,
        message::Response::class_type                       responseType = message::DefaultResponse::TYPE,
        std::vector<std::unique_ptr<message::Response>> *   responses = nullptr);

} /* namespace dclasio */

#endif /* COMPUTENODEIMPL_H_ */
