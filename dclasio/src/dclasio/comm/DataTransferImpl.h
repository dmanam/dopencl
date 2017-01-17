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
 * \file DataTransferImpl.h
 *
 * \date 2011-12-17
 * \author Philipp Kegel
 */

#ifndef DATATRANSFERIMPL_H_
#define DATATRANSFERIMPL_H_

#include <dcl/DataTransfer.h>
#include <dcl/DCLException.h>

#include <dcl/util/Clock.h>
#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <boost/system/error_code.hpp>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <vector>

namespace dclasio {

namespace comm {

struct Receive {
    typedef void * pointer_type;

    static void logFinish(
            size_t size,
            double latency,
            double bandwidth) {
        dcl::util::Logger << dcl::util::Debug
                << "Received " << size << " bytes\n"
                << "\tlatency: " << latency << " ms, bandwidth: " << bandwidth << " MB/s"
                << std::endl;

    }
};

struct Send {
    typedef const void * pointer_type;

    static void logFinish(
            size_t size,
            double latency,
            double bandwidth) {
        dcl::util::Logger << dcl::util::Debug
                << "Sent " << size << " bytes\n"
                << "\tlatency: " << latency << " ms, bandwidth: " << bandwidth << " MB/s"
                << std::endl;
    }
};

/* ****************************************************************************/

template<typename Operation>
class DataTransferImpl: public dcl::DataTransfer {
private:
    /*!
     * \brief Executes all of this data transfer's callbacks.
     */
    void triggerCallbacks() {
        std::lock_guard<std::mutex> lock(_mutex);
        // TODO Do not lock data transfer while executing callbacks
        for (auto& callback  : _callbacks) {
            callback(_status);
        }
    }

public:
    DataTransferImpl(
            size_t size, typename Operation::pointer_type ptr) :
            _size(size), _ptr(ptr),
            _submit(dcl::util::clock.getTime()), _start(0L), _end(0L),
            _status(CL_SUBMITTED) { }

    void setCallback(
            const std::function<void (cl_int)>& notify) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_status == CL_SUCCESS || _status < 0) { // data transfer is already finished
            /* The data transfer is already finished, but it is undefined whether
             * the callback list has already been processed or not.
             * Hence, if a new callback is added to the callback list it might
             * eventually be called or not.
             * Adding the new callback to the callback list and calling it might
             * result in a redundant execution of the callback.
             * Therefore, call the new callback immediately without adding it to the
             * callback list to ensure that it is only called once. */
            notify(_status);
        } else {
            /* Since we obtained the lock for the callback list while the data
             * transfer was still in progress we can be sure that the data
             * transfer will not be finished by onFinish, before we added the new
             * callback. */
            _callbacks.push_back(notify);
        }
    }

    cl_ulong submit() const {
        // TODO Throw error if DataTransfer is not submitted yet
        return _submit;
    }

    cl_ulong start() const {
        // TODO Throw error if DataTransfer is not started yet
        return _start;
    }

    cl_ulong end() const {
        // TODO Throw error if DataTransfer is not finished yet
        return _end;
    }

    bool isComplete() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return (_status == CL_SUCCESS || _status < 0);
    }

    void wait() const {
        std::lock_guard<std::mutex> lock(_mutex);
        while (_status != CL_SUCCESS && _status > 0) {
            _statusChanged.wait(_mutex);
        }
        if (_status < 0) {
            throw dcl::IOException("Data transfer failed");
        }
    }

    void abort() {
        std::lock_guard<std::mutex> lock(_mutex);
        /* TODO Implement DataTransferImpl::abort */
    }

    size_t size() const {
        return _size;
    }

    typename Operation::pointer_type ptr() const {
        return _ptr;
    }

    void onStart() {
        std::lock_guard<std::mutex> lock(_mutex);
        _start = dcl::util::clock.getTime(); // take time stamp
        _status = CL_RUNNING;
        // signal start
        _statusChanged.notify_all();
    }

    void onFinish(
            const boost::system::error_code& ec,
            size_t bytes_transferred) {
        std::unique_lock<std::mutex> lock(_mutex);
        _end = dcl::util::clock.getTime(); // take time stamp

        double latency = static_cast<double>(_start - _submit) / 1000000.0;
        double durance = static_cast<double>(_end   - _start) / 1000000000.0;

        // TODO Use more specific error codes
        _status = ec ? CL_IO_ERROR_WWU : CL_SUCCESS;
        // signal completion
        _statusChanged.notify_all();
        lock.unlock();

        triggerCallbacks();

        double bandwidth = (_size / static_cast<double>(1024 * 1024)) / durance;
        Operation::logFinish(_size, latency, bandwidth);
    }

private:
	const size_t _size;
	typename Operation::pointer_type _ptr;

	cl_ulong _submit;
	cl_ulong _start;
	cl_ulong _end;

	cl_int _status; //!< status of data transfer
	std::vector<std::function<void (cl_int)>> _callbacks;

    mutable std::mutex _mutex;
    mutable std::condition_variable_any _statusChanged;
};

/* ****************************************************************************/

typedef DataTransferImpl<Receive> DataReceipt;
typedef DataTransferImpl<Send> DataSending;

} /* namespace comm */

} /* namespace dclasio */

#endif /* DATATRANSFERIMPL_H_ */
