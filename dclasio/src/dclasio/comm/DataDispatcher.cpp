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
 * \file DataDispatcher.cpp
 *
 * \date 2014-03-07
 * \author Philipp Kegel
 */

#include "DataDispatcher.h"

#include "../DCLAsioTypes.h"

#include "ConnectionListener.h"
#include "DataStream.h"
#include "DataTransferImpl.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>

#include <dcl/util/Logger.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/system/error_code.hpp>

#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <ostream>
#include <thread>
#include <utility>

namespace dclasio {

namespace comm {

DataDispatcher::DataDispatcher(
        dcl::process_id pid) : _work(_io_service), _pid(pid) {
}

DataDispatcher::~DataDispatcher() {
    stop();
}

DataStream * DataDispatcher::create_data_stream(
        const endpoint_type& endpoint) {
    // create socket
    auto socket(std::make_shared<boost::asio::ip::tcp::socket>(_io_service));
    return add_data_stream(new DataStream(socket, endpoint));
}

void DataDispatcher::destroy_data_stream(
        DataStream *data_stream) {
    std::lock_guard<std::mutex> lock(_mutex);
    // remove data stream from list; implicitly calls destructor
    _data_streams.remove_if([data_stream](const std::unique_ptr<DataStream>& entry){
            return entry.get() == data_stream; });
}

void DataDispatcher::add_connection_listener(
        connection_listener& listener) {
    std::lock_guard<std::mutex> lock(_mutex);
    _connection_listeners.insert(&listener);
}

void DataDispatcher::remove_connection_listener(
        connection_listener& listener) {
    std::lock_guard<std::mutex> lock(_mutex);
    _connection_listeners.erase(& listener);
}

void DataDispatcher::bind(
        const endpoint_type& endpoint) {
    // create server socket
    _acceptor.reset(new boost::asio::ip::tcp::acceptor(_io_service));
    _acceptor->open(boost::asio::ip::tcp::v4());
    _acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    _acceptor->bind(endpoint);
}

void DataDispatcher::start() {
    if (_acceptor) {
        try {
            _acceptor->listen();

            // initiate accept loop
            start_accept();
        } catch (const boost::system::system_error& err) {
            dcl::util::Logger << dcl::util::Error
                    << "Could not start data stream acceptor: "
                    << err.what()
                    << std::endl;
        }
    }

    /* start worker thread
     * use lambda to resolve overloaded boost::asio::io_service::run */
    _worker = std::thread([this](){ _io_service.run(); });
}

void DataDispatcher::stop() {
    _io_service.stop();
    if (_worker.joinable()) _worker.join();
}

void DataDispatcher::start_accept() {
    auto socket(std::make_shared<boost::asio::ip::tcp::socket>(_io_service));
    // await incoming data stream connection
    _acceptor->async_accept(*socket,
            [this, socket](const boost::system::error_code& ec){
                    handle_accept(socket, ec); });
}

void DataDispatcher::handle_accept(
        std::shared_ptr<boost::asio::ip::tcp::socket> socket,
        const boost::system::error_code& ec) {
    if (ec) {
        dcl::util::Logger << dcl::util::Error
                << "Could not accept data stream: "
                << ec.message() << std::endl;
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

    start_accept(); // await another incoming data stream connection
}

void DataDispatcher::handle_approval(
        std::shared_ptr<boost::asio::ip::tcp::socket> socket,
        std::shared_ptr<dcl::ByteBuffer> buf,
        const boost::system::error_code& ec,
        size_t bytes_transferred) {
    if (ec) {
        dcl::util::Logger << dcl::util::Error
                << "Could not approve data stream: "
                << ec.message() << std::endl;
        return;
    }

    dcl::process_id pid;
    uint8_t proc_type; // process type
    uint8_t proto; // protocol (must be data stream)
    *buf >> pid >> proc_type >> proto;
    // TODO Ensure pid != 0
    /* TODO Ensure process type
    ProcessImpl::Type process_type = static_cast<ProcessImpl::Type>(proc_type);
     */
    // TODO Ensure data stream protocol

    // request connection approval
    std::unique_lock<std::mutex> lock(_mutex);
    std::vector<connection_listener *> listeners(
            std::begin(_connection_listeners), std::end(_connection_listeners));
    lock.unlock();
    bool approved = false;
    for (auto listener : listeners) {
        if (listener->approve_data_stream(pid)) {
            approved = true;
            break;
        }
    }

    if (approved) {
        // data stream has been approved - keep it
        auto dataStream = add_data_stream(new DataStream(socket));

#if USE_DATA_STREAM_RESPONSE
        *buf << _pid; // signal approval: return own process ID
        boost::asio::write(*socket, boost::asio::buffer(buf->begin(), buf->size()));
#endif
        dcl::util::Logger << dcl::util::Verbose
                << "Accepted data stream from process (pid=" << pid << ')'
                << std::endl;

        for (auto listener : listeners) {
            listener->data_stream_connected(*dataStream, pid);
        }
    } else {
#if USE_DATA_STREAM_RESPONSE
        // signal reject: return process ID 0
        *buf << dcl::process_id(0);
        boost::asio::write(*socket, boost::asio::buffer(buf->begin(), buf->size()));
#endif
        dcl::util::Logger << dcl::util::Error
                << "Rejected data stream from process (pid=" << pid << ')'
                << std::endl;
    }
}

} // namespace comm

} // namespace dclasio
