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
 * \file CopyDataCommand.h
 *
 * \date 2014-01-19
 * \author Philipp Kegel
 */

#ifndef COPYDATACOMMAND_H_
#define COPYDATACOMMAND_H_

#include "Command.h"

#include <dclasio/message/CommandMessage.h>

#include <dcl/DataTransfer.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>
#include <dcl/Host.h>

#include <dcl/util/Logger.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.hpp>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cassert>
#include <functional>
#include <memory>

namespace dcld {

namespace command {

struct DeviceToHost {
    typedef const void *PointerType;

    static std::shared_ptr<dcl::DataTransfer> transferData(
            dcl::Host& host, size_t cb, PointerType ptr) {
        return host.sendData(cb, ptr);
    }
};

struct HostToDevice {
    typedef void *PointerType;

    static std::shared_ptr<dcl::DataTransfer> transferData(
            dcl::Host& host, size_t cb, PointerType ptr) {
        return host.receiveData(cb, ptr);
    }
};

/* ****************************************************************************/

template<typename Direction>
class CopyDataCommand: public Command {
public:
    CopyDataCommand(
            dcl::Host&                      host,
            dcl::object_id                  commandId,
            size_t                          cb,
            typename Direction::PointerType ptr,
            const cl::UserEvent&            event) :
        Command(host, commandId, event), _cb(cb), _ptr(ptr) {
    }

    void execute(
            cl_int errcode) {
        assert(errcode == CL_SUCCESS || errcode < 0);
        if (errcode == CL_SUCCESS) {
            try {
                // start data transfer on host
                dclasio::message::CommandExecutionStatusChangedMessage message(_commandId, CL_SUBMITTED);

                _host.sendMessage(message);

                dcl::util::Logger << dcl::util::Debug
                        << "Sent update of command execution status to host (ID=" << _commandId
                        << ", status=CL_SUBMITTED"
                        << ')' << std::endl;

                // start local data transfer
                /* Copy event to increase reference count. Thus, the user event
                 * will not be released until the data transfer has been
                 * completed and updated the event status */
                Direction::transferData(_host, _cb, _ptr)->setCallback(std::bind(
                        &cl::UserEvent::setStatus, _event, std::placeholders::_1));
            } catch (const dcl::IOException& err) {
                dcl::util::Logger << dcl::util::Error
                        << "Failed to send update of command execution status to host (ID=" << _commandId
                        << ", status=CL_SUBMITTED)"
                        << ", error: " << err.what()
                        << std::endl;

                _event.setStatus(CL_IO_ERROR_WWU);
            }
        } else {
            // forward execution status to associated event
            _event.setStatus(errcode);
        }
    }

private:
    size_t _cb;
    typename Direction::PointerType _ptr;
};

} /* namespace command */

} /* namespace dcld */

#endif /* COPYDATACOMMAND_H_ */
