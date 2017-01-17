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
 * \file CLRequestProcessor.h
 *
 * \date 2014-04-04
 * \author Philipp Kegel
 */

#ifndef CLREQUESTPROCESSOR_H_
#define CLREQUESTPROCESSOR_H_

#include <dcl/ComputeNode.h>
#include <dcl/Device.h>
#include <dcl/Event.h>
#include <dcl/Session.h>

#include <memory>
#include <vector>

namespace dclasio {

/* forward declaration */
class ComputeNodeCommunicationManagerImpl;
class HostImpl;
class SmartCLObjectRegistry;

namespace message {

class Request;
class Response;

} // namespace message

/* ****************************************************************************/

namespace comm {

/**
 * @brief A processor for incoming command requests.
 *
 * Requests are unmarshalled and forwarded to the application.
 */
class CLRequestProcessor {
public:
    CLRequestProcessor(
            ComputeNodeCommunicationManagerImpl& communicationManager);
    virtual ~CLRequestProcessor();

    bool dispatch(
            const message::Request& request,
            dcl::process_id         pid);

private:
    dcl::Session& getSession(
            const HostImpl& host) const;
    SmartCLObjectRegistry& getObjectRegistry(
            HostImpl& host) const;
    void getComputeNodes(
            const std::vector<dcl::process_id>& computeNodeIds,
            std::vector<dcl::ComputeNode *>&    computeNodes) const;
    void getDevices(
            const std::vector<dcl::object_id>&  deviceIds,
            std::vector<dcl::Device *>&         devices) const;
    void getEventWaitList(
            SmartCLObjectRegistry&                      registry,
            const std::vector<dcl::object_id>&          eventIdWaitList,
            std::vector<std::shared_ptr<dcl::Event>>&   eventWaitList) const;

    /**
     * @brief Execute a given request
     *
     * A request is unmarshalled and an appropriate method is called.
     * The method's output is marshalled into a response.
     *
     * @param[in]  request  the request
     * @param[in]  host     the requesting host
     * @return the response
     */
    template<class T>
    std::unique_ptr<message::Response> execute(
            const T&    request,
            HostImpl&   host);

    ComputeNodeCommunicationManagerImpl& _communicationManager;
};

} /* namespace comm */

} /* namespace dclasio */

#endif /* CLREQUESTPROCESSOR_H_ */
