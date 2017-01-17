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
 * \file ContextErrorMessage.h
 *
 * \date 2014-04-12
 * \author Sebastian Pribnow
 */

#ifndef CONTEXTERRORMESSAGE_H_
#define CONTEXTERRORMESSAGE_H_

#include <dclasio/message/Message.h>

#include <dcl/Binary.h>
#include <dcl/ByteBuffer.h>
#include <dcl/DCLTypes.h>

#include <string>

namespace dclasio {

namespace message {

/*!
 * \brief Notification of context error.
 *
 * This message is sent from compute nodes to the host.
 */
class ContextErrorMessage: public Message {
public:
    ContextErrorMessage() {
    }
    ContextErrorMessage(
            dcl::object_id contextId_,
            const std::string& errInfo_,
            const dcl::Binary& privateInfo_) :
            contextId(contextId_), errorInfo(errInfo_), privateInfo(
                    privateInfo_) {
    }
    ContextErrorMessage(const ContextErrorMessage& rhs) :
            contextId(rhs.contextId), errorInfo(rhs.errorInfo), privateInfo(
                    rhs.privateInfo) {
    }
    virtual ~ContextErrorMessage() {
    }

    dcl::object_id contextId;
    std::string errorInfo;
    dcl::Binary privateInfo;

    static const class_type TYPE = 8599;

    class_type get_type() const {
        return TYPE;
    }

    void pack(dcl::ByteBuffer& buf) const {
        buf << contextId << errorInfo << privateInfo;
    }

    void unpack(dcl::ByteBuffer& buf) {
        buf >> contextId >> errorInfo >> privateInfo;
    }
};

} /* namespace message */

} /* namespace dclasio */

#endif /* CONTEXTERRORMESSAGE_H_ */
