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
 * \file    ComputeNode.h
 *
 * \date    2011-10-30
 * \author  Philipp Kegel
 *
 * C++ API declarations for dOpenCL communication layer
 */

#ifndef DCL_COMPUTENODE_H_
#define DCL_COMPUTENODE_H_

#include "Process.h"

/* TODO Remove message classes from ComputeNode interface */
#include <dclasio/message/Message.h>
#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>

#ifdef __APPLE__
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl_wwu_dcl.h>
#endif

#include <memory>
#include <vector>

namespace dcl {

class Binary;
class Device;

/* ****************************************************************************/

class ComputeNode: public virtual Process {
public:
    virtual ~ComputeNode() { }

    /*!
     * \brief Obtains a list of devices that are hosted by the compute node.
     *
     * \param[out]  devices a list of devices associated with this compute node
     */
    virtual void getDevices(
            std::vector<Device *>& devices) = 0;

    virtual void getInfo(
            cl_compute_node_info_WWU    param_name,
            Binary&                     param) const = 0;

    /*!
     * \brief Sends a request message to this compute node.
     *
     * \param[in]  request  the request to send
     */
    virtual void sendRequest(
            dclasio::message::Request& request) const = 0;

    /*!
     * \brief Waits for this compute node's response to the specified request.
     *
     * This methods throws a dcl::ProtocolException, if the response does not
     * have the expected type.
     *
     * \param[in]  request      the sent request
     * \param[in]  responseType the expected type of response
     * \return the received response
     */
    virtual std::unique_ptr<dclasio::message::Response> awaitResponse(
            const dclasio::message::Request&         request,
            dclasio::message::Response::class_type   responseType) = 0;
    /*!
     * \brief Waits for this compute node's response to the specified request.
     *
     * The expected response is Response::TYPE, i.e., a simple response which
     * only contains an error code.
     *
     * \param[in]  request  the sent request
     */
    virtual void awaitResponse(
            const dclasio::message::Request& request) = 0;

    /*!
     * \brief Execute a command on this compute node.
     *
     * This methods throws a dcl::ProtocolException, if the response does not
     * have the expected type.
     *
     * \param[in]  request      the request to send
     * \param[in]  responseType the expected type of response
     * \return the received response
     */
    virtual std::unique_ptr<dclasio::message::Response> executeCommand(
            dclasio::message::Request&               request,
            dclasio::message::Response::class_type   responseType) = 0;

    /*!
     * \brief Execute a command on this compute node.
     *
     * The expected response for this command is Response::TYPE, i.e., a simple
     * response which only contains an error code.
     *
     * \param[in]  request  the request to send
     */
    virtual void executeCommand(
            dclasio::message::Request& request) = 0;
};

/* ****************************************************************************/

/* The following function are free functions to avoid hiding of virtual methods.
 * E.g., if sendMessage becomes a static member function of ComputeNode, it
 * hides the virtual sendMessage which ComputeNode inherits from Process. */

/* TODO Discard ComputeNode::sendMessage
 * Use ComputeNodeImpl::sendMessage instead */
void sendMessage(
        const std::vector<ComputeNode *>&   computeNodes,
        dclasio::message::Message&           message);

/* TODO Discard ComputeNode::sendRequest
 * Use ComputeNodeImpl::sendRequest instead */
void sendRequest(
        const std::vector<ComputeNode *>&   computeNodes,
        dclasio::message::Request&           request);

/* TODO Discard ComputeNode::executeCommand
 * Use ComputeNodeImpl::executeCommand instead */
void executeCommand(
        const std::vector<ComputeNode *>&                           computeNodes,
        dclasio::message::Request&                                   request,
        dclasio::message::Response::class_type                       responseType = dclasio::message::DefaultResponse::TYPE,
        std::vector<std::unique_ptr<dclasio::message::Response>> *   responses = nullptr);

} /* namespace dcl */

#endif /* DCL_COMPUTENODE_H_ */
