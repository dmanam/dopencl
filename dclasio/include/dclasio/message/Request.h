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
 * \file Request.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef REQUEST_H_
#define REQUEST_H_

#include <dclasio/message/Message.h>

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

namespace dclasio {
namespace message {

class Request: public Message {
public:
    typedef uint32_t id_type;

	enum command : class_type {
		INVALID                     = 0,

	    GET_DEVICE_IDS              = 1,
	    GET_DEVICE_INFO             = 2,

	    CREATE_CONTEXT              = 11,
	    RELEASE_CONTEXT             = 12,

	    CREATE_BUFFER               = 21,
	    RELEASE_MEM_OBJECT          = 22,

	    CREATE_COMMAND_QUEUE        = 31,
	    RELEASE_COMMAND_QUEUE       = 32,

	    CREATE_PROGRAM_WITH_SOURCE  = 41,
	    CREATE_PROGRAM_WITH_BINARY  = 42,
	    RELEASE_PROGRAM             = 43,
	    BUILD_PROGRAM               = 44,
	    GET_PROGRAM_INFO            = 45,
        GET_PROGRAM_BUILD_LOG       = 46,

	    CREATE_KERNEL               = 51,
	    CREATE_KERNELS_IN_PROGRAM   = 52,
	    RELEASE_KERNEL              = 53,
	    SET_KERNEL_ARG              = 54,
	    SET_KERNEL_ARG_BINARY       = 55,
	    SET_KERNEL_ARG_MEM_OBJECT   = 56,
	    GET_KERNEL_INFO             = 57,
	    GET_KERNEL_WORK_GROUP_INFO  = 58,

	    CREATE_EVENT                = 61,
	    RELEASE_EVENT               = 62,
	    GET_EVENT_PROFILING_INFOS   = 63,

	    FLUSH                       = 71,
	    FINISH                      = 72,

	    ENQUEUE_READ_BUFFER         = 81,
	    ENQUEUE_WRITE_BUFFER        = 82,
	    ENQUEUE_COPY_BUFFER         = 83,
	    ENQUEUE_NDRANGE_KERNEL      = 84,
	    ENQUEUE_MARKER              = 85,
	    ENQUEUE_WAIT_FOR_EVENTS     = 86,
	    ENQUEUE_BARRIER             = 87,
	    ENQUEUE_MAP_BUFFER          = 88,
	    ENQUEUE_UNMAP_BUFFER        = 89,

	    ENQUEUE_BROADCAST_BUFFER    = 91,
	    ENQUEUE_REDUCE_BUFFER       = 92
	};

	Request();
	Request(
	        const Request& rhs);
	virtual ~Request();

    virtual class_type get_type() const = 0;

    void pack(dcl::ByteBuffer& buf) const {
        buf << id;
    }

    void unpack(dcl::ByteBuffer& buf) {
        buf >> id;
    }

	id_type id;

private:
	static id_type requestCount;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* REQUEST_H_ */
