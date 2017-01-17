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
 * \file CreateContext.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef CREATECONTEXT_H_
#define CREATECONTEXT_H_

#include "Request.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <vector>

namespace dclasio {
namespace message {

/* TODO Decide on how to specify compute nodes
 * 1) compute nodes are dynamically created on each other when they should
 *    interact in a context. In this case URLs or other identifiers have to be
 *    transmitted.
 * 2) compute nodes should be created on each other as soon as they are created
 *    on the host. In this case compute nodes can be identified by their ID
 *    within dOpenCL.
 */
class CreateContext: public Request {
public:
    CreateContext();
    CreateContext(
            dcl::object_id                      contextId,
            const std::vector<dcl::process_id>& computeNodeIds);
    CreateContext(
            dcl::object_id                      contextId,
            const std::vector<dcl::process_id>& computeNodeIds,
            cl_device_type                      deviceType);
    CreateContext(
            dcl::object_id                      contextId,
            const std::vector<dcl::process_id>& computeNodeIds,
            const std::vector<dcl::object_id>&  deviceIds);
    CreateContext(
            const CreateContext& rhs);

    dcl::object_id contextId() const;
    const std::vector<dcl::process_id>& computeNodeIds() const;
    cl_device_type deviceType() const;
    const std::vector<dcl::object_id>& deviceIds() const;

    static const class_type TYPE = 100 + CREATE_CONTEXT;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _contextId << _computeNodeIds << _deviceType << _deviceIds;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _contextId >> _computeNodeIds >> _deviceType >> _deviceIds;
    }

private:
    dcl::object_id _contextId;
    std::vector<dcl::process_id> _computeNodeIds;
    //! Only select devices of the specified types(s) for the context
    cl_device_type _deviceType;
    //! Only select the specified devices for the context, or all devices if no devices are specified
    std::vector<dcl::object_id> _deviceIds;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* CREATECONTEXT_H_ */
