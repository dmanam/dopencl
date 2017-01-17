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
 * \file SynchronizationListener.h
 *
 * \date 2013-01-30
 * \author Philipp Kegel
 * \author Sebastian Pribnow
 */

#ifndef DCL_SYNCHRONIZATIONLISTENER_H_
#define DCL_SYNCHRONIZATIONLISTENER_H_

namespace dcl {

class Process;

/* ****************************************************************************/

/*!
 * \brief A listener API for synchronization request
 *
 * A synchronization listener is informed when a process requests a synchronization.
 * This API is implemented by events in the dOpenCL ICD and daemon in order to perform memory updates.
 */
class SynchronizationListener {
public:
    virtual ~SynchronizationListener() { }

    /*!
     * \brief Synchronizes (releases) the changes associated with this event wrapper's native event
     *
     * Summarizes two scenarios:
     * - a compute node has to synchronize its memory object because of a found
     *   event listener and performs an acquire operation. In this case,
     *   onAcquire is called on the host to answer the request of the compute
     *   node. This is necessary, since the compute nodes can't communicate
     *   among each other at the moment. \c process then is the requesting
     *   compute node.
     *   If available, the host sends the updated copy of the requested memory
     *   object to the requesting compute node. Otherwise, the host performs an
     *   acquire operation on the compute node being the owner of the event.
     *   This leads to the second case:
     * - onAcquire is called on a compute node, triggered by an acquire
     *   operation on the host. In this case, \c process is the host and the
     *   compute node's copies of all memory objects associated with the event
     *   are sent to the host.
     *
     * \param[in]  process  the process (host or compute node) that requested the synchronization
     */
    virtual void onSynchronize(
            Process& process) = 0;

};

} /* namespace dcl */

#endif /* DCL_SYNCHRONIZATIONLISTENER_H_ */
