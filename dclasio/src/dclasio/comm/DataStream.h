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
 * \file DataStream.h
 *
 * \date 2014-03-07
 * \author Philipp Kegel
 */

#ifndef DATASTREAM_H_
#define DATASTREAM_H_

#include "DataTransferImpl.h"

#include <dcl/DCLTypes.h>

#include <boost/asio/ip/tcp.hpp>

#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace dclasio {

namespace comm {

// TODO Split DataStream into InputDataStream and OutputDataStream to model simplex connections and reduce code redundancy
/*!
 * \brief A data stream maintains a set of incoming and outgoing data transfers from/to a single remote process
 */
class DataStream {
public:
    typedef std::queue<std::shared_ptr<DataReceipt>, std::list<std::shared_ptr<DataReceipt>>> readq_type;
    typedef std::queue<std::shared_ptr<DataSending>, std::list<std::shared_ptr<DataSending>>> writeq_type;

    // TODO Accept rvalue reference rather than pointer to socket (requires Boost 1.47)
    /*!
     * \brief Creates a data stream from a connected socket
     * The data stream becomes owner of the socket.
     *
     * \param[in]  socket   the socket to use for the data stream
     */
    DataStream(
            const std::shared_ptr<boost::asio::ip::tcp::socket>& socket);
    /*!
     * \brief Creates a data stream to the specified remote endpoint
     *
     * \param socket            a socket associated with a local endpoint
     * \param remote_endpoint   the remote process
     */
    DataStream(
            const std::shared_ptr<boost::asio::ip::tcp::socket>& socket,
            boost::asio::ip::tcp::endpoint remote_endpoint);
    virtual ~DataStream();

    /*!
     * \brief Connects this data stream to its remote process
     * The ID of the local process associated with this data stream is send to
     * the remote process.
     *
     * \param[in]  pid  ID of the local process
     * \return the ID of the remote process, or 0 if the connection has been rejected
     */
    dcl::process_id connect(
            dcl::process_id pid);

    void disconnect();

    /*!
     * \brief Submits a data receipt for this data stream
     *
     * \param[in]  size  number of bytes to receipt
     * \param[in]  ptr   destination buffer for received bytes
     * \return a handle for the data receipt
     */
    std::shared_ptr<DataReceipt> read(
            size_t size,
            void *ptr);

    /*!
     * \brief Submits a data sending for this data stream
     *
     * \param[in]  size  number of bytes to send
     * \param[in]  ptr   source buffer for sent bytes
     * \return a handle for the data sending
     */
    std::shared_ptr<DataSending> write(
            size_t size,
            const void *ptr);

private:
    /* Data streams must be non-copyable */
    DataStream(
            const DataStream&) = delete;
    DataStream& operator=(
            const DataStream&) = delete;

    /*!
     * \brief Processes the next data transfer from the read queue.
     *
     * \param[in]  readq    list of incoming data transfers to process
     *             If \c readq is empty, it is filled with data transfers from the stream's internal read queue
     */
    void start_read(
            readq_type *readq = new readq_type());

    void handle_read(
            readq_type *readq,
            const boost::system::error_code& ec,
            size_t bytes_transferred);

    /*!
     * \brief Processes the next data transfer from the write queue.
     *
     * \param[in]  writeq   list of incoming data transfers to process
     *             If \c writeq is empty, it is filled with data transfers from the stream's internal write queue
     */
    void start_write(
            writeq_type *writeq = new writeq_type());

    void handle_write(
            writeq_type *writeq,
            const boost::system::error_code& ec,
            size_t bytes_transferred);

    // TODO Store socket instance rather than smart pointer
    std::shared_ptr<boost::asio::ip::tcp::socket> _socket; //!< I/O object for remote process
    boost::asio::ip::tcp::endpoint _remote_endpoint; //!< remote endpoint of data stream

    bool _receiving; //!< \c true, if currently receiving data, otherwise \c false
    bool _sending; //!< \c true, if currently sending data, otherwise \c false
    readq_type _readq; //!< pending data receipts
    std::mutex _readq_mtx; //!< protects read queue and flag
    writeq_type _writeq; //!< pending data sendings
    std::mutex _writeq_mtx; //!< protects write queue and flag
};

} // namespace comm

} // namespace dclasio

#endif /* DATASTREAM_H_ */
