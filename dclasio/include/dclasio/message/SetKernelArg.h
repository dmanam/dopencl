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
 * \file SetKernelArg.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef SETKERNELARG_H_
#define SETKERNELARG_H_

#include "Request.h"

#include <dcl/Binary.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>

namespace dclasio {

namespace message {

/*!
 * \brief A basic request message for setting a kernel argument.
 *
 * A base class to set kernel arguments on the compute node.
 * Use SetKernelArgMemObject or SetKernelArgBinary to send arguments.
 */
class SetKernelArg: public Request {
public:
    SetKernelArg();
    SetKernelArg(dcl::object_id kernelId, cl_uint index);
    SetKernelArg(const SetKernelArg& rhs);
    virtual ~SetKernelArg();

    dcl::object_id kernelId() const;
    cl_uint argIndex() const;
//	virtual size_t argSize() const = 0;
//	virtual const void * argSize() const = 0;

    static const class_type TYPE = 100 + SET_KERNEL_ARG;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _kernelId << _index;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _kernelId >> _index;
    }

private:
    dcl::object_id _kernelId;
    cl_uint _index;
};

/* ****************************************************************************/

/*!
 * \brief A request message for setting an arbitrary kernel argument.
 *
 * Use SetKernelArgMemObject, if the argument is a memory object.
 */
class SetKernelArgBinary: public SetKernelArg {
public:
    SetKernelArgBinary();
    SetKernelArgBinary(
            dcl::object_id kernelId,
            cl_uint index,
            size_t size,
            const void * value);
    SetKernelArgBinary(const SetKernelArgBinary& rhs);
    virtual ~SetKernelArgBinary();

    size_t argSize() const;
    const void * argValue() const;

    static const class_type TYPE = 100 + SET_KERNEL_ARG_BINARY;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        SetKernelArg::pack(buf);
        buf << _arg;
    }

    void unpack(dcl::ByteBuffer& buf) {
        SetKernelArg::unpack(buf);
        buf >> _arg;
    }

private:
    dcl::Binary _arg;
};

/* ****************************************************************************/

/*!
 * \brief A request message for setting a memory object as kernel argument.
 */
class SetKernelArgMemObject: public SetKernelArg {
public:
    SetKernelArgMemObject();
    SetKernelArgMemObject(dcl::object_id kernelId, cl_uint index, size_t size);
    SetKernelArgMemObject(
            dcl::object_id kernelId,
            cl_uint index,
            dcl::object_id memObjectId);
    SetKernelArgMemObject(const SetKernelArgMemObject& rhs);
    virtual ~SetKernelArgMemObject();

    size_t argSize() const;
    const void * argValue() const;
    dcl::object_id memObjectId() const;

    static const class_type TYPE = 100 + SET_KERNEL_ARG_MEM_OBJECT;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        SetKernelArg::pack(buf);
        buf << _size << _memObjectId;
    }

    void unpack(dcl::ByteBuffer& buf) {
        SetKernelArg::unpack(buf);
        buf >> _size >> _memObjectId;
    }

private:
    size_t _size;
    dcl::object_id _memObjectId; //!< memory object ID, or 0 for nullptr
};

} /* namespace message */
} /* namespace dclasio */

#endif /* SETKERNELARG_H_ */
