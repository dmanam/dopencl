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
 * \file ReadWriteCommand.cpp
 *
 * \date 2012-03-24
 * \author Philipp Kegel
 */

#include "ReadWriteCommand.h"

#include "../../CommandQueue.h"

#include "Command.h"

#include <dcl/ComputeNode.h>
#include <dcl/DataTransfer.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <functional>
#include <memory>

namespace dclicd {

namespace command {

ReadMemoryCommand::ReadMemoryCommand(cl_command_type type,
		cl_command_queue commandQueue, size_t cb, void *ptr) :
	Command(type, commandQueue), _cb(cb), _ptr(ptr) {
}

cl_int ReadMemoryCommand::submit() {
	// start data receipt
	std::shared_ptr<dcl::DataTransfer> receipt(
			_commandQueue->computeNode().receiveData(_cb, _ptr));
	// register callback to complete ReadMemoryCommand
	receipt->setCallback(std::bind(
	        &ReadMemoryCommand::onExecutionStatusChanged, this, std::placeholders::_1));

	return CL_RUNNING;
}

/* ****************************************************************************/

WriteMemoryCommand::WriteMemoryCommand(cl_command_type type,
		cl_command_queue commandQueue, size_t cb, const void *ptr) :
	Command(type, commandQueue), _cb(cb), _ptr(ptr) {
}

cl_int WriteMemoryCommand::submit() {
    // start data sending
	_commandQueue->computeNode().sendData(_cb, _ptr);
	
    // WriteMemoryCommand will be completed by compute node

	return CL_RUNNING;
}

} /* namespace command */

} /* namespace dclicd */
