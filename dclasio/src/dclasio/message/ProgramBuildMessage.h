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
 * \file ProgramBuildMessage.h
 *
 * Messages regarding program build.
 *
 * \date 2011-12-23
 * \author Philipp Kegel
 */

#ifndef PROGRAMBUILDMESSAGE_H_
#define PROGRAMBUILDMESSAGE_H_

#include <dclasio/message/Message.h>

#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <vector>

namespace dclasio {

namespace message {

/**
 * @brief Notification of completion of program build.
 *
 * This message is sent from compute nodes to the host to indicate completion of program build.
 * The build status of each device is returned.
 */
class ProgramBuildMessage: public Message {
public:
	ProgramBuildMessage() {
	}

	ProgramBuildMessage(
	        dcl::object_id programId_,
	        const std::vector<dcl::object_id>& deviceIds_,
	        const std::vector<cl_build_status>& buildStatus_):
	    programBuildId(programId_), deviceIds(deviceIds_), buildStatus(buildStatus_) {
	}

	ProgramBuildMessage(
	        const ProgramBuildMessage& rhs) :
	    programBuildId(rhs.programBuildId), deviceIds(rhs.deviceIds),
	    buildStatus(rhs.buildStatus) {
	}

	~ProgramBuildMessage() { }

    dcl::object_id programBuildId;
    std::vector<dcl::object_id> deviceIds;
    std::vector<cl_build_status> buildStatus;

	static const class_type TYPE = 701;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        buf << programBuildId << deviceIds << buildStatus;
    }

    void unpack(dcl::ByteBuffer& buf) {
        buf >> programBuildId >> deviceIds >> buildStatus;
    }
};

} /* namespace message */

} /* namespace dclasio */

#endif /* PROGRAMBUILDMESSAGE_H_ */
