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
 * \file ReadWriteCommand.h
 *
 * \date 2012-03-24
 * \author Philipp Kegel
 */

#ifndef READWRITECOMMAND_H_
#define READWRITECOMMAND_H_

#include "Command.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>

namespace dclicd {

namespace command {

class ReadMemoryCommand: public Command {
public:
    ReadMemoryCommand(
            cl_command_type     type,
            cl_command_queue    commandQueue,
            size_t              cb,
            void *              ptr);

private:
    cl_int submit();

    size_t _cb;
    void *_ptr;
};

/* ****************************************************************************/

class WriteMemoryCommand: public Command {
public:
    WriteMemoryCommand(
            cl_command_type     type,
            cl_command_queue    commandQueue,
            size_t              cb,
            const void *        ptr);

private:
    cl_int submit();

    size_t _cb;
    const void *_ptr;
};

} /* namespace command */

} /* namespace dclicd */

#endif /* READWRITECOMMAND_H_ */
