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
 * \!file SetKernelArg.cpp
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#include <dclasio/message/SetKernelArg.h>
#include <dclasio/message/Request.h>

#include <dcl/Binary.h>
#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cassert>
#include <cstddef>

namespace dclasio {
namespace message {

/* ****************************************************************************
 * Set kernel argument
 ******************************************************************************/

SetKernelArg::SetKernelArg() :
        _kernelId(0) {
}

SetKernelArg::SetKernelArg(dcl::object_id kernelId, cl_uint index) :
        _kernelId(kernelId), _index(index) {
}

SetKernelArg::SetKernelArg(const SetKernelArg& rhs) :
        Request(rhs), _kernelId(rhs._kernelId), _index(rhs._index) {
}

SetKernelArg::~SetKernelArg() {
}

dcl::object_id SetKernelArg::kernelId() const {
    return _kernelId;
}

cl_uint SetKernelArg::argIndex() const {
    return _index;
}

/* ****************************************************************************
 * Set binary kernel argument
 ******************************************************************************/

SetKernelArgBinary::SetKernelArgBinary() {
}

SetKernelArgBinary::SetKernelArgBinary(
        dcl::object_id kernelId,
        cl_uint index,
        size_t size,
        const void *value) :
        SetKernelArg(kernelId, index), _arg(size, value) {
}

SetKernelArgBinary::SetKernelArgBinary(const SetKernelArgBinary& rhs) :
        SetKernelArg(rhs), _arg(rhs._arg) {
}

SetKernelArgBinary::~SetKernelArgBinary() {
}

size_t SetKernelArgBinary::argSize() const {
    return _arg.size();
}

const void * SetKernelArgBinary::argValue() const {
    return _arg.value();
}

/* ****************************************************************************
 * Set memory object as kernel argument
 ******************************************************************************/

SetKernelArgMemObject::SetKernelArgMemObject() :
        _memObjectId(0) {
}

SetKernelArgMemObject::SetKernelArgMemObject(
        dcl::object_id kernelId,
        cl_uint index,
        size_t size) :
        SetKernelArg(kernelId, index), _size(size), _memObjectId(0) {
}

SetKernelArgMemObject::SetKernelArgMemObject(
        dcl::object_id kernelId,
        cl_uint index,
        dcl::object_id memObjectId) :
        SetKernelArg(kernelId, index), _size(sizeof(cl_mem)), _memObjectId(
                memObjectId) {
}

SetKernelArgMemObject::SetKernelArgMemObject(const SetKernelArgMemObject& rhs) :
        SetKernelArg(rhs), _size(rhs._size), _memObjectId(rhs._memObjectId) {
}

SetKernelArgMemObject::~SetKernelArgMemObject() {
}

size_t SetKernelArgMemObject::argSize() const {
    return _size;
}

const void * SetKernelArgMemObject::argValue() const {
    return (_memObjectId == 0) ? nullptr : &_memObjectId;
}

dcl::object_id SetKernelArgMemObject::memObjectId() const {
    return _memObjectId;
}

} /* namespace message */
} /* namespace dclasio */
