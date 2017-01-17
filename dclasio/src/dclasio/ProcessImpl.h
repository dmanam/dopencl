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
 * \file ProcessImpl.h
 *
 * \date 2012-03-18
 * \author Philipp Kegel
 */

#ifndef PROCESSIMPL_H_
#define PROCESSIMPL_H_

#include "DCLAsioTypes.h"

#include <dclasio/message/Message.h>

#include <dcl/DataTransfer.h>
#include <dcl/DCLTypes.h>
#include <dcl/Process.h>

#include <boost/asio/ip/tcp.hpp>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

namespace dclasio {

namespace comm {

class DataDispatcher;
class DataStream;
class MessageDispatcher;
class message_queue;

} // namespace comm

/* ****************************************************************************/

/*!
 * \brief An implementation the dOpenCL Process interface.
 */
class ProcessImpl: public virtual dcl::Process {
public:
    enum class Type : char {
        HOST,
        COMPUTE_NODE
    };

    enum class ConnectionStatus : unsigned int {
        DISCONNECTED            = 0,
        MESSAGE_QUEUE_CONNECTED = 1,
        DATA_STREAM_CONNECTED   = 1 << 1,
        CONNECTED               = MESSAGE_QUEUE_CONNECTED | DATA_STREAM_CONNECTED
    };

    //! default response timeout is 3 seconds
    static const std::chrono::seconds DEFAULT_RESPONSE_TIMEOUT;


    /*!
     * \brief Creates a process instance from a message queue connection
     * The data stream will be attached to this process later on using setDataStream.
     *
     * \param[in]  id                   the process' ID
     *             The process ID has been obtained via the message queue
     * \param[in]  messageDispatcher    an associated message dispatcher
     * \param[in]  dataDispatcher       an associated data dispatcher
     * \param[in]  messageQueue         an associated message queue (owned by message dispatcher).
     *             The message queue also provides the remote endpoint associated with this process
     */
    ProcessImpl(
            dcl::process_id             id,
            comm::MessageDispatcher&    messageDispatcher,
            comm::DataDispatcher&       dataDispatcher,
            comm::message_queue&        messageQueue);
    /*!
     * \brief Creates a process instance for a remote endpoint.
     * This process is not connected and has no valid process ID.
     *
     * \param[in]  messageDispatcher    an associated message dispatcher
     * \param[in]  dataDispatcher       an associated data dispatcher
     * \param[in]  endpoint             the remote endpoint associated with this process
     */
    ProcessImpl(
            comm::MessageDispatcher&    messageDispatcher,
            comm::DataDispatcher&       dataDispatcher,
            const endpoint_type&        endpoint);
    virtual ~ProcessImpl();

    /*!
     * \brief Returns this process' ID
     * @return the process ID
     */
    dcl::process_id get_id() const;

    /*!
     * \brief Disconnects the process
     *
     * A connection is disconnected in two steps:
     * 1. destroy data stream
     * 2. disconnect message queue
     */
    void disconnect();
    /*!
     * \brief Tests, if the compute node is connected
     *
     * \return \c true, if the compute node is connected, otherwise \c false
     */
    bool isConnected();

    const std::string& url() const;

    void sendMessage(
            const message::Message& message) const;

    std::shared_ptr<dcl::DataTransfer> sendData(
            size_t      size,
            const void *ptr);
    std::shared_ptr<dcl::DataTransfer> receiveData(
            size_t  size,
            void *  ptr);

    /*!
     * \brief (Un)sets the processes data stream
     *
     * This method is called internally by the communication manager.
     *
     * \param[in]  dataStream   a connected data stream or \c nullptr
     */
    void setDataStream(
            comm::DataStream *dataStream = nullptr);

protected:
    /*!
     * \brief Waits until a given connection status has been reached, or a specified timeout expired
     *
     * \param[in] status    the connection status to wait for
     * \param[in] timeout   the maximum duration to wait for the specified connection status
     * \return \c false if timeout has been reached, otherwise \c true.
     */
    template<class Rep, class Period>
    bool awaitConnectionStatus(
            ConnectionStatus status,
            const std::chrono::duration<Rep, Period>& timeout) {
        std::lock_guard<std::recursive_mutex> lock(_connectionStatusMutex);
        while (_connectionStatus != status) {
            if (_connectionStatusChanged.wait_for(_connectionStatusMutex, timeout) == std::cv_status::timeout) {
                // timeout expired
                break;
            }
        }
        return (_connectionStatus == status);
    }

    /*!
     * \brief Waits until a given connection status or time point has been reached
     *
     * \param[in] status    the connection status to wait for
     * \param[in] deadline  the time to stop waiting
     * \return \c false if deadline has been reached, otherwise \c true.
     */
    template<class Clock, class Duration>
    bool awaitConnectionStatus(
            ConnectionStatus status,
            const std::chrono::time_point<Clock, Duration>& deadline) {
        std::lock_guard<std::recursive_mutex> lock(_connectionStatusMutex);
        while (_connectionStatus != status) {
            if (_connectionStatusChanged.wait_until(_connectionStatusMutex, deadline) == std::cv_status::timeout) {
                // deadline reached
                break;
            }
        }
        return (_connectionStatus == status);
    }

    /*!
     * \brief Returns the process' associated data stream
     * This method blocks until the data stream is available
     *
     * \return a data stream
     */
    comm::DataStream& getDataStream();

    dcl::process_id _pid; //!< process ID

    comm::MessageDispatcher& _messageDispatcher;
    comm::message_queue& _messageQueue; //!< message queue associated with this process; is owned by message dispatcher
    comm::DataDispatcher& _dataDispatcher;
    comm::DataStream *_dataStream; //!< data stream associated with this process; is owned by data dispatcher
    std::condition_variable_any _dataStreamReady;

    ConnectionStatus _connectionStatus; //!< Process connection status
    std::recursive_mutex _connectionStatusMutex; //!< Protects _connectionStatus and _dataStream
    std::condition_variable_any _connectionStatusChanged;

private:
    /* Processes must be non-copyable */
    ProcessImpl(
            const ProcessImpl&) = delete;
    ProcessImpl& operator=(
            const ProcessImpl&) = delete;

    mutable std::string _url; //!< Process URL
};

} /* namespace dclasio */

#endif /* PROCESSIMPL_H_ */
