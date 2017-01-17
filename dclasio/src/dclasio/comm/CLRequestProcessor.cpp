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
 * \file CLRequestProcessor.cpp
 *
 * \date 2014-04-04
 * \author Philipp Kegel
 */

#include "CLRequestProcessor.h"

#include "../ComputeNodeCommunicationManagerImpl.h"
#include "../ComputeNodeImpl.h"
#include "../ContextListenerImpl.h"
#include "../HostImpl.h"
#include "../ProgramBuildListenerImpl.h"
#include "../SmartCLObjectRegistry.h"

#include "../comm/DataTransferImpl.h"

#include "../message/DeviceIDsResponse.h"
//#include "../message/DeviceInfosResponse.h"
#include "../message/GetDeviceIDs.h"
#include "../message/GetDeviceInfo.h"

#include <dclasio/message/BuildProgram.h>
#include <dclasio/message/CreateBuffer.h>
#include <dclasio/message/CreateCommandQueue.h>
#include <dclasio/message/CreateContext.h>
#include <dclasio/message/CreateEvent.h>
#include <dclasio/message/CreateKernel.h>
#include <dclasio/message/CreateKernelsInProgram.h>
#include <dclasio/message/CreateProgramWithSource.h>
#include <dclasio/message/DeleteMemory.h>
#include <dclasio/message/DeleteCommandQueue.h>
#include <dclasio/message/DeleteContext.h>
#include <dclasio/message/DeleteEvent.h>
#include <dclasio/message/DeleteKernel.h>
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
#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
#include <dclasio/message/EnqueueWaitForEvents.h>
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)
#include <dclasio/message/EnqueueWriteBuffer.h>
#include <dclasio/message/ErrorResponse.h>
#include <dclasio/message/EventProfilingInfosResponse.h>
#include <dclasio/message/FinishRequest.h>
#include <dclasio/message/FlushRequest.h>
#include <dclasio/message/GetEventProfilingInfos.h>
#include <dclasio/message/GetKernelInfo.h>
#include <dclasio/message/InfoResponse.h>
#include <dclasio/message/Request.h>
#include <dclasio/message/Response.h>
#include <dclasio/message/SetKernelArg.h>

#include <dcl/CommandQueue.h>
#include <dcl/Context.h>
#include <dcl/ContextListener.h>
#include <dcl/DCLTypes.h>
#include <dcl/Device.h>
#include <dcl/Event.h>
#include <dcl/Kernel.h>
#include <dcl/Memory.h>
#include <dcl/Program.h>
#include <dcl/Session.h>

#include <dcl/util/Logger.h>

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <CL/cl_wwu_dcl.h>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

namespace {

/* A helper function to wrap new responses into a unique pointer */
template<typename T, typename ... Args>
std::unique_ptr<T> make_unique(Args&& ... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args) ...));
}

} /* unnamed namespace */

/* ****************************************************************************/

