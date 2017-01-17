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
 * \file BlockingQueue.h
 *
 * \date 2011-03-10
 * \author Philipp Kegel
 */

#ifndef BLOCKINGQUEUE_H_
#define BLOCKINGQUEUE_H_

#include "DCLException.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>

namespace dcl {

/*!
 * \brief A thread-safe blocking queue
 */
template<class T, class Container = typename std::queue<T>::container_type >
class BlockingQueue: private std::queue<T, Container> {
public:
    typedef typename Container::value_type value_type;
    typedef typename Container::size_type  size_type;

    bool empty() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return std::queue<T, Container>::empty();
    }

    size_type size() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return std::queue<T, Container>::size();
    }

    value_type& front() {
        std::lock_guard<std::mutex> lock(_mutex);
        awaitElement();
        return std::queue<T, Container>::front();
    }

    const value_type& front() const {
        std::lock_guard<std::mutex> lock(_mutex);
        awaitElement();
        return std::queue<T, Container>::front();
    }

    value_type& back() {
        std::lock_guard<std::mutex> lock(_mutex);
        awaitElement();
        return std::queue<T, Container>::back();
    }

    const value_type& back() const {
        std::lock_guard<std::mutex> lock(_mutex);
        awaitElement();
        return std::queue<T, Container>::back();
    }

    void push(const T& x) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::queue<T, Container>::push(x);
        /* all waiting threads have to be notified as multiple calls of, e.g.,
         * front can be valid even with only a single element. */
        _modified.notify_all();
    }

    void pop() {
        std::lock_guard<std::mutex> lock(_mutex);

        awaitElement();
        std::queue<T, Container>::pop();
    }

    void interrupt() {
        std::lock_guard<std::mutex> lock(_mutex);
        _interrupt = true;
        _modified.notify_all();
    }

private:
    mutable std::mutex _mutex;
    std::condition_variable_any _modified;

    bool _interrupt;

    /*!
     * \brief Waits for the queue to become non-empty
     *
     * This methods blocks until an element has been added to the queue, or
     * interrupt is called.
     */
    void awaitElement() {
        _interrupt = false;

        while (std::queue<T, Container>::empty() && !_interrupt) {
            _modified.wait(_mutex);
        }

        if (_interrupt) throw ThreadInterrupted();
    }

};

} /* namespace dcl */

#endif /* BLOCKINGQUEUE_H_ */
