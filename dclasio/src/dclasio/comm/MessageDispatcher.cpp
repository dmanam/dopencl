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
 * \file MessageDispatcher.cpp
 *
 * \date 2014-03-10
 * \author Philipp Kegel
 */

#include "MessageDispatcher.h"

#include "../DCLAsioTypes.h"
#include "../ProcessImpl.h"

#include "ConnectionListener.h"
#include "MessageListener.h"
#include "MessageQueue.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <dcl/util/Logger.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/system/error_code.hpp>

#include <array>
#include <memory>
#include <mutex>
#include <ostream>
#include <thread>

namespace dclasio {
namespace comm {

MessageDispatcher::MessageDispatcher(
        dcl::process_id pid) : _work(_io_service), _pid(pid) {
}

MessageDispatcher::~MessageDispatcher() {
    stop();
}

message_queue * MessageDispatcher::create_message_queue(
        const endpoint_type& endpoint) {
    // create socket
    auto socket(std::make_shared<boost::asio::ip::tcp::socket>(_io_service));
    return add_message_queue(new message_queue(socket, endpoint));
}

void MessageDispatcher::destroy_message_queue(message_queue *msgq) {
    std::lock_guard<std::mutex> lock(_mutex);
    // remove message queue from list; implicitly calls destructor
    _message_queues.remove_if([msgq](const std::unique_ptr<message_queue>& entry){
            return entry.get() == msgq; });
}

void MessageDispatcher::add_connection_listener(
        connection_listener& listener) {
    std::lock_guard<std::mutex> lock(_listener_mutex);
    _connection_listeners.insert(&listener);
}

void MessageDispatcher::remove_connection_listener(
        connection_listener& listener) {
    std::lock_guard<std::mutex> lock(_listener_mutex);
    _connection_listeners.erase(& listener);
}

void MessageDispatcher::add_message_listener(
        message_listener& listener) {
    std::lock_guard<std::mutex> lock(_listener_mutex);
    _message_listeners.insert(&listener);
}

void MessageDispatcher::remove_message_listener(
        message_listener& listener) {
    std::lock_guard<std::mutex> lock(_listener_mutex);
    _message_listeners.erase(&listener);
}

void MessageDispatcher::bind(const endpoint_type& endpoint) {
    // create server socket
    _acceptor.reset(new boost::asio::ip::tcp::acceptor(_io_service));
    _acceptor->open(boost::asio::ip::tcp::v4());
    _acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    _acceptor->bind(endpoint);
}

void MessageDispatcher::start() {
    if (_acceptor) {
        try {
            _acceptor->listen();

            // start accept loop
            start_accept();
        } catch (const boost::system::system_error& err) {
            dcl::util::Logger << dcl::util::Error
                    << "Could not start message queue acceptor: " << err.what()
                    << std::endl;
        }
    }

    /* start worker thread
     * use lambda to resolve overloaded boost::asio::io_service::run */
    _worker = std::thread([this](){ _io_service.run(); });
}

void MessageDispatcher::stop() {
    _io_service.stop();
    if (_worker.joinable()) _worker.join();
}

void MessageDispatcher::start_accept() {
    auto socket(std::make_shared<boost::asio::ip::tcp::socket>(_io_service));
    // await incoming message queue connection
    _acceptor->async_accept(*socket,
            [this, socket](const boost::system::error_code& ec){
                    handle_accept(socket, ec); });
}

void MessageDispatcher::handle_accept(
        std::shared_ptr<boost::asio::ip::tcp::socket> socket,
        const boost::system::error_code& ec) {
    // TODO Handle error
    if (ec) {
        dcl::util::Logger << dcl::util::Error
                << "Could not accept message queue: " << ec.message()
                << std::endl;
        return;
    }

    auto buf(std::make_shared<dcl::ByteBuffer>());
    buf->resize(sizeof(dcl::process_id) + 2);

    // await authentication request from incoming data stream
    boost::asio::async_read(
            *socket,
            boost::asio::buffer(buf->begin(), buf->size()),
            [this, socket, buf](const boost::system::error_code& ec, size_t bytes_transferred) {
                    handle_approval(socket, buf, ec, bytes_transferred); });

    start_accept(); // await another incoming message queue connection
}

void MessageDispatcher::handle_approval(
        std::shared_ptr<boost::asio::ip::tcp::socket> socket,
        std::shared_ptr<dcl::ByteBuffer> buf,
        const boost::system::error_code& ec,
        size_t bytes_transferred) {
    if (ec) {
        dcl::util::Logger << dcl::util::Error
                << "Could not approve data stream: " << ec.message()
                << std::endl;
        return;
    }

    dcl::process_id pid;
    uint8_t proc_type; // process type
    uint8_t proto; // protocol (must be message queue)
    *buf >> pid >> proc_type >> proto;
    // TODO Ensure pid != 0
    ProcessImpl::Type process_type = static_cast<ProcessImpl::Type>(proc_type);
    // TODO Ensure message queue protocol

    // request connection approval
    std::unique_lock<std::mutex> lock(_listener_mutex);
    std::vector<connection_listener *> listeners(
            std::begin(_connection_listeners), std::end(_connection_listeners));
    lock.unlock();
    bool approved = false;
    for (auto listener : listeners) {
        if (listener->approve_message_queue(process_type, pid)) {
            approved = true;
            break;
        }
    }

    if (approved) {
        // message queue has been approved - keep it
        auto msgq = add_message_queue(new message_queue(socket, pid));

        *buf << _pid; // signal approval: return own process ID
        boost::asio::write(*socket, boost::asio::buffer(buf->begin(), buf->size()));
        dcl::util::Logger << dcl::util::Verbose
                << "Accepted message queue from process (pid=" << pid << ')'
                << std::endl;

        for (auto listener : listeners) {
            listener->message_queue_connected(*msgq, process_type, pid);
        }

        // start reading messages from queue
        start_read_message(*msgq);
    } else {
        // signal reject: return process ID 0
        *buf << dcl::process_id(0);
        boost::asio::write(*socket, boost::asio::buffer(buf->begin(), buf->size()));
        dcl::util::Logger << dcl::util::Error
                << "Rejected message queue from process (pid=" << pid << ')'
                << std::endl;
    }
}

void MessageDispatcher::start_read_message(
        message_queue& msgq) {
    msgq.recv_message(
            [this, &msgq] (message::Message *message, const boost::system::error_code& ec) {
                    handle_message(msgq, message, ec); });
}

/*!
 * \brief Callback for incoming messages
 */
void MessageDispatcher::handle_message(
        message_queue& msgq,
        message::Message *message,
        const boost::system::error_code& ec) {
    if (ec) {
        // TODO Handle errors
        std::lock_guard<std::mutex> lock(_listener_mutex);
        for (auto listener : _connection_listeners) {
            listener->message_queue_disconnected(msgq);
        }
    } else {
        assert(message && "No message");
        std::unique_lock<std::mutex> lock(_listener_mutex);
        for (auto listener : _message_listeners) {
            listener->message_received(msgq, *message);
        }
        lock.unlock();

        // read next message
        start_read_message(msgq);
    }
}

} /* namespace comm */
} /* namespace dclasio */
