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
 * \file    ComputeNode.cpp
 *
 * \date    01.11.2011
 * \author  Philipp Kegel
 */

#include <dcl/ComputeNode.h>

#include <dclasio/message/Message.h>
#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>

#include <dcl/Process.h>

#include <memory>
#include <vector>

namespace dcl {

void sendMessage(
        const std::vector<ComputeNode *>& computeNodes,
        dclasio::message::Message& message) {
    for (auto computeNode : computeNodes) {
        computeNode->sendMessage(message);
    }
}

void sendRequest(
		const std::vector<ComputeNode *>& computeNodes,
		dclasio::message::Request& request) {
    for (auto computeNode : computeNodes) {
        computeNode->sendRequest(request);
    }
}

void executeCommand(
		const std::vector<ComputeNode *>& computeNodes,
		dclasio::message::Request& request,
		dclasio::message::Response::class_type responseType,
		std::vector<std::unique_ptr<dclasio::message::Response>> *responses) {
	/* Send request to all compute nodes, such that the command is executed
	 * simultaneously */
	sendRequest(computeNodes, request);

	// Await responses from all compute nodes
	/* TODO Receive message from *all* compute nodes, i.e., do not stop receipt on first failure */
	if (responses) {
		responses->clear();
		responses->reserve(computeNodes.size());

	    for (auto computeNode : computeNodes) {
            // move response into responses
            responses->push_back(
                    computeNode->awaitResponse(request, responseType));
	    }
	} else {
	    for (auto computeNode : computeNodes) {
	        // discard response
	        computeNode->awaitResponse(request, responseType);
	    }
	}
}

} /* namespace dcl */
