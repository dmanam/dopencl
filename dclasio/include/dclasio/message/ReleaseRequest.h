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
 * \file ReleaseRequest.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef RELEASEREQUEST_H_
#define RELEASEREQUEST_H_

#if 0
#include "Request.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

namespace dclasio {
namespace message {

/*!
 * \brief A generalization of the Delete... request messages
 *
 * !!! This template class is currently not used !!!
 */
template<Message::value_type TypeId>
class ReleaseRequest: public Request {
public:
    ReleaseRequest(
            dcl::object_id objectId);
    ReleaseRequest(
            const ReleaseRequest& rhs);
    virtual ~ReleaseRequest();

    dcl::object_id objectId() const {
        return _objectId;
    }

    static const value_type TYPE = TypeId;

    value_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _objectId;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _objectId;
    }

private:
    dcl::object_id _objectId;
};

typedef ReleaseRequest<100 + Request::RELEASE_COMMAND_QUEUE> ReleaseCommandQueueRequest;
typedef ReleaseRequest<100 + Request::RELEASE_CONTEXT> ReleaseContextRequest;
typedef ReleaseRequest<100 + Request::RELEASE_EVENT> ReleaseEventRequest;
typedef ReleaseRequest<100 + Request::RELEASE_KERNEL> ReleaseKernelRequest;
typedef ReleaseRequest<100 + Request::RELEASE_MEM_OBJECT> ReleaseMemObjectRequest;
typedef ReleaseRequest<100 + Request::RELEASE_PROGRAM> ReleaseProgramRequest;

} /* namespace message */
} /* namespace dclasio */
#endif

#endif /* RELEASEREQUEST_H_ */
