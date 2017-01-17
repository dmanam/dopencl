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
 * \file MessageQueue.h
 *
 * \date 2014-03-10
 * \author Philipp Kegel
 */

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include "../ProcessImpl.h"

#include <dclasio/message/Message.h>

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#if !defined(NO_TEMPLATES)
#include <dcl/util/Logger.h>

#include <boost/asio/read.hpp>
#endif

#include <boost/asio/ip/tcp.hpp>

#include <boost/system/error_code.hpp>

#include <cstddef>
#include <memory>
#include <mutex>
#include <set>
#include <string>

namespace dclasio {

namespace message {

class Message;

} /* namespace message */

/* ****************************************************************************/

namespace comm {

class message_queue {
public:
    /*!
     * \brief Creates a message queue from a connected socket
     * \param[in]  socket   the socket
     */
    message_queue(
            const std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
            dcl::process_id pid);
    /*!
     * \brief Creates a message queue to the specified remote endpoint
     *
     * \param socket            a socket associated with a local endpoint
     * \param remote_endpoint   the remote process
     */
    message_queue(
            const std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
            boost::asio::ip::tcp::endpoint remote_endpoint);
    message_queue(
            message_queue&& other);
    virtual ~message_queue();

    dcl::process_id get_process_id() const { return _pid; }

    /*!
     * \brief Connects this message queue to a remote process
     * Moreover, the ID of the local process associated with this message queue is
     * send to the remote process.
     *
     * \param process_type  type of the local process
     * \param process_id    ID of the local process
     * \return the ID of the remote process, or 0 if the connection has been rejected
     */
    dcl::process_id connect(
            dclasio::ProcessImpl::Type process_type,
            dcl::process_id process_id);

    void disconnect();

    void send_message(
            const message::Message& message);

    template<typename MessageHandler>
    void recv_message(
            // TODO Provide message location as input
//            message::Message *& message,
            MessageHandler handler) {
        start_read_header(handler);
    }

private:
    typedef struct {
        message::Message::size_type size;
        message::Message::class_type type;
    } header_type; //!< message header comprising size of message body and message type ID

    void start_read_header();
    void handle_header(
            const boost::system::error_code& ec,
            size_t bytes_transferred);
    void start_read_message(
            message::Message::size_type size);
    void handle_message(
            const boost::system::error_code& ec,
            size_t bytes_transferred);

    template<typename MessageHandler>
    void start_read_header(
            MessageHandler handler) {
        // read header
        boost::asio::async_read(*_socket,
                boost::asio::buffer(&_message_header, sizeof(header_type)),
                [this, handler](const boost::system::error_code& ec, size_t bytes_transferred){
                        handle_header(ec, bytes_transferred, handler); });
    }

    template<typename MessageHandler>
    void handle_header(
            const boost::system::error_code& ec,
            size_t bytes_transferred,
            MessageHandler handler) {
        if (ec) {
            dcl::util::Logger << dcl::util::Error
                    << "Could not read message header: " << ec.message()
                    << std::endl;
            // FIXME Report error asynchronously
            handler(nullptr, ec);
        } else {
            // extract message size and type from header and start reading message body
            start_read_message(ntohl(_message_header.size), handler);
        }
    }

    template<typename MessageHandler>
    void start_read_message(
            message::Message::size_type size,
            MessageHandler handler) {
        dcl::util::Logger << dcl::util::Verbose
                << "Incoming message (size=" << size << ')' << std::endl;
        _message_buffer.resize(size);
        // read message
        boost::asio::async_read(*_socket,
                boost::asio::buffer(_message_buffer.begin(), _message_buffer.size()),
                [this, handler](const boost::system::error_code& ec, size_t bytes_transferred){
                        handle_message(ec, bytes_transferred, handler); });
    }

    template<typename MessageHandler>
    void handle_message(
            const boost::system::error_code& ec,
            size_t bytes_transferred,
            MessageHandler handler) {
        std::unique_ptr<message::Message> message;

        if (ec) {
            dcl::util::Logger << dcl::util::Error
                    << "Could not read message: " << ec.message()
                    << std::endl;
        } else {
            // create message of type _message_header.type from _message_buffer
            message.reset(message::createMessage(ntohl(_message_header.type)));
            dcl::util::Logger << dcl::util::Debug
                    << "Received message (size=" << _message_buffer.size()
                    << ", type=" << message->get_type() << ')'
                    << std::endl;

            message->unpack(_message_buffer); // restore message from buffer
        }

        /* FIXME Return message and report error asynchronously
        _socket->get_io_service().post([handler, message, ec] () { handler(message, ec); });
         */
        handler(message.get(), ec);
    }

    // TODO Store socket instance rather than smart pointer to instance
    std::shared_ptr<boost::asio::ip::tcp::socket> _socket; //!< I/O object for remote process
    boost::asio::ip::tcp::endpoint _remote_endpoint; //!< remote endpoint of message queue
    header_type _message_header;
    dcl::ByteBuffer _message_buffer;
#if defined(USE_SEND_BUFFER)
    header_type _send_header;
    dcl::ByteBuffer _send_buffer;
#endif
    /* FIXME Remove process ID from message_queue
     * This is a hack to avoid a message_queue-to-process_id lookup table in MessageDispatcher */
    dcl::process_id _pid;

    std::recursive_mutex _mutex; //! mutex to protect output channel
};

} /* namespace comm */
} /* namespace dclasio */

#endif /* MESSAGEQUEUE_H_ */
