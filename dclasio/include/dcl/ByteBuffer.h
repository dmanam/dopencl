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
 * \file ByteBuffer.h
 *
 * \date 2014-03-20
 * \author Philipp Kegel
 */

#ifndef DCL_BYTEBUFFER_H_
#define DCL_BYTEBUFFER_H_

#include <dcl/Binary.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace {

/*!
 * \brief Serialization type traits
 */
template<typename T> struct serialization;
template<> struct serialization<cl_char>   { static const size_t size = sizeof(cl_char);   }; // 8 bit
template<> struct serialization<cl_uchar>  { static const size_t size = sizeof(cl_uchar);  }; // 8 bit
template<> struct serialization<cl_short>  { static const size_t size = sizeof(cl_short);  }; // 16 bit
template<> struct serialization<cl_ushort> { static const size_t size = sizeof(cl_ushort); }; // 16 bit
template<> struct serialization<cl_int>    { static const size_t size = sizeof(cl_int);    }; // 32 bit
template<> struct serialization<cl_uint>   { static const size_t size = sizeof(cl_uint);   }; // 32 bit
template<> struct serialization<cl_long>   { static const size_t size = sizeof(cl_long);   }; // 64 bit
template<> struct serialization<cl_ulong>  { static const size_t size = sizeof(cl_ulong);  }; // 64 bit
template<> struct serialization<float>     { static const size_t size = sizeof(float);     }; // 32 bit
template<> struct serialization<double>    { static const size_t size = sizeof(double);    }; // 64 bit

} // anonymous namespace

/* ****************************************************************************/

namespace dcl {

/*!
 * \brief A simple de-/serialization facility
 * This class is able to serialize the following types:
 *  + OpenCL API types (cl_int, cl_uint, cl_ulong, ...),
 *  + float, double,
 *  + size_t,
 *  + C strings, std::string, and
 *  + std::vector<T>, where T is any serializable type.
 * Network byte order is used for serialized representation to ensure portability.
 * Deserialization is *not* type-safe, i.e., it is the caller's responsibility
 * to extract serialized data correctly.
 * This class is not thread-safe for performance reasons.
 */
class ByteBuffer {
public:
    typedef char value_type;
    typedef uint32_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    typedef value_type * iterator;
    typedef const value_type * const_iterator;

    const static size_type DEFAULT_SIZE = 512; //!< default buffer size in bytes
    const static size_type DEFAULT_MAX_SIZE = 65536; //!< default maximum buffer size in bytes

private:
    /*!
     * \brief Resizes the buffer to the specified internal size.
     * \param[in]  size the new internal buffer size
     * \throw std::out_of_range if \c size exceeds max_size
     */
    inline void reserve(
            size_type size) {
        if (size > _max_size) throw std::out_of_range("Internal buffer overflow");
        if (size <= _size) return; // no operation
        /* TODO Resize buffer
         * First, try to recover memory of read bytes just by moving content to beginning
         * If this does not provide enough space, increase buffer size before moving content bytes to beginning */
        assert(!"resize not implemented");
    }

    /*!
     * \brief Ensures that at least \c size bytes can be written to the buffer
     * \param[in]  free the number of bytes to write
     */
    inline void ensure_free(
            size_type free) {
        auto size = _len + free;
        if (size > _size) { // ensure required buffer size
            while (size < _size) {
                size *= 2; // double buffer size
            }
            reserve(std::max(std::min(size, _max_size), free));
        }
    }

    /*!
     * \brief Ensures that at least \c size bytes can be read from the buffer
     * \param[in]  size the number of bytes to read
     * \throw std::out_of_range if less than \c size bytes can be read from the buffer
     */
    inline void ensure_bytes(
            size_type size) {
        if ((_len - _pos) < size) throw std::out_of_range("Buffer underflow");
    }

    ByteBuffer(const ByteBuffer& other) = delete;

public:
    ByteBuffer();
    /*!
     * \brief Creates a buffer with the specified number of reserved bytes
     * The buffer size as returned by ByteBuffer::size is 0.
     * \param[in]  initial_size the internal size of the byte buffer
     */
    ByteBuffer(
            size_type initial_size);
    /*!
     * \brief Creates a buffer from raw bytes
     * The buffer becomes owner of the bytes - it does *not* copy the bytes.
     * \param[in]  size     the number of bytes
     * \param[in]  bytes    the raw bytes
     */
    ByteBuffer(
            size_type size,
            value_type bytes[]);
    ByteBuffer(
            ByteBuffer&& other);
    virtual ~ByteBuffer();

