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

/**
 * @file ResponseBuffer.h
 *
 * @date 2011-04-18
 * @author Philipp Kegel
 */

#ifndef RESPONSEBUFFER_H_
#define RESPONSEBUFFER_H_

#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>

#include <dcl/DCLException.h>

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace dclasio {

namespace comm {

/**
 * @class A ring buffer for saving responses from compute nodes.
 */
class ResponseBuffer {
public:
	static const size_t DEFAULT_SIZE = 64;

	ResponseBuffer(
	        size_t size = DEFAULT_SIZE);
	virtual ~ResponseBuffer();

	void put(
	        std::unique_ptr<message::Response>&& response);
    /* TODO Add timed method for adding responses */

	std::unique_ptr<message::Response> tryGet(
	        const message::Request& request);
	/**
	 * @brief Waits for a response indefinitely.
	 *
     * @param[in]  request  the request which is associated with the response
	 * @return the response
	 */
	std::unique_ptr<message::Response> get(
	        const message::Request& request);

	/**
	 * @brief Waits for a response.
	 *
	 * @param[in]  request  the request which is associated with the response
	 * @param[in]  timeout	a timeout
	 * @return the response, or NULL if @c timeout has been reached
	 */
	template<class Rep, class Period>
	std::unique_ptr<message::Response> get(
	        const message::Request& request,
	        const std::chrono::duration<Rep, Period>& timeout) {
	    std::lock_guard<std::mutex> lock(_mutex);
	    std::unique_ptr<message::Response> response;

	    while (!_interrupt && !(response = remove(request))) {
	        if (_responseAdded.wait_for(_mutex, timeout) == std::cv_status::timeout) {
	            /* timeout expired */
	            break;
	        }
	    };
	    if (_interrupt) throw dcl::ThreadInterrupted();

	    return response;
	}

	void interrupt();
	void clear();

private:
	bool insert(
	        std::unique_ptr<message::Response>&& response);
	std::unique_ptr<message::Response> remove(
	        const message::Request& request);

	std::vector<std::unique_ptr<message::Response>> _responses;
	std::vector<std::unique_ptr<message::Response>>::iterator _head, _tail;

	std::mutex _mutex;                            /**< buffer mutex */
	std::condition_variable_any _responseAdded;   /**< condition: added response to buffer */
	std::condition_variable_any _responseRemoved; /**< condition: removed response from buffer */
	bool _interrupt;
};

} /* namespace comm */

} /* namespace dclasio */

#endif /* RESPONSEBUFFER_H_ */
