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
 * \file HostCommunicationManagerImpl.h
 *
 * \date 2011-10-26
 * \author Philipp Kegel
 */

#ifndef HOSTCOMMUNICATIONMANAGERIMPL_H_
#define HOSTCOMMUNICATIONMANAGERIMPL_H_

#include "CommunicationManagerImpl.h"

#include <dcl/CLObjectRegistry.h>
#include <dcl/CommunicationManager.h>
#include <dcl/ComputeNode.h>

#include <memory>
#include <string>
#include <vector>

namespace dclasio {

class ComputeNodeImpl;

namespace comm {

class CLResponseProcessor;

} // namespace message

namespace message {

class Message;

} // namespace message

/* ****************************************************************************/

class HostCommunicationManagerImpl:
        public CommunicationManagerImpl,
        public dcl::HostCommunicationManager {
public:
    HostCommunicationManagerImpl();
    virtual ~HostCommunicationManagerImpl();

    /*!
     * \brief Returns this communication manager's application object registry.
     *
     * \return an application object registry
     */
    dcl::CLObjectRegistry& objectRegistry();

    /*
     * Host communication manager API
     */
    dcl::ComputeNode * createComputeNode(
            const std::string& url);
    void createComputeNodes(
            const std::vector<std::string>&     urls,
            std::vector<dcl::ComputeNode *>&    computeNodes);
    void destroyComputeNode(
            dcl::ComputeNode *computeNode);

    /*
     * Message listener API
     */
    void message_received(
            comm::message_queue& msgq,
            const message::Message& message);

private:
    dcl::CLObjectRegistry _objectRegistry; //!< Registry for application objects

    std::unique_ptr<comm::CLResponseProcessor> _clResponseProcessor; //!< Processor for command responses
};

} /* namespace dclasio */

#endif /* HOSTCOMMUNICATIONMANAGERIMPL_H_ */
