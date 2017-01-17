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
 * \file CreateEvent.h
 *
 * \date 2014-04-05
 * \author Philipp Kegel
 */

#ifndef CREATEEVENT_H_
#define CREATEEVENT_H_

#include "Request.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <vector>

namespace dclasio {

namespace message {

/*!
 * \brief A request message for creating an event.
 */
class CreateEvent: public Request {
public:
    CreateEvent();
	CreateEvent(
			dcl::object_id                     contextId,
			dcl::object_id                     eventId,
			const std::vector<dcl::object_id>& memObjectIds);
	CreateEvent(
	        const CreateEvent& rhs);
	virtual ~CreateEvent();

	dcl::object_id contextId() const;
	dcl::object_id eventId() const;
	const std::vector<dcl::object_id>& memObjectIds() const;

    static const class_type TYPE = 100 + CREATE_EVENT;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        Request::pack(buf);
        buf << _contextId << _eventId << _memObjectIds;
    }

    void unpack(dcl::ByteBuffer& buf) {
        Request::unpack(buf);
        buf >> _contextId >> _eventId >> _memObjectIds;
    }

private:
	dcl::object_id _contextId;
	dcl::object_id _eventId;
	std::vector<dcl::object_id> _memObjectIds;
};

} /* namespace message */
} /* namespace dclasio */

#endif /* CREATEEVENT_H_ */
