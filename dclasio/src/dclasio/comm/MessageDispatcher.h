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
 * \file MessageDispatcher.h
 *
 * \date 2014-03-10
 * \author Philipp Kegel
 */

#ifndef MESSAGEDISPATCHER_H_
#define MESSAGEDISPATCHER_H_

#include "../DCLAsioTypes.h"
#include "../ProcessImpl.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <boost/asio/io_service.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/system/error_code.hpp>

#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <utility>

namespace dclasio {
namespace comm {

class connection_listener;
class message_listener;
class message_queue;

/* ****************************************************************************/

/*!
 * \brief Processes incoming message queue connections and message transfers.
 */
class MessageDispatcher {
public:
    MessageDispatcher(
            dcl::process_id pid);
    virtual ~MessageDispatcher();

    /*!
     * \brief Creates a message queue associated to the specified remote process
     *
     * \param endpoint  endpoint of the remote process
     */
    message_queue * create_message_queue(
            const endpoint_type& endpoint);
    void destroy_message_queue(
            message_queue *msgq);

    void add_connection_listener(
            connection_listener& listener);
    void remove_connection_listener(
            connection_listener& listener);

    void add_message_listener(
            message_listener& listener);
    void remove_message_listener(
            message_listener& listener);

    void bind(
            const endpoint_type& endpoint);

    void start();
    void stop();

    // FIXME MessageDispatcher::start_read_message should be private
    void start_read_message(
            message_queue& msgq);

private:
    void start_accept();

    void handle_accept(
            std::shared_ptr<boost::asio::ip::tcp::socket> socket,
            const boost::system::error_code& ec);

    /*!
     * \brief Callback for message queue handshake
     * This method authenticates an incoming message queue and either notifies
     * registered connection listeners, or rejects and closes the message queue.
     *
     * \param[in]  socket   socket of incoming message queue
     * \param[in]  buf      identification received from message queue
     * \param[in]  ec       error code
     * \param[in]  bytes_transferred    number of received bytes
     */
    void handle_approval(
            std::shared_ptr<boost::asio::ip::tcp::socket> socket,
            std::shared_ptr<dcl::ByteBuffer> buf,
            const boost::system::error_code& ec,
            size_t bytes_transferred);

    /*!
     * \brief Callback for incoming messages
     */
    void handle_message(
            message_queue& msgq,
            message::Message *message,
            const boost::system::error_code& ec);

    /*!
     * \brief Adds a message queue to this message dispatcher
     *
     * \param[in]  args arguments for emplacing a unique pointer to the message queue
     * \return a raw pointer to the added message queue
     */
    template<typename ... Args>
    message_queue * add_message_queue(
            Args&& ... args) {
        std::lock_guard<std::mutex> lock(_mutex);
        _message_queues.emplace_back(std::forward<Args>(args) ...);
        return _message_queues.back().get();
    }

    boost::asio::io_service _io_service;
    boost::asio::io_service::work _work; //!< work flag for I/O service
    std::unique_ptr<boost::asio::ip::tcp::acceptor> _acceptor; //!< server socket

    dcl::process_id _pid;
    std::thread _worker;

    std::list<std::unique_ptr<message_queue>> _message_queues; //!< message queues managed by this message dispatcher
    std::unordered_set<connection_listener *> _connection_listeners; //!< connection listeners
    std::unordered_set<message_listener *> _message_listeners; //!< message listeners
    std::mutex _mutex; //!< protects message queue container
    std::mutex _listener_mutex; //!< protects connection and message listener containers

};

} /* namespace comm */
} /* namespace dclasio */

#endif /* MESSAGEDISPATCHER_H_ */
