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
 * \file CommandMessage.h
 *
 * Command related messages
 *
 * \date 2011-07-11
 * \author Philipp Kegel
 */

#ifndef COMMAND_MESSAGE_H_
#define COMMAND_MESSAGE_H_

#include "Message.h"

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclasio {

namespace message {

/*!
 * \brief Notification of command execution status changes.
 *
 * This message is sent from compute nodes to the host.
 */
class CommandExecutionStatusChangedMessage: public Message {
public:
    CommandExecutionStatusChangedMessage();
	CommandExecutionStatusChangedMessage(
			dcl::object_id  commandId,
			cl_int          status);
	CommandExecutionStatusChangedMessage(
	        const CommandExecutionStatusChangedMessage& rhs);
	virtual ~CommandExecutionStatusChangedMessage();

	dcl::object_id commandId() const;
	cl_int status() const;

    static const class_type TYPE = 601;

    Message::class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        buf << _commandId << _status;
    }

    void unpack(dcl::ByteBuffer& buf) {
        buf >> _commandId >> _status;
    }


private:
	dcl::object_id _commandId;
	cl_int _status;
};

} /* namespace message */

} /* namespace dclasio */

#endif /* COMMAND_MESSAGE_H_ */