namespace dclasio {
namespace comm {

CLRequestProcessor::CLRequestProcessor(
        ComputeNodeCommunicationManagerImpl& communicationManager) :
        _communicationManager(communicationManager) {
}

CLRequestProcessor::~CLRequestProcessor() {
}

dcl::Session& CLRequestProcessor::getSession(
        const HostImpl& host) const {
    dcl::Session *session = _communicationManager.getDaemon()->getSession(host);
    assert(session);
    return *session;
}

SmartCLObjectRegistry& CLRequestProcessor::getObjectRegistry(
        HostImpl& host) const {
    /* TODO Do not make registry a member of host */
    return host.objectRegistry();
}

void CLRequestProcessor::getComputeNodes(
        const std::vector<dcl::process_id>& computeNodeIds,
        std::vector<dcl::ComputeNode *>& computeNodes) const {
    computeNodes.clear();

    /* TODO Lookup (or create) compute nodes
     * This operation can introduce new compute nodes to this compute node */
}

void CLRequestProcessor::getDevices(
        const std::vector<dcl::object_id>& deviceIds,
        std::vector<dcl::Device *>& devices) const {
    /* TODO Resolve device IDs */
    _communicationManager.objectRegistry().lookup(deviceIds, devices);
}

void CLRequestProcessor::getEventWaitList(
        SmartCLObjectRegistry& registry,
        const std::vector<dcl::object_id>& eventIdWaitList,
        std::vector<std::shared_ptr<dcl::Event>>& eventWaitList) const {
    eventWaitList.clear();
    eventWaitList.reserve(eventIdWaitList.size());

    for (auto eventId : eventIdWaitList) {
        auto event = registry.lookup<std::shared_ptr<dcl::Event>>(eventId);
        if (!event) { // event not found
            throw cl::Error(CL_INVALID_EVENT_WAIT_LIST);
        }

        eventWaitList.push_back(event);
    }
}

/******************************************************************************/

/* TODO Use unified registry for all processes
 * A registry is provided by each host as each host assigns its own (and thus
 * possibly identical) IDs to objects. Therefore, lookup host in compute node's
 * connection manager, obtain registry, and lookup object in registry, rather
 * than in the session.
 * This also requires to register newly created objects in the registry within
 * this class.
 *
 * Resolve an object ID:
         HostImpl *host = _connectionManager.getHost();
         assert(host != nullptr);
         host->objectRegistry().lookup...(...Id);
 * Register a new object:
         object = ...(...Id);
         HostImpl *host = _connectionManager.getHost();
         assert(host != nullptr);
         host->objectRegistry().register...(...Id, object);
 *
 * Object lookup is hard to implement, if the request is not sent by a host but
 * rather another compute node or a device manager (formerly resource manager).
 * This is, e.g., the case if events status changes are sent between compute
 * nodes, directly.
 * In these cases, it is not clear at which registry the objects are registered
 * whose IDs are used by the compute node or device manager.
 *
 * Device IDs should not be resolved by a host's registry, as devices are not
 * directly associated which a host but rather a platform.
 * Solution idea: use connection manager to assign and resolve device IDs
 * 1) add platform (cf. DeviceManager) to ComputeNodeCommunicationManager
 * 2) Connection manager obtains list of all devices from platform and assigns
 *    IDs to all devices
 */

#if 0
/**
 * @brief Obtain all platform information and return as compute node information.
 */
template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::message::GetComputeNodeInfos& request,
        HostImpl& host) {
    try {
        dcl::ComputeNode computeNode = _communicationManager.getComputeNode();

        return make_unique<message::ComputeNodeInfosResponse>(request,
                computeNode.getInfo<CL_PLATFORM_EXTENSIONS>(),
                computeNode.getInfo<CL_PLATFORM_NAME>(),
                computeNode.getInfo<CL_PLATFORM_PROFILE>(),
                computeNode.getInfo<CL_PLATFORM_VENDOR>(),
                computeNode.getInfo<CL_PLATFORM_VERSION>());
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}
#endif

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::GetDeviceIDs& request,
        HostImpl& host) {
    try {
        std::vector<dcl::object_id> deviceIDs;

        _communicationManager.objectRegistry().getIDs<dcl::Device *>(deviceIDs);

        dcl::util::Logger << dcl::util::Info
                << "Got device IDs" << std::endl;

        /* TODO Return list of (device ID, device type) pairs */
        return make_unique<message::DeviceIDsResponse>(request, deviceIDs);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::GetDeviceInfo& request,
        HostImpl& host) {
    dcl::Binary param;

    try {
        auto device = _communicationManager.objectRegistry().lookup<dcl::Device *>(request.deviceId);

        device->getInfo(request.paramName, param);

        dcl::util::Logger << dcl::util::Info
                << "Got device info (device ID=" << request.deviceId
                << ')' << std::endl;

        return make_unique<message::InfoResponse>(request, param.size(), param.value());
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::CreateContext& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<dcl::ComputeNode *> computeNodes;
    std::vector<dcl::Device *> devices;

    try {
        // TODO Read compute node IDs from message
        getDevices(request.deviceIds(), devices);

        std::shared_ptr<dcl::ContextListener> contextListener(
                std::make_shared<ContextListenerImpl>(request.contextId(), host));
        auto context = getSession(host).createContext(
                host, computeNodes, devices, contextListener);

        registry.bind(request.contextId(), context);

        /* TODO Asynchronously connect to created compute nodes
         * Return response when compute nodes have been connected */

        dcl::util::Logger << dcl::util::Info
                << "Context created (ID=" << request.contextId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::DeleteContext& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        getSession(host).releaseContext(
                registry.lookup<std::shared_ptr<dcl::Context>>(request.contextId()));
        registry.unbind<std::shared_ptr<dcl::Context>>(request.contextId());

        dcl::util::Logger << dcl::util::Info
                << "Context released (ID=" << request.contextId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::CreateBuffer& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        cl_mem_flags hostPtrFlags = request.flags() &
                (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR);
        size_t size = request.size();
        std::unique_ptr<unsigned char[]> host_ptr;

        if (hostPtrFlags) {
            /* receive buffer data from host */
            host_ptr.reset(new unsigned char[size]);
            if (!host_ptr) throw cl::Error(CL_OUT_OF_RESOURCES);
            host.receiveData(size, host_ptr.get())->wait();
        }

        auto buffer = getSession(host).createBuffer(
                registry.lookup<std::shared_ptr<dcl::Context>>(request.contextId()),
                request.flags(), request.size(), host_ptr.get());
        registry.bind(request.bufferId(), buffer);

        dcl::util::Logger << dcl::util::Info
                << "Buffer created (ID=" << request.bufferId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::DeleteMemory& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        getSession(host).releaseMemObject(
                registry.lookupMemory(request.memObjectId()));
        registry.unbindMemory(request.memObjectId());

        dcl::util::Logger << dcl::util::Info
                << "Memory object released (ID=" << request.memObjectId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::CreateCommandQueue& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        auto device = _communicationManager.objectRegistry().lookup<dcl::Device *>(request.deviceId());

        auto commandQueue = getSession(host).createCommandQueue(
                registry.lookup<std::shared_ptr<dcl::Context>>(request.contextId()),
                device,
                request.properties());
        registry.bind(request.commandQueueId(), commandQueue);

        dcl::util::Logger << dcl::util::Info
                << "Command queue created (ID=" << request.commandQueueId() << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::DeleteCommandQueue& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        getSession(host).releaseCommandQueue(
                registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId()));
        registry.unbind<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId());

        dcl::util::Logger << dcl::util::Info
                << "Command queue released (ID=" << request.commandQueueId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::CreateProgramWithSource& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::unique_ptr<char[]> source;
    size_t length = request.length();

    /* Receive program sources */
    try {
        source.reset(new char[length]);
        host.receiveData(length, source.get())->wait(); // blocking receive
    } catch (const std::bad_alloc&) {
        return make_unique<message::ErrorResponse>(request, CL_OUT_OF_RESOURCES);
    }

    try {
        auto program = getSession(host).createProgram(
                registry.lookup<std::shared_ptr<dcl::Context>>(request.contextId()),
                source.get(), length);
        registry.bind(request.programId(), program);

        dcl::util::Logger << dcl::util::Info
                << "Program created from source (ID=" << request.programId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

#if 0
template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::CreateProgramWithBinary& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::unique_ptr<std::unique_ptr<unsigned char[]>[]> strings;
    size_t *lengths;
    unsigned char **binaries;
    VECTOR_CLASS<cl_int> binary_status;

    /* Receive program binaries */
    try {
        strings.reset(new std::unique_ptr<char[]>[lengths.size()]);

        for (unsigned int i = 0; i < lengths.size(); ++i) {
            size_t length = lengths[i];

            strings[i].reset(new unsigned char[length]);
            void *binary = static_cast<void *>(strings[i].get());
            host.receiveData(length, binary)->wait(); // blocking receive
        }

        /* DO NOT DELETE strings, as binaries hold pointers to strings */
    } catch (const std::bad_alloc&) {
        return make_unique<message::ErrorResponse>(request, CL_OUT_OF_RESOURCES);
    }

    /* TODO Initialize lengths and binaries arguments for dcld::Session::createProgram */

    try {
        auto program = getSession(host).createProgram(
                registry.lookup<std::shared_ptr<dcl::Context>>(request.contextId()),
                lengths,
                binaries,
                &binary_status);
        registry.bind(request.programId(), program);

        dcl::util::Logger << dcl::util::Info
                << "Program created from binaries (ID=" << request.programId() << ')'
                << std::endl;

        return make_unique<message::ProgramBinaryStatusResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }

    /* Now strings may be deleted safely */
}
#endif

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::DeleteProgram& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        getSession(host).releaseProgram(
                registry.lookup<std::shared_ptr<dcl::Program>>(request.programId()));
        registry.unbind<std::shared_ptr<dcl::Program>>(request.programId());

        dcl::util::Logger << dcl::util::Info
                << "Program released (ID=" << request.programId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::BuildProgram& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        auto program = registry.lookup<std::shared_ptr<dcl::Program>>(request.programId());
        std::vector<dcl::Device *> devices;

        getDevices(request.deviceIds(), devices);

        /* TODO Register program build listener; manage ID externally */
        std::shared_ptr<dcl::ProgramBuildListener> programBuildListener(
                std::make_shared<ProgramBuildListenerImpl>(request.programBuildId(), host));
        program->build(devices, request.options().c_str(), programBuildListener);

        dcl::util::Logger << dcl::util::Info
                << "Program build submitted (program ID=" << request.programId()
                << ", build ID=" << request.programBuildId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err){
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::CreateKernel& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        auto kernel = getSession(host).createKernel(
                registry.lookup<std::shared_ptr<dcl::Program>>(
                        request.programId()), request.kernelName());
        registry.bind(request.kernelId(), kernel);

        dcl::util::Logger << dcl::util::Info
                << "Kernel created (ID=" << request.kernelId()
                << ", name=" << request.kernelName()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err){
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::CreateKernelsInProgram& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        auto kernels = getSession(host).createKernelsInProgram(
                registry.lookup<std::shared_ptr<dcl::Program>>(request.programId()),
                request.kernelIds().size());

        /* Register kernels */
        auto id = std::begin(request.kernelIds());
        auto kernel = std::begin(kernels);
        while (id != std::end(request.kernelIds()) && kernel != std::end(kernels)) {
            registry.bind(*id++, *kernel++);
        }

        dcl::util::Logger << dcl::util::Info
                << "Kernels in program created (program ID=" << request.programId()
                << ", #kernels=" << kernels.size()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::DeleteKernel& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        getSession(host).releaseKernel(
                registry.lookup<std::shared_ptr<dcl::Kernel>>(request.kernelId()));
        registry.unbind<std::shared_ptr<dcl::Kernel>>(request.kernelId());

        dcl::util::Logger << dcl::util::Info
                << "Kernel released (ID=" << request.kernelId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::CreateEvent& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Memory>> memoryObjects;

    try {
        /* Resolve memory object IDs */
        memoryObjects.reserve(request.memObjectIds().size());
        for (auto memObjectId : request.memObjectIds()) {
            memoryObjects.push_back(registry.lookupMemory(memObjectId));
        }

        std::shared_ptr<dcl::Event> event = getSession(host).createEvent(
                request.eventId(),
                registry.lookup<std::shared_ptr<dcl::Context>>(request.contextId()),
                memoryObjects);
        registry.bind(request.eventId(), event);

        dcl::util::Logger << dcl::util::Info
                << "Event created (ID=" << request.eventId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::DeleteEvent& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        getSession(host).releaseEvent(
                registry.lookup<std::shared_ptr<dcl::Event>>(request.eventId()));
        registry.unbind<std::shared_ptr<dcl::Event>>(request.eventId());

        dcl::util::Logger << dcl::util::Info
                << "Event released (ID=" << request.eventId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::GetEventProfilingInfos& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        cl_ulong received, queued, submit, start, end;

        auto event = registry.lookup<std::shared_ptr<dcl::Event>>(request.eventId());

        event->getProfilingInfo(CL_PROFILING_COMMAND_RECEIVED_WWU, received);
        event->getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, queued);
        event->getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, submit);
        event->getProfilingInfo(CL_PROFILING_COMMAND_START, start);
        event->getProfilingInfo(CL_PROFILING_COMMAND_END, end);

        dcl::util::Logger << dcl::util::Info
                << "Got event profiling info (ID=" << request.eventId() << ')'
                << std::endl;

        return make_unique<message::EventProfilingInfosReponse>(
                request, received, queued, submit, start, end);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::GetKernelInfo& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        dcl::Binary param;

        registry.lookup<std::shared_ptr<dcl::Kernel>>(request.kernelId())->getInfo(
                request.paramName(), param);

        dcl::util::Logger << dcl::util::Info
                << "Got kernel info (ID=" << request.kernelId() << ')'
                << std::endl;

        return make_unique<message::InfoResponse>(request, param.size(), param.value());
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::GetKernelWorkGroupInfo& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    dcl::Binary param;

    try {
        auto device = _communicationManager.objectRegistry().lookup<dcl::Device *>(request.deviceId());

        registry.lookup<std::shared_ptr<dcl::Kernel>>(request.kernelId())->getWorkGroupInfo(
                device, request.paramName(), param);

        dcl::util::Logger << dcl::util::Info
                << "Got kernel work group info (kernel ID=" << request.kernelId()
                << ", device ID=" << request.deviceId()
                << ')' << std::endl;

        return make_unique<message::InfoResponse>(
                request, param.size(), param.value());
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueCopyBuffer& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Event>> eventWaitList;
    std::shared_ptr<dcl::Event> copyBuffer;

    try {
        getEventWaitList(registry, request.eventIdWaitList(), eventWaitList);

        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->enqueueCopyBuffer(
                registry.lookup<std::shared_ptr<dcl::Buffer>>(request.srcBufferId()),
                registry.lookup<std::shared_ptr<dcl::Buffer>>(request.dstBufferId()),
                request.srcOffset(), request.dstOffset(), request.cb(),
                (eventWaitList.empty() ? nullptr : &eventWaitList),
                request.commandId(),
                (request.event() ? &copyBuffer : nullptr)
        );

        if (copyBuffer) { // an event should be associated with this command
            /* FIXME Add event to session automatically */
            getSession(host).addEvent(copyBuffer);
            registry.bind(request.commandId(), copyBuffer);
        }

        dcl::util::Logger << dcl::util::Info
                << "Enqueued copy buffer (command queue ID=" << request.commandQueueId()
                << ", src buffer ID=" << request.srcBufferId()
                << ", dst buffer ID=" << request.dstBufferId()
                << ", command ID=" << request.commandId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueWriteBuffer& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Event>> eventWaitList;
    std::shared_ptr<dcl::Event> writeBuffer;

    try {
        getEventWaitList(registry, request.eventIdWaitList(), eventWaitList);

        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->enqueueWriteBuffer(
                registry.lookup<std::shared_ptr<dcl::Buffer>>(request.bufferId()),
                request.blocking(), request.offset(),
                request.cb(),
                (eventWaitList.empty() ? nullptr : &eventWaitList),
                request.commandId(),
                (request.event() ? &writeBuffer : nullptr)
        );

        if (writeBuffer) { // an event should be associated with this command
            /* FIXME Add event to session automatically */
            getSession(host).addEvent(writeBuffer);
            registry.bind(request.commandId(), writeBuffer);
        }

        dcl::util::Logger << dcl::util::Info
                << "Enqueued data upload to buffer (command queue ID="
                << request.commandQueueId() << ", buffer ID=" << request.bufferId()
                << ", command ID=" << request.commandId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueReadBuffer& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Event>> eventWaitList;
    std::shared_ptr<dcl::Event> readBuffer;

    try {
        getEventWaitList(registry, request.eventIdWaitList(), eventWaitList);

        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->enqueueReadBuffer(
                registry.lookup<std::shared_ptr<dcl::Buffer>>(request.bufferId()),
                request.blocking(), request.offset(),
                request.cb(),
                (eventWaitList.empty() ? nullptr : &eventWaitList),
                request.commandId(),
                (request.event() ? &readBuffer : nullptr)
        );

        if (readBuffer) { // an event should be associated with this command
            /* FIXME Add event to session automatically */
            getSession(host).addEvent(readBuffer);
            registry.bind(request.commandId(), readBuffer);
        }

        dcl::util::Logger << dcl::util::Info
                << "Enqueued data download from buffer (command queue ID="
                << request.commandQueueId() << ", buffer ID=" << request.bufferId()
                << ", command ID=" << request.commandId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueBroadcastBuffer& request,
        HostImpl& host) {
    try {
        /* TODO Implement and call dcl::CommandQueue::enqueueBroadcastBuffer */
        assert(!"dcl::CommandQueue::enqueueBroadcastBuffer not implemented");

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueReduceBuffer& request,
        HostImpl& host) {
    try {
        /* TODO Implement and call dcl::CommandQueue::enqueueReduceBuffer */
        assert(!"dcl::CommandQueue::enqueueReduceBuffer not implemented");

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueNDRangeKernel& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Event>> eventWaitList;
    std::shared_ptr<dcl::Event> ndRangeKernel;

    try {
        getEventWaitList(registry, request.eventIdWaitList(), eventWaitList);

        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->enqueueNDRangeKernel(
                registry.lookup<std::shared_ptr<dcl::Kernel>>(request.kernelId()),
                request.offset(), request.global(), request.local(),
                (eventWaitList.empty() ? nullptr : &eventWaitList),
                request.commandId(),
                (request.event() ? &ndRangeKernel : nullptr)
        );

        if (ndRangeKernel) { // an event should be associated with this command
            /* FIXME Add event to session automatically */
            getSession(host).addEvent(ndRangeKernel);
            registry.bind(request.commandId(), ndRangeKernel);
        }

        dcl::util::Logger << dcl::util::Info
                << "Enqueued ND range kernel (command queue ID=" << request.commandQueueId()
                << ", kernel ID=" << request.kernelId()
                << ", command ID=" << request.commandId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueBarrier& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Event>> eventWaitList;
    std::shared_ptr<dcl::Event> barrier;

    try {
        getEventWaitList(registry, request.eventIdWaitList(), eventWaitList);

        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->enqueueBarrier(
                (eventWaitList.empty() ? nullptr : &eventWaitList),
                request.commandId(),
                (request.event() ? &barrier : nullptr)
        );

        if (barrier) { // an event should be associated with this command
            /* FIXME Add event to session automatically */
            getSession(host).addEvent(barrier);
            registry.bind(request.commandId(), barrier);
        }

        dcl::util::Logger << dcl::util::Info
                << "Enqueued barrier (command queue ID=" << request.commandQueueId()
                << ", command ID=" << request.commandId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueWaitForEvents& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Event>> eventList;

    try {
        /* Lookup events of event list
         * Unlike other enqueued commands, wait for events throws CL_INVALID_EVENT
         * rather than CL_INVALID_EVENT_WAIT_LIST if the event list contains an
         * invalid event. Moreover, the event list must not contain user events */
        eventList.reserve(request.eventIdList().size());
        for (auto eventId : request.eventIdList()) {
            eventList.push_back(registry.lookup<std::shared_ptr<dcl::Event>>(eventId));
        }

        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->enqueueWaitForEvents(
                eventList);

        dcl::util::Logger << dcl::util::Info
                << "Enqueued wait for events (command queue ID=" << request.commandQueueId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueMapBuffer& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Event>> eventWaitList;
    std::shared_ptr<dcl::Event> mapBuffer;

    try {
        getEventWaitList(registry, request.eventIdWaitList(), eventWaitList);

        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->enqueueMapBuffer(
                registry.lookup<std::shared_ptr<dcl::Buffer>>(request.bufferId()),
                request.blocking(), request.mapFlags(),
                request.offset(), request.cb(),
                (eventWaitList.empty() ? nullptr : &eventWaitList),
                request.commandId(),
                (request.event() ? &mapBuffer : nullptr)
        );

        if (mapBuffer) { // an event should be associated with this command
            /* FIXME Add event to session automatically */
            getSession(host).addEvent(mapBuffer);
            registry.bind(request.commandId(), mapBuffer);
        }

        dcl::util::Logger << dcl::util::Info
                << "Enqueued map buffer (command queue ID=" << request.commandQueueId()
                << ", command ID=" << request.commandId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueUnmapBuffer& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Event>> eventWaitList;
    std::shared_ptr<dcl::Event> unmapBuffer;

    try {
        getEventWaitList(registry, request.eventIdWaitList(), eventWaitList);

        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->enqueueUnmapBuffer(
                registry.lookup<std::shared_ptr<dcl::Buffer>>(request.bufferId()),
                request.mapFlags(),
                request.offset(), request.cb(),
                (eventWaitList.empty() ? nullptr : &eventWaitList),
                request.commandId(),
                (request.event() ? &unmapBuffer : nullptr)
        );

        if (unmapBuffer) { // an event should be associated with this command
            /* FIXME Add event to session automatically */
            getSession(host).addEvent(unmapBuffer);
            registry.bind(request.commandId(), unmapBuffer);
        }

        dcl::util::Logger << dcl::util::Info
                << "Enqueued unmap buffer (command queue ID=" << request.commandQueueId()
                << ", command ID=" << request.commandId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::EnqueueMarker& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);
    std::vector<std::shared_ptr<dcl::Event>> eventWaitList;
    std::shared_ptr<dcl::Event> marker;

    try {
        getEventWaitList(registry, request.eventIdWaitList(), eventWaitList);

        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->enqueueMarker(
                (eventWaitList.empty() ? nullptr : &eventWaitList),
                request.commandId(),
                (request.event() ? &marker : nullptr)
        );

        if (marker) { // an event should be associated with this command
            /* FIXME Add event to session automatically */
            getSession(host).addEvent(marker);
            registry.bind(request.commandId(), marker);
        }

        dcl::util::Logger << dcl::util::Info
                << "Enqueued marker (command queue ID=" << request.commandQueueId()
                << ", command ID=" << request.commandId()
                << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::FinishRequest& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        /* TODO Finish command queue asynchronously */
        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->finish();

        dcl::util::Logger << dcl::util::Info
                << "Finished command queue (ID=" << request.commandQueueId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::FlushRequest& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        registry.lookup<std::shared_ptr<dcl::CommandQueue>>(request.commandQueueId())->flush();

        dcl::util::Logger << dcl::util::Info
                << "Flushed command queue (ID=" << request.commandQueueId() << ')'
                << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::SetKernelArgMemObject& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        auto kernel = registry.lookup<std::shared_ptr<dcl::Kernel>>(request.kernelId());

        if (request.argValue() == nullptr) {
            kernel->setArg(request.argIndex(), request.argSize());
        } else {
            auto memory = registry.lookupMemory(
                    *reinterpret_cast<const dcl::object_id *>(request.argValue()));
            kernel->setArg(request.argIndex(), memory);
        }

        dcl::util::Logger << dcl::util::Info
                << "Kernel argument set (ID=" << request.kernelId() << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

template<>
std::unique_ptr<message::Response> CLRequestProcessor::execute(
        const message::SetKernelArgBinary& request,
        HostImpl& host) {
    SmartCLObjectRegistry& registry = getObjectRegistry(host);

    try {
        registry.lookup<std::shared_ptr<dcl::Kernel>>(request.kernelId())->setArg(
                request.argIndex(), request.argSize(), request.argValue());

        dcl::util::Logger << dcl::util::Info
                << "Kernel argument set (ID=" << request.kernelId() << ')' << std::endl;

        return make_unique<message::DefaultResponse>(request);
    } catch (const cl::Error& err) {
        return make_unique<message::ErrorResponse>(request, err.err());
    }
}

bool CLRequestProcessor::dispatch(
        const message::Request& request,
        dcl::process_id pid) {
    std::unique_ptr<message::Response> response;

    HostImpl *host = _communicationManager.get_host(pid);
    /* TODO Discard request from unknown processes
     * Only processes that have identified ('connected') themselves as host
     * or possibly also compute nodes are allowed to issue requests. */
    assert(host && "No host for request");
    if (!host)
        return false;

    /*
     * Dispatch request
     */
    switch (request.get_type()) {

    /* Requests sent by any dOpenCL node */
    case message::GetDeviceIDs::TYPE:
        response = execute<message::GetDeviceIDs>(
                static_cast<const message::GetDeviceIDs&>(request), *host);
        break;
    case message::GetDeviceInfo::TYPE:
        response = execute<message::GetDeviceInfo>(
                static_cast<const message::GetDeviceInfo&>(request), *host);
        break;

    /* Request sent by hosts */
    case message::CreateContext::TYPE:
        response = execute<message::CreateContext>(
                static_cast<const message::CreateContext&>(request), *host);
        break;
    case message::DeleteContext::TYPE:
        response = execute<message::DeleteContext>(
                static_cast<const message::DeleteContext&>(request), *host);
        break;
    case message::DeleteMemory::TYPE:
        response = execute<message::DeleteMemory>(
                static_cast<const message::DeleteMemory&>(request), *host);
        break;
    case message::CreateBuffer::TYPE:
        response = execute<message::CreateBuffer>(
                static_cast<const message::CreateBuffer&>(request), *host);
        break;
    case message::CreateCommandQueue::TYPE:
        response = execute<message::CreateCommandQueue>(
                static_cast<const message::CreateCommandQueue&>(request), *host);
        break;
    case message::DeleteCommandQueue::TYPE:
        response = execute<message::DeleteCommandQueue>(
                static_cast<const message::DeleteCommandQueue&>(request), *host);
        break;
    case message::EnqueueBarrier::TYPE:
        response = execute<message::EnqueueBarrier>(
                static_cast<const message::EnqueueBarrier&>(request), *host);
        break;
    case message::EnqueueBroadcastBuffer::TYPE:
        response = execute<message::EnqueueBroadcastBuffer>(
                static_cast<const message::EnqueueBroadcastBuffer&>(request), *host);
        break;
    case message::EnqueueCopyBuffer::TYPE:
        response = execute<message::EnqueueCopyBuffer>(
                static_cast<const message::EnqueueCopyBuffer&>(request), *host);
        break;
    case message::EnqueueMapBuffer::TYPE:
        response = execute<message::EnqueueMapBuffer>(
                static_cast<const message::EnqueueMapBuffer&>(request), *host);
        break;
    case message::EnqueueMarker::TYPE:
        response = execute<message::EnqueueMarker>(
                static_cast<const message::EnqueueMarker&>(request), *host);
        break;
    case message::EnqueueNDRangeKernel::TYPE:
        response = execute<message::EnqueueNDRangeKernel>(
                static_cast<const message::EnqueueNDRangeKernel&>(request), *host);
        break;
    case message::EnqueueWriteBuffer::TYPE:
        response = execute<message::EnqueueWriteBuffer>(
                static_cast<const message::EnqueueWriteBuffer&>(request), *host);
        break;
    case message::EnqueueReadBuffer::TYPE:
        response = execute<message::EnqueueReadBuffer>(
                static_cast<const message::EnqueueReadBuffer&>(request), *host);
        break;
    case message::EnqueueReduceBuffer::TYPE:
        response = execute<message::EnqueueReduceBuffer>(
                static_cast<const message::EnqueueReduceBuffer&>(request), *host);
        break;
    case message::EnqueueUnmapBuffer::TYPE:
        response = execute<message::EnqueueUnmapBuffer>(
                static_cast<const message::EnqueueUnmapBuffer&>(request), *host);
        break;
#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
    case message::EnqueueWaitForEvents::TYPE:
        response = execute<message::EnqueueWaitForEvents>(
                static_cast<const message::EnqueueWaitForEvents&>(request), *host);
        break;
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)
    case message::FinishRequest::TYPE:
        response = execute<message::FinishRequest>(
                static_cast<const message::FinishRequest&>(request), *host);
        break;
    case message::FlushRequest::TYPE:
        response = execute<message::FlushRequest>(
                static_cast<const message::FlushRequest&>(request), *host);
        break;
    case message::CreateProgramWithSource::TYPE:
        response = execute<message::CreateProgramWithSource>(
                static_cast<const message::CreateProgramWithSource&>(request), *host);
        break;
    case message::DeleteProgram::TYPE:
        response = execute<message::DeleteProgram>(
                static_cast<const message::DeleteProgram&>(request), *host);
        break;
    case message::BuildProgram::TYPE:
        response = execute<message::BuildProgram>(
                static_cast<const message::BuildProgram&>(request), *host);
        break;
    case message::CreateKernel::TYPE:
        response = execute<message::CreateKernel>(
                static_cast<const message::CreateKernel&>(request), *host);
        break;
    case message::CreateKernelsInProgram::TYPE:
        response = execute<message::CreateKernelsInProgram>(
                static_cast<const message::CreateKernelsInProgram&>(request), *host);
        break;
    case message::DeleteKernel::TYPE:
        response = execute<message::DeleteKernel>(
                static_cast<const message::DeleteKernel&>(request), *host);
        break;
    case message::CreateEvent::TYPE:
        response = execute<message::CreateEvent>(
                static_cast<const message::CreateEvent&>(request), *host);
        break;
    case message::DeleteEvent::TYPE:
        response = execute<message::DeleteEvent>(
                static_cast<const message::DeleteEvent&>(request), *host);
        break;
    case message::GetKernelInfo::TYPE:
        response = execute<message::GetKernelInfo>(
                static_cast<const message::GetKernelInfo&>(request), *host);
        break;
    case message::GetKernelWorkGroupInfo::TYPE:
        response = execute<message::GetKernelWorkGroupInfo>(
                static_cast<const message::GetKernelWorkGroupInfo&>(request), *host);
        break;
    case message::GetEventProfilingInfos::TYPE:
        response = execute<message::GetEventProfilingInfos>(
                static_cast<const message::GetEventProfilingInfos&>(request), *host);
        break;
    case message::SetKernelArgMemObject::TYPE:
        response = execute<message::SetKernelArgMemObject>(
                static_cast<const message::SetKernelArgMemObject&>(request), *host);
        break;
    case message::SetKernelArgBinary::TYPE:
        response = execute<message::SetKernelArgBinary>(
                static_cast<const message::SetKernelArgBinary&>(request), *host);
        break;

    default: // unknown request
        return false;
    }

    // a response should have been created to answer a request
    assert(response && "No response");
    if (response) {
        host->sendMessage(*response);
    }

    return true;
}

} /* namespace comm */
} /* namespace dclasio */
