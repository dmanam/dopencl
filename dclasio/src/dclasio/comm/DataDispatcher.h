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
 * \file DataDispatcher.h
 *
 * \date 2014-03-07
 * \author Philipp Kegel
 */

#ifndef DATADISPATCHER_H_
#define DATADISPATCHER_H_

#include "../DCLAsioTypes.h"

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
class DataStream;

/* ****************************************************************************/

/*!
 * Manages the threads for process pending data transfers of all data streams
 */
class DataDispatcher {
public:
    DataDispatcher(
            dcl::process_id pid);
    virtual ~DataDispatcher();

    /*!
     * \brief Creates a data stream that is processed by this data dispatcher
     * Use destroyDataStream to destroy the data stream
     *
     * \param endpoint  endpoint of the remote process
     * \return a data stream
     */
    DataStream * create_data_stream(
            const endpoint_type& endpoint);

    /*!
     * \brief Destroys a data stream that is processed by this data dispatcher
     *
     * \param[in]  dataStream   the data stream to destroy
     */
    void destroy_data_stream(
            DataStream *dataStream);

    void add_connection_listener(
            connection_listener& listener);
    void remove_connection_listener(
            connection_listener& listener);

    void bind(
            const endpoint_type& endpoint);

    void start();
    void stop();

private:
    /*!
     * \brief Asynchronously start listening for an incoming data stream
     */
    void start_accept();

    /*!
     * \brief Callback for incoming data stream
     *
     * \param[in]  socket   socket of incoming data stream
     * \param[in]  ec       error code
     */
    void handle_accept(
            std::shared_ptr<boost::asio::ip::tcp::socket> socket,
            const boost::system::error_code& ec);

    /*!
     * \brief Callback for data stream handshake
     * This method authenticates an incoming data stream and either notifies
     * registered connection listeners, or rejects and closes the data stream.
     *
     * \param[in]  socket   socket of incoming data stream
     * \param[in]  buf      identification received from data stream
     * \param[in]  ec       error code
     * \param[in]  bytes_transferred    number of received bytes
     */
    void handle_approval(
            std::shared_ptr<boost::asio::ip::tcp::socket> socket,
            std::shared_ptr<dcl::ByteBuffer> buf,
            const boost::system::error_code& ec,
            size_t bytes_transferred);

    /*!
     * \brief Adds a data stream to this data dispatcher
     *
     * \param[in]  args arguments for emplacing a unique pointer to the data stream
     * \return a raw pointer to the added data stream
     */
    template<typename ... Args>
    DataStream * add_data_stream(
            Args&& ... args) {
        std::lock_guard<std::mutex> lock(_mutex);
        _data_streams.emplace_back(std::forward<Args>(args) ...);
        return _data_streams.back().get();
    }

    boost::asio::io_service _io_service;
    boost::asio::io_service::work _work; //!< work flag for I/O service
    std::unique_ptr<boost::asio::ip::tcp::acceptor> _acceptor; //!< server socket

    dcl::process_id _pid;
    std::thread _worker;

    std::list<std::unique_ptr<DataStream>> _data_streams; //!< data stream managed by this data dispatcher
    std::unordered_set<connection_listener *> _connection_listeners; //!< connection listeners
    std::mutex _mutex; //!< protects data stream and connection listener containers
};

} /* namespace comm */

} /* namespace dclasio */

#endif /* DATADISPATCHER_H_ */
