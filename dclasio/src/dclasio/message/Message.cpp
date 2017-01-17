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
 * \file Message.cpp
 *
 * \date 2014-03-26
 * \author Philipp Kegel
 */

#include "ContextErrorMessage.h"
#include "DeviceIDsResponse.h"
#include "DeviceInfosResponse.h"
#include "GetDeviceIDs.h"
#include "GetDeviceInfo.h"
#include "ProgramBuildMessage.h"

#include <dclasio/message/BuildProgram.h>
#include <dclasio/message/CreateBuffer.h>
#include <dclasio/message/CreateContext.h>
#include <dclasio/message/CreateCommandQueue.h>
#include <dclasio/message/CreateEvent.h>
#include <dclasio/message/CreateKernel.h>
#include <dclasio/message/CreateKernelsInProgram.h>
#include <dclasio/message/CreateProgramWithBinary.h>
#include <dclasio/message/CreateProgramWithSource.h>
#include <dclasio/message/CommandMessage.h>
#include <dclasio/message/DeleteCommandQueue.h>
#include <dclasio/message/DeleteContext.h>
#include <dclasio/message/DeleteEvent.h>
#include <dclasio/message/DeleteKernel.h>
#include <dclasio/message/DeleteMemory.h>
#include <dclasio/message/DeleteProgram.h>
#include <dclasio/message/EnqueueBarrier.h>
#include <dclasio/message/EnqueueBroadcastBuffer.h>
#include <dclasio/message/EnqueueCopyBuffer.h>
#include <dclasio/message/EnqueueMapBuffer.h>
#include <dclasio/message/EnqueueMarker.h>
#include <dclasio/message/EnqueueNDRangeKernel.h>
#include <dclasio/message/EnqueueReadBuffer.h>
#include <dclasio/message/EnqueueReduceBuffer.h>
#include <dclasio/message/EnqueueUnmapBuffer.h>
#include <dclasio/message/EnqueueWaitForEvents.h>
#include <dclasio/message/EnqueueWriteBuffer.h>
#include <dclasio/message/ErrorResponse.h>
#include <dclasio/message/EventProfilingInfosResponse.h>
#include <dclasio/message/EventSynchronizationMessage.h>
#include <dclasio/message/FinishRequest.h>
#include <dclasio/message/FlushRequest.h>
#include <dclasio/message/GetEventProfilingInfos.h>
#include <dclasio/message/GetKernelInfo.h>
#include <dclasio/message/InfoResponse.h>
#include <dclasio/message/Message.h>
#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>
#include <dclasio/message/SetKernelArg.h>

#include <stdexcept>

namespace dclasio {
namespace message {

Message * createMessage(Message::class_type type) {
    // TODO Register message classes to associate message type ID and message class automatically
    switch (type)
    {
    // event messages
    case CommandExecutionStatusChangedMessage::TYPE:
        return new CommandExecutionStatusChangedMessage();
    case ContextErrorMessage::TYPE:         return new ContextErrorMessage();
    case EventSynchronizationMessage::TYPE: return new EventSynchronizationMessage();
    case ProgramBuildMessage::TYPE:         return new ProgramBuildMessage();

    // request messages
    case BuildProgram::TYPE:                return new BuildProgram();
    case CreateBuffer::TYPE:                return new CreateBuffer();
    case CreateCommandQueue::TYPE:          return new CreateCommandQueue();
    case CreateContext::TYPE:               return new CreateContext();
    case CreateEvent::TYPE:                 return new CreateEvent();
    case CreateKernel::TYPE:                return new CreateKernel();
    case CreateKernelsInProgram::TYPE:      return new CreateKernelsInProgram();
    // TODO Implement CreateProgramWithBinary message
//    case CreateProgramWithBinary::TYPE:     return new CreateProgramWithBinary();
    case CreateProgramWithSource::TYPE:     return new CreateProgramWithSource();
    case DeleteCommandQueue::TYPE:          return new DeleteCommandQueue();
    case DeleteContext::TYPE:               return new DeleteContext();
    case DeleteEvent::TYPE:                 return new DeleteEvent();
    case DeleteKernel::TYPE:                return new DeleteKernel();
    case DeleteMemory::TYPE:                return new DeleteMemory();
    case DeleteProgram::TYPE:               return new DeleteProgram();
    case EnqueueBarrier::TYPE:              return new EnqueueBarrier();
    case EnqueueBroadcastBuffer::TYPE:      return new EnqueueBroadcastBuffer();
    case EnqueueCopyBuffer::TYPE:           return new EnqueueCopyBuffer();
    case EnqueueMapBuffer::TYPE:            return new EnqueueMapBuffer();
    case EnqueueMarker::TYPE:               return new EnqueueMarker();
    case EnqueueNDRangeKernel::TYPE:        return new EnqueueNDRangeKernel();
    case EnqueueReadBuffer::TYPE:           return new EnqueueReadBuffer();
    case EnqueueReduceBuffer::TYPE:         return new EnqueueReduceBuffer();
    case EnqueueUnmapBuffer::TYPE:          return new EnqueueUnmapBuffer();
    case EnqueueWaitForEvents::TYPE:        return new EnqueueWaitForEvents();
    case EnqueueWriteBuffer::TYPE:          return new EnqueueWriteBuffer();
    case FinishRequest::TYPE:               return new FinishRequest();
    case FlushRequest::TYPE:                return new FlushRequest();
    case GetDeviceIDs::TYPE:                return new GetDeviceIDs();
    case GetDeviceInfo::TYPE:               return new GetDeviceInfo();
    case GetEventProfilingInfos::TYPE:      return new GetEventProfilingInfos();
    case GetKernelInfo::TYPE:               return new GetKernelInfo();
    case GetKernelWorkGroupInfo::TYPE:      return new GetKernelWorkGroupInfo();
    // TODO Implement GetProgramBuildLog message
//    case GetProgramBuildLog::TYPE:          return new GetProgramBuildLog();
    case SetKernelArg::TYPE:                return new SetKernelArg();
    case SetKernelArgBinary::TYPE:          return new SetKernelArgBinary();
    case SetKernelArgMemObject::TYPE:       return new SetKernelArgMemObject();

    // response messages
    case DefaultResponse::TYPE:             return new DefaultResponse();
    case DeviceIDsResponse::TYPE:           return new DeviceIDsResponse();
    case DeviceInfosResponse::TYPE:         return new DeviceInfosResponse();
    case EventProfilingInfosReponse::TYPE:  return new EventProfilingInfosReponse();
    case ErrorResponse::TYPE:               return new ErrorResponse();
    case InfoResponse::TYPE:                return new InfoResponse();

    default:
        throw std::invalid_argument("Invalid message type");
        /* no break */
    }
}

} /* namespace message */
} /* namespace dclasio */