    /*!
     * \brief Restricts the buffer's maximum size to the specified value
     * \param[in]  max_size the buffer's maximum size
     */
    void set_max_size(
            size_type max_size);

    template<typename T>
    ByteBuffer& operator<<(
            const T& value) {
        ensure_free(serialization<T>::size);
        // TODO Convert to network byte order
        // TODO Use std::copy
        memcpy(_bytes.get() + _len, &value, serialization<T>::size);
        _len += serialization<T>::size;
        return *this;
    }

    ByteBuffer& operator<<(
            const bool flag);
#if USE_CSTRING
    ByteBuffer& operator<<(
            const char *str);
#endif
    ByteBuffer& operator<<(
            const std::string& str);
    ByteBuffer& operator<<(
            const Binary& data);

    /*!
     * \brief Serializes a vector of serializable values
     * \param[in]  values   the vector to serialize
     * \return this byte buffer
     */
    template<typename T>
    ByteBuffer& operator<<(
            const std::vector<T>& values) {
        operator<<(values.size()); // write number of elements
        for (const auto& value : values) {
            operator<<(value);
        }
        return *this;
    }

    /*!
     * \brief Serializes a map of serializable values
     * \param[in]  pairs    the map of key-value-pairs to serialize
     * \return this byte buffer
     */
    template<typename Key, typename Value>
    ByteBuffer& operator<<(
            const std::map<Key, Value>& pairs) {
        operator<<(pairs.size()); // write number of pairs
        for (const auto& pair : pairs) {
            operator<<(pair.first);
            operator<<(pair.second);
        }
        return *this;
    }

    template<typename T>
    ByteBuffer& operator>>(
            T& value) {
        ensure_bytes(serialization<T>::size);
        // TODO Convert to host byte order
        // TODO Use std::copy
        memcpy(&value, _bytes.get() + _pos, serialization<T>::size);
        _pos += serialization<T>::size;
        return *this;
    }

    ByteBuffer& operator>>(
            bool& flag);
#if USE_CSTRING
    ByteBuffer& operator>>(
            char *str);
#endif
    ByteBuffer& operator>>(
            std::string& str);
    ByteBuffer& operator>>(
            Binary& data);

    template<typename T>
    ByteBuffer& operator>>(
            std::vector<T>& values) {
        size_t size;
        operator>>(size); // read number of elements
        values.resize(size);
        for (auto& value : values) {
            // remove const qualifier from value to update it from byte buffer
            operator>>(const_cast<typename std::remove_const<decltype(value)>::type>(value));
        }
        return *this;
    }

    template<typename Key, typename Value>
    ByteBuffer& operator>>(
            std::map<Key, Value>& pairs) {
        size_t size;
        operator>>(size); // read number of pairs
        pairs.clear();
        for (size_t i = 0; i < size; ++i) {
            // remove const qualifiers from key and value to update them from byte buffer
            typename std::remove_const<Key>::type key;
            typename std::remove_const<Value>::type value;
            operator>>(key);
            operator>>(value);
            pairs.insert(std::make_pair(std::move(key), std::move(value)));
            // TODO std::map::emplace does not compile with GCC 4.6
//            pairs.emplace(std::move(key), std::move(value));
        }
        return *this;
    }

    /*!
     * \brief Resizes the buffer to the specified size
     * The buffer's content is undefined after this operation.
     * Usually, this method is used before overwriting the buffer directly using an iterator.
     * \param[in]  size the new buffer size
     */
    void resize(
            size_type size);

    size_type size() const;

    iterator begin();
    const_iterator begin() const;
    const_iterator cbegin() const;

    iterator end();
    const_iterator end() const;
    const_iterator cend() const;

private:
    size_type _pos; // read count
    size_type _len; // write count, i.e., size of buffer content *including* the read bytes
    size_type _max_size;
    size_type _size; // buffer size
    std::unique_ptr<value_type[]> _bytes; // buffer data
};

} // namespace dcl

#endif /* DCL_BYTEBUFFER_H_ */
