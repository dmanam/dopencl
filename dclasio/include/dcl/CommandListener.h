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
 * \file CommandListener.h
 *
 * \date 2011-08-07
 * \author Philipp Kegel
 */

#ifndef DCL_COMMANDLISTENER_H_
#define DCL_COMMANDLISTENER_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace dcl {

/*!
 * \brief An listener API for command status changes
 *
 * This API is usually implemented by event objects in order to monitor the execution status of their associated command.
 * However, this API is also used to control command execution within the dOpenCL runtime system
 */
class CommandListener {
public:
    virtual ~CommandListener() { }

    /*!
     * \brief Callback for incoming command execution status change notifications.
     *
     * \param[in]  executionStatus  new command execution status
     */
    virtual void onExecutionStatusChanged(
            cl_int executionStatus) = 0;
};

} /* namespace dcl */

#endif /* DCL_COMMANDLISTENER_H_ */
