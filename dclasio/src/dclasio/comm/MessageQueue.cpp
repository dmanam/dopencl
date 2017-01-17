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
 * \file MessageQueue.cpp
 *
 * \date 2014-03-10
 * \author Philipp Kegel
 */

#include "MessageQueue.h"

#include <dclasio/message/Message.h>

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <dcl/util/Logger.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <boost/asio/ip/tcp.hpp>

// TODO Replace htonl, ntohl by own, portable implementation
#include <netinet/in.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace dclasio {
namespace comm {

message_queue::message_queue(
        const std::shared_ptr<boost::asio::ip::tcp::socket>& socket, dcl::process_id pid) :
        _socket(socket), _pid(pid) {
    // TODO Ensure that socket is connected
    _remote_endpoint = _socket->remote_endpoint();
    /* Disable Nagle's algorithm on listening socket
     * Due to the RPC-style protocol of dOpenCL, short messages usually wait for
     * a response before the next message is send. Hence, waiting for another
     * short message to merge with the first one is pointless.
     * In particular, command status messages suffer from the high latency
     * introduced by Nagle's algorithm. */
    _socket->set_option(boost::asio::ip::tcp::no_delay(true));
}

message_queue::message_queue(
        const std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
        boost::asio::ip::tcp::endpoint remote_endpoint) :
        _socket(socket), _remote_endpoint(remote_endpoint), _pid(0) {
    assert(!socket->is_open()); // socket must not be connect
}

message_queue::message_queue(
        message_queue&& other) : _socket(std::move(other._socket)),
                _remote_endpoint(other._remote_endpoint), _pid(other._pid) {
}

message_queue::~message_queue() {
}

dcl::process_id message_queue::connect(
        ProcessImpl::Type process_type,
        dcl::process_id pid) {
    _socket->connect(_remote_endpoint); // connect socket
    /* Disable Nagle's algorithm on listening socket
     * Due to the RPC-style protocol of dOpenCL, short messages usually wait for
     * a response before the next message is send. Hence, waiting for another
     * short message to merge with the first one is pointless.
     * In particular, command status messages suffer from the high latency
     * introduced by Nagle's algorithm. */
    _socket->set_option(boost::asio::ip::tcp::no_delay(true));

    // send local process ID and type to remote process
    // TODO Encode message queue protocol
    dcl::ByteBuffer buf;
    buf << pid << uint8_t(process_type) << uint8_t(0);
    boost::asio::write(*_socket, boost::asio::buffer(buf.begin(), buf.size()));
    dcl::util::Logger << dcl::util::Verbose
            << "Sent process identification message for message queue (process type="
            << (process_type == ProcessImpl::Type::HOST ? "HOST" : "COMPUTE_NODE")
            << ", pid=" << pid << ')'
            << std::endl;

    // receive response
    buf.resize(sizeof(dcl::process_id));
    boost::asio::read(*_socket, boost::asio::buffer(buf.begin(), buf.size()));
    buf >> _pid;
    dcl::util::Logger << dcl::util::Verbose
            << "Received identification message response (pid=" << _pid << ')'
            << std::endl;

    return _pid;
}

void message_queue::disconnect() {
    _socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    _socket->close();
}

void message_queue::send_message(
        const message::Message& message) {
#if defined(USE_SEND_BUFFER)
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    _send_buffer.resize(0);
    message.pack(_send_buffer);
    _send_header.size = htonl(_send_buffer.size());
    _send_header.type = htonl(message.get_type());
    // send message header and body in one go
    boost::asio::write(*_socket, std::vector<boost::asio::const_buffer>( {
            boost::asio::const_buffer(&_send_header, sizeof(header_type)),
            boost::asio::const_buffer(_send_buffer.begin(), _send_buffer.size()) }));

    dcl::util::Logger << dcl::util::Verbose
            << "Sent message (size=" << _send_buffer.size() << ", type=" << message.get_type() << ')'
            << std::endl;
#else
    // send message length and type (4 + 4 Byte), followed by message body
    dcl::ByteBuffer buf;
    message.pack(buf); // pack message to determine length
    header_type header({ htonl(buf.size()), htonl(message.get_type()) });

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    // send message header and body in one go
    boost::asio::write(*_socket, std::vector<boost::asio::const_buffer>( {
            boost::asio::const_buffer(&header, sizeof(header_type)),
            boost::asio::const_buffer(buf.begin(), buf.size()) }));

    dcl::util::Logger << dcl::util::Verbose
            << "Sent message (size=" << buf.size() << ", type=" << message.get_type() << ')'
            << std::endl;
#endif
}

void message_queue::start_read_header() {
    // read header
    boost::asio::async_read(*_socket,
            boost::asio::buffer(&_message_header, sizeof(header_type)),
            [this](const boost::system::error_code& ec, size_t bytes_transferred){
                    handle_header(ec, bytes_transferred); });
}

void message_queue::handle_header(
        const boost::system::error_code& ec,
        size_t bytes_transferred) {
    if (ec) {
        dcl::util::Logger << dcl::util::Error
                << "Could not read message header: " << ec.message()
                << std::endl;
        // FIXME Report error via MessageHandler
    } else {
        // extract message size and type from header and start reading message body
        start_read_message(ntohl(_message_header.size));
    }
}

void message_queue::start_read_message(
        message::Message::size_type size) {
    dcl::util::Logger << dcl::util::Verbose
            << "Incoming message (size=" << size << ')' << std::endl;
    _message_buffer.resize(size);
    // read message
    boost::asio::async_read(*_socket,
            boost::asio::buffer(_message_buffer.begin(), _message_buffer.size()),
            [this](const boost::system::error_code& ec, size_t bytes_transferred){
                    handle_message(ec, bytes_transferred); });
}

void message_queue::handle_message(
        const boost::system::error_code& ec,
        size_t bytes_transferred) {
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

    // FIXME Return message or report error via MessageHandler
}

} /* namespace comm */
} /* namespace dclasio */
