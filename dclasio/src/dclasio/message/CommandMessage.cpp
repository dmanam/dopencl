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
 * \file CommandMessage.cpp
 *
 * Command related messages
 *
 * \date 2011-07-11
 * \author Philipp Kegel
 */

#include <dclasio/message/CommandMessage.h>

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dclasio {

namespace message {

CommandExecutionStatusChangedMessage::CommandExecutionStatusChangedMessage() :
    _commandId(0), _status(CL_SUBMITTED) {
}

CommandExecutionStatusChangedMessage::CommandExecutionStatusChangedMessage(
		dcl::object_id commandId, cl_int status):
	_commandId(commandId), _status(status) {
}

CommandExecutionStatusChangedMessage::CommandExecutionStatusChangedMessage(
		const CommandExecutionStatusChangedMessage& rhs) :
	_commandId(rhs._commandId), _status(rhs._status) {
}

CommandExecutionStatusChangedMessage::~CommandExecutionStatusChangedMessage() { }

dcl::object_id CommandExecutionStatusChangedMessage::commandId() const {
	return _commandId;
}

cl_int CommandExecutionStatusChangedMessage::status() const {
	return _status;
}

} /* namespace message */

} /* namespace dclasio */
