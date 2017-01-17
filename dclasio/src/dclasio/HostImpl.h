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
 * \file HostImpl.h
 *
 * \date 2011-10-26
 * \author Philipp Kegel
 */

#ifndef HOSTIMPL_H_
#define HOSTIMPL_H_

#include "ProcessImpl.h"
#include "SmartCLObjectRegistry.h"

#include <dcl/DCLTypes.h>
#include <dcl/Host.h>

#include <memory>

namespace dclasio {

namespace comm {

class DataDispatcher;
class MessageDispatcher;
class message_queue;

} // namespace comm

/* ****************************************************************************/

/*!
 * \brief An implementation of the host interface of the dOpenCL C++ compute node API.
 */
class HostImpl: public dcl::Host, public ProcessImpl {
public:
	/*!
	 * \brief Creates a host instance from a message queue connection
     * The data stream will be attached to this process later on using setDataStream.
	 *
     * \param[in]  id                   the host's process ID
     * \param[in]  messageDispatcher    an associated message dispatcher
     * \param[in]  dataDispatcher       an associated data dispatcher
     * \param[in]  messageQueue         an associated message queue (owned by message dispatcher)
	 */
	HostImpl(
	        dcl::process_id             id,
            comm::MessageDispatcher&    messageDispatcher,
            comm::DataDispatcher&       dataDispatcher,
            comm::message_queue&        messageQueue);
	virtual ~HostImpl();

	/* TODO Replace HostImpl::objectRegistry by ComputeNodeCommunicationManagerImpl::objectRegistry
     * HostImpl::objectRegistry is a temporary solution to ensure unique IDs
     * when multiple hosts are connected to a daemon.
     * Eventually, CommunicationManagerImpl::objectRegistry should be the global
     * registry used by all hosts and compute nodes. */
    SmartCLObjectRegistry& objectRegistry();

private:
	SmartCLObjectRegistry _objectRegistry;
};

} /* namespace dclasio */

#endif /* HOSTIMPL_H_ */
