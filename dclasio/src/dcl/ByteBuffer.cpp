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
 * \file ByteBuffer.cpp
 *
 * \date 2014-03-20
 * \author Philipp Kegel
 */

#include <dcl/Binary.h>
#include <dcl/ByteBuffer.h>

#include <algorithm>
#if USE_CSTRING
#include <cstring>
#endif
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace dcl {

ByteBuffer::ByteBuffer() :
    _pos(0), _len(0), _max_size(DEFAULT_MAX_SIZE), _size(DEFAULT_SIZE), _bytes(new value_type[_size]) { }
ByteBuffer::ByteBuffer(size_type initial_size) :
    _pos(0), _len(0), _max_size(DEFAULT_MAX_SIZE), _size(initial_size), _bytes(new value_type[_size]) { }
ByteBuffer::ByteBuffer(size_type size, value_type bytes[]) :
    _pos(0), _len(size), _max_size(DEFAULT_MAX_SIZE), _size(size), _bytes(bytes) { }
ByteBuffer::ByteBuffer(ByteBuffer&& other) :
    _pos(other._pos), _len(other._len), _max_size(DEFAULT_MAX_SIZE), _size(other._size), _bytes(std::move(other._bytes)) { }
ByteBuffer::~ByteBuffer() { }

void ByteBuffer::set_max_size(size_type max_size) {
    // max_size must not be reduced below current buffer size
    if (max_size < _size) throw std::out_of_range("Buffer limit must be greater than buffer size");
    _max_size = max_size;
}

ByteBuffer& ByteBuffer::operator<<(const bool flag) {
    ensure_free(1);
    _bytes[_len] = (flag ? 1 : 0);
    ++_len;
    return *this;
}

#if USE_CSTRING
ByteBuffer& ByteBuffer::operator<<(const char *str) {
    size_t size = strlen(str) + 1; // size of C string including terminating null character
    ensure_free(size);
    std::copy(str, str + size, end());
    _len += size;
    return *this;
}
#endif

ByteBuffer& ByteBuffer::operator<<(const std::string& str) {
    auto size = str.size();
    operator<<(size); // write number of characters
    ensure_free(size);
    std::copy(std::begin(str), std::end(str), end());
    _len += size;
    return *this;
}

ByteBuffer& ByteBuffer::operator<<(const Binary& data) {
    auto size = data.size();
    operator<<(size); // write number of bytes
    ensure_free(size);
    auto begin = static_cast<const value_type *>(data.value());
    std::copy(begin, begin + size, end());
    _len += size;
    return *this;
}

ByteBuffer& ByteBuffer::operator>>(bool& flag) {
    ensure_bytes(1);
    flag = (_bytes[_pos] != 0);
    ++_pos;
    return *this;
}

#if USE_CSTRING
ByteBuffer& ByteBuffer::operator>>(char *str) {
    size_t size = strlen(reinterpret_cast<char *>(_bytes.get() + _pos)) + 1;
    ensure_bytes(size); // fails if C string is not terminated (within this buffer)
    std::copy(cbegin(), cbegin() + size, str);
    _pos += size;
    return *this;
}
#endif

ByteBuffer& ByteBuffer::operator>>(std::string& str) {
    size_t size;
    operator>>(size); // read number of characters
    ensure_bytes(size);
    str.assign(cbegin(), size);
    _pos += size;
    return *this;
}

ByteBuffer& ByteBuffer::operator>>(Binary& data) {
    size_t size;
    operator>>(size); // read number of bytes
    ensure_bytes(size);
    data.assign(size, cbegin());
    _pos += size;
    return *this;
}

void ByteBuffer::resize(size_type size_) {
    reserve(size_); // no operation, if internal buffer size is greater or equal
    _pos = 0;
    _len = size_;
}

ByteBuffer::size_type ByteBuffer::size() const {
    return _len - _pos;
}

ByteBuffer::iterator ByteBuffer::begin() {
    return _bytes.get() + _pos;
}

ByteBuffer::const_iterator ByteBuffer::begin() const {
    return _bytes.get() + _pos;
}

ByteBuffer::const_iterator ByteBuffer::cbegin() const {
    return _bytes.get() + _pos;
}

ByteBuffer::iterator ByteBuffer::end() {
    return _bytes.get() + _len;
}

ByteBuffer::const_iterator ByteBuffer::end() const {
    return _bytes.get() + _len;
}

ByteBuffer::const_iterator ByteBuffer::cend() const {
    return _bytes.get() + _len;
}

} // namespace dcl
