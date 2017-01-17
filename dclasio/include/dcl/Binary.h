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
 * \file Binary.h
 *
 * \date 2011-10-15
 * \author Philipp Kegel
 */

#ifndef BINARY_H_
#define BINARY_H_

#include <cassert>
#include <cstddef>
#include <cstring>
#include <utility>

namespace dcl {

/*!
 * \brief A container for a sequence of bytes.
 *
 * This class is used to store, e.g., object info or kernel arguments.
 * It is similar to std::string, but stores bytes rather than characters.
 */
class Binary {
public:
    /*!
     * \brief Creates an empty binary object.
     */
	Binary() :
		_size(0), _value(nullptr) {
	}

	// FIXME sizeof is not portable
	template<typename T>
	Binary(const T& value) :
	    _size(sizeof(T)),
	    _value(new char[sizeof(T)]) /* allocate memory for value */ {
        ::memcpy(_value, &value, _size); // copy value
	}

	Binary(size_t size, const void *value) :
		_size(size),
		_value(new char[size]) /* allocate memory for value */ {
		::memcpy(_value, value, _size); // copy value
	}

	Binary(const Binary& rhs) :
		_size(rhs._size),
		_value(new char[rhs._size]) /* allocate memory for value */ {
		::memcpy(_value, rhs._value, rhs._size); // copy value
	}

    Binary(Binary&& rhs) :
        _size(rhs._size), _value(rhs._value) {
        /* detach data from rvalue */
        rhs._size = 0;
        rhs._value = nullptr;
    }

	~Binary() {
		delete[] _value;
	}

	/*!
	 * \brief Performs a byte-wise comparison.
	 *
	 * \param[in]  rhs  the binary to compare this binary with
	 * \return \c true, if this binary equals rhs, otherwise \c false
	 */
	bool operator==(const Binary& rhs) const {
		return ((_size == rhs._size) &&
				(::memcmp(_value, rhs._value, _size) == 0));
	}

	bool operator!=(const Binary& rhs) const {
		return !(*this == rhs);
	}

	void assign(size_t size, const void *value) {
	    /* FIXME Handle self assignment in dcl::Binary::assign */

		if (_size != size) {
			char *mem = new char[size]; // allocate memory for new value
			delete[] _value; // delete current value

			_size = size;
			_value = mem;
		}

		/* initialize or overwrite current value */
		::memcpy(_value, value, size);
	}

	Binary& operator=(const std::pair<size_t, const void *>& param) {
		assign(param.first, param.second);
		return *this;
	}

	Binary& operator=(const Binary& rhs) {
		if (this != &rhs) {
			assign(rhs._size, rhs._value);
		}
		return *this;
	}

	/*!
	 * \brief Move assignment operator.
	 *
	 * This operator swaps the values of two binaries.
	 *
	 * \param[inout] rhs    value to move
	 * \return a reference to this binary
	 */
    Binary& operator=(Binary&& rhs) {
        assert(this != &rhs); // a temporary should always be unique
        /* swap value to ensure that rhs deletes current value */
        std::swap(_size, rhs._size);
        std::swap(_value, rhs._value);
        return *this;
    }

	size_t size() const {
		return _size;
	}

	const void * value() const {
		return _value;
	}

private:
	size_t _size;
	char *_value;
};

} /* namespace dcl */

#endif /* BINARY_H_ */
