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
 * \file EventSynchronizationMessage.h
 *
 * \date 2014-04-03
 * \author Philipp Kegel
 */

#ifndef EVENTSYNCHRONIZATIONMESSAGE_H_
#define EVENTSYNCHRONIZATIONMESSAGE_H_

#include "Message.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

namespace dclasio {

namespace message {

/**
 * @brief Request an update of memory objects, linked with this event.
 *
 * This message is sent by compute nodes to synchronize with an event, i.e. to
 * update to the changes associated with the specified event.
 */
class EventSynchronizationMessage: public Message {
public:
    EventSynchronizationMessage();
    EventSynchronizationMessage(
            const dcl::object_id commandId);
    EventSynchronizationMessage(
            const EventSynchronizationMessage& rhs);
    virtual ~EventSynchronizationMessage();

    dcl::object_id commandId() const;

    static const class_type TYPE = 8802;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        buf << _commandId;
    }

    void unpack(dcl::ByteBuffer& buf) {
        buf >> _commandId;
    }

private:
    dcl::object_id _commandId;
};

} /* namespace message */

} /* namespace dclasio */

#endif /* EVENTSYNCHRONIZATIONMESSAGE_H_ */
