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
 * \file EventProfilingInfosResponse.h
 *
 * \date 2013-10-27
 * \author Philipp Kegel
 */

#ifndef EVENTPROFILINGINFOSRESPONSE_H_
#define EVENTPROFILINGINFOSRESPONSE_H_

#include "Request.h"
#include "Response.h"

#include <dcl/ByteBuffer.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclasio {
namespace message {

/*!
 * \brief A response message containing all event profiling information.
 */
class EventProfilingInfosReponse: public DefaultResponse {
public:
    EventProfilingInfosReponse();
    EventProfilingInfosReponse(
            const Request& request,
            cl_ulong received,
            cl_ulong queued,
            cl_ulong submit,
            cl_ulong start,
            cl_ulong end);
    EventProfilingInfosReponse(
            const EventProfilingInfosReponse& rhs);
    virtual ~EventProfilingInfosReponse();

    static const class_type TYPE = 200 + Request::GET_EVENT_PROFILING_INFOS;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        DefaultResponse::pack(buf);
        buf << received << queued << submit << start << end;
    }

    void unpack(dcl::ByteBuffer& buf) {
        DefaultResponse::unpack(buf);
        buf >> received >> queued >> submit >> start >> end;
    }

    cl_ulong received;
    cl_ulong queued;
    cl_ulong submit;
    cl_ulong start;
    cl_ulong end;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* EVENTPROFILINGINFOSRESPONSE_H_ */
