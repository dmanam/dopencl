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
 * @file Retainable.h
 *
 * @date 2013-10-26
 * @author Philipp Kegel
 */

#ifndef RETAINABLE_H_
#define RETAINABLE_H_

#include <cassert>

namespace dclicd {

namespace detail {

/**
 * @brief A class to performs implicit reference counting to alleviate handling of shared objects
 *
 * This class should probably be based on boost::intrusive_ptr.
 *
 * !!! This class is currently not used !!!
 */
template<typename T>
class Retainable {
    Retainable() : _object(nullptr) {
    }

    Retainable(T *object) : _object(object) {
        /* FIXME Never(!) call a virtual function within a constructor */
//		validate();
        retain();
    }

    Retainable(const Retainable& rhs) : _object(rhs._object) {
        retain();
    }

    virtual ~Retainable() {
        if (_object == nullptr) return;
        release();
    }

    Retainable& operator=(const Retainable& rhs) {
        if (_object != rhs._object) {
            release();
            _object = rhs._object;
            retain();
        }

        return *this;
    }

    operator T *() {
        retain();
        return _object;
    }

    void retain() {
        if (_object == nullptr) return;
        ++_object->ref_count;
    }

    void release() {
        if (_object == nullptr) return;
        assert(_object->ref_count > 0);
        if (--_object->ref_count == 0) {
            destroy();
            delete _object;
        }
    }

protected:
    virtual void destroy() = 0;

    T *_object;
};

} /* namespace detail */

} /* namespace dclicd */

#endif /* RETAINABLE_H_ */
