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
 * @file ResponseBuffer.cpp
 *
 * @date 2011-04-18
 * @author Philipp Kegel
 */

#include "ResponseBuffer.h"

#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>

#include <dcl/DCLException.h>

#include <condition_variable>
#include <cstddef>
#include <iterator>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace dclasio {

namespace comm {

ResponseBuffer::ResponseBuffer(size_t size) :
	_interrupt(false) {
    _responses.resize(size);
    _tail = _head = std::begin(_responses);
}

ResponseBuffer::~ResponseBuffer() {
}

void ResponseBuffer::put(std::unique_ptr<message::Response>&& response) {
	std::lock_guard<std::mutex> lock(_mutex);

	while (!_interrupt && !insert(std::move(response))) {
		_responseRemoved.wait(_mutex);
	}
	if (_interrupt) throw dcl::ThreadInterrupted();
}

std::unique_ptr<message::Response> ResponseBuffer::tryGet(const message::Request& request) {
	std::lock_guard<std::mutex> lock(_mutex);
	return remove(request);
}

std::unique_ptr<message::Response> ResponseBuffer::get(const message::Request& request) {
	std::lock_guard<std::mutex> lock(_mutex);
	std::unique_ptr<message::Response> response;

	while (!_interrupt && !(response = remove(request))) {
		_responseAdded.wait(_mutex);
	};
	if (_interrupt) throw dcl::ThreadInterrupted();

	return response;
}

void ResponseBuffer::interrupt() {
	std::lock_guard<std::mutex> lock(_mutex);
	_interrupt = true;
	_responseAdded.notify_all();
	_responseRemoved.notify_all();
}

void ResponseBuffer::clear() {
	std::lock_guard<std::mutex> lock(_mutex);

	_responses.clear();
	_tail = _head = std::begin(_responses);

    _responseRemoved.notify_all();
}

bool ResponseBuffer::insert(std::unique_ptr<message::Response>&& response) {
    std::vector<std::unique_ptr<message::Response>>::iterator i = _tail;

    do {
    	std::vector<std::unique_ptr<message::Response>>::iterator entry = i;

        /* next buffer position */
    	if (++i == std::end(_responses)) {
    		i = std::begin(_responses);
    	}

        if (!*entry) {
            *entry = std::move(response); /* move response into buffer */
            _tail = i;                    /* remember next buffer position for subsequent insert operation */
            _responseAdded.notify_all();
            break;
        }
    } while (i != _tail); /* only run through buffer once */

    return (response == nullptr);
}

std::unique_ptr<message::Response> ResponseBuffer::remove(const message::Request& request) {
    std::unique_ptr<message::Response> response;
    std::vector<std::unique_ptr<message::Response>>::iterator i = _head;

    do {
        std::vector<std::unique_ptr<message::Response>>::iterator entry = i;

        /* next buffer position */
        if (++i == std::end(_responses)) {
            i = std::begin(_responses);
        }
        if (*entry && (*entry)->get_request_id() == request.id) {
            response = std::move(*entry); /* remove response from buffer
                                             (must be removed *before* notification) */
            _head = i;                    /* remember next buffer position for subsequent search */
            _responseRemoved.notify_one();
            break;
        }

    } while (i != _head); /* only run through buffer once */

    return response;
}

} /* namespace comm */

} /* namespace dclasio */
