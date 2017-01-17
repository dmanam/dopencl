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
 * \file ConnectionListener.h
 *
 * \date 2014-04-06
 * \author Philipp Kegel
 */

#ifndef CONNECTIONLISTENER_H_
#define CONNECTIONLISTENER_H_

#include "../ProcessImpl.h"

#include <dcl/DCLTypes.h>

namespace dclasio {
namespace comm {

class DataStream;
class message_queue;

/* ****************************************************************************/

/*!
 * Connection listener API
 */
class connection_listener {
public:
    virtual ~connection_listener() { }

    virtual bool approve_message_queue(
            ProcessImpl::Type process_type,
            dcl::process_id process_id) = 0;

    virtual void message_queue_connected(
            message_queue& msgq,
            ProcessImpl::Type process_type,
            dcl::process_id process_id) = 0;

    virtual void message_queue_disconnected(
            message_queue& msgq) = 0;

    virtual bool approve_data_stream(
            dcl::process_id process_id) = 0;

    virtual void data_stream_connected(
            DataStream& dataStream,
            dcl::process_id process_id) = 0;
};

} /* namespace comm */
} /* namespace dclasio */

#endif /* CONNECTIONLISTENER_H_ */
