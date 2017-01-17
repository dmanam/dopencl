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
 * \file MappingCommand.cpp
 *
 * \date 2012-03-24
 * \author Philipp Kegel
 */

#include "MappingCommand.h"

#include "../../CommandQueue.h"
#include "../../Memory.h"

#include "../Buffer.h"
#include "../Error.h"
#include "../utility.h"

#include "../detail/MappedMemory.h"

#include "Command.h"

#include <dcl/ComputeNode.h>
#include <dcl/DataTransfer.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>

namespace dclicd {

namespace command {

MapBufferCommand::MapBufferCommand(
        cl_command_queue commandQueue,
        Buffer *buffer,
        cl_map_flags flags,
        size_t cb,
        void *ptr) :
    Command(CL_COMMAND_MAP_BUFFER, commandQueue), _buffer(buffer),
    _flags(flags), _cb(cb), _ptr(ptr) {
    assert(_buffer != nullptr); // buffer must not be NULL
    _buffer->retain();
}

MapBufferCommand::~MapBufferCommand() {
    release(_buffer);
}

cl_int MapBufferCommand::submit() {
    if ((_flags & CL_MAP_READ)) {
        /*
         * The mapped buffer region has to be synchronized, i.e., it has to be
         * downloaded to the mapped pointer.
         */
        // start data transfer
        std::shared_ptr<dcl::DataTransfer> receipt(
                _commandQueue->computeNode().receiveData(_cb, _ptr));
        // register callback to complete MapBufferCommand
        receipt->setCallback(std::bind(
                &MapBufferCommand::onExecutionStatusChanged, this, std::placeholders::_1));
    }

    return CL_RUNNING;
}

/* ****************************************************************************/

UnmapBufferCommand::UnmapBufferCommand(
        cl_command_queue    commandQueue,
        Buffer *            memobj,
        cl_map_flags        flags,
        size_t              cb,
        void *              ptr) :
    Command(CL_COMMAND_UNMAP_MEM_OBJECT, commandQueue), _memobj(memobj),
    _flags(flags), _cb(cb), _ptr(ptr) {
    assert(_memobj != nullptr); // buffer must not be NULL
    _memobj->retain();
}

UnmapBufferCommand::~UnmapBufferCommand() {
    release(_memobj);
}

cl_int UnmapBufferCommand::submit() {
    if ((_flags & CL_MAP_WRITE)) {
        /*
         * The mapped buffer region has to be synchronized, i.e., its data has
         * to be uploaded to the command queue's compute node.
         */
        // start data transfer
        std::shared_ptr<dcl::DataTransfer> sending(
                _commandQueue->computeNode().sendData(_cb, _ptr));

        // UnmapBufferCommand will be completed by compute node
    }

    return CL_RUNNING;
}

cl_int UnmapBufferCommand::complete(
        cl_int errcode) {
    if (errcode == CL_SUCCESS) {
        try {
            _memobj->unmap(_ptr); // release pointer to mapped memory
        } catch (const dclicd::Error& err) {
            return err.err();
        }
    }

    return errcode;
}

} /* namespace command */

} /* namespace dclicd */
