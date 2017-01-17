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
 * \file Session.cpp
 *
 * \date 2013-09-29
 * \author Philipp Kegel
 */

#include "Session.h"

#include "CommandQueue.h"
#include "Context.h"
#include "Device.h"
#include "Event.h"
#include "Kernel.h"
#include "Memory.h"
#include "Program.h"

#include <dcl/CommandQueue.h>
#include <dcl/ComputeNode.h>
#include <dcl/Context.h>
#include <dcl/ContextListener.h>
#include <dcl/DCLTypes.h>
#include <dcl/Event.h>
#include <dcl/Host.h>
#include <dcl/Kernel.h>
#include <dcl/Memory.h>
#include <dcl/Program.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <memory>
#include <set>
#include <vector>

namespace dcld {

Session::Session(const cl::Platform& platform) :
        _platform(platform) {
}

Session::~Session() {
}

/* ****************************************************************************
 * Session APIs
 ******************************************************************************/

std::shared_ptr<dcl::Context> Session::createContext(
		dcl::Host& host,
        const std::vector<dcl::ComputeNode *>& computeNodes,
        const std::vector<dcl::Device *>& devices,
        const std::shared_ptr<dcl::ContextListener>& listener) {
    auto context = std::make_shared<Context>(
            std::ref(host), computeNodes, _platform, devices, listener);
    _contexts.insert(context);

    return context;
}

void Session::releaseContext(
        const std::shared_ptr<dcl::Context>& context) {
    if (_contexts.erase(context) != 1) {
        throw cl::Error(CL_INVALID_CONTEXT);
    }
}

std::shared_ptr<dcl::CommandQueue> Session::createCommandQueue(
        const std::shared_ptr<dcl::Context>& context,
        dcl::Device *device,
        cl_command_queue_properties properties) {
    auto commandQueue = std::make_shared<CommandQueue>(
            std::dynamic_pointer_cast<Context>(context),
            dynamic_cast<Device *>(device),
            properties);
    _commandQueues.insert(commandQueue);

	return commandQueue;
}

void Session::releaseCommandQueue(
		const std::shared_ptr<dcl::CommandQueue>& commandQueue) {
    if (_commandQueues.erase(commandQueue) != 1) {
        throw cl::Error(CL_INVALID_COMMAND_QUEUE);
    }
}

std::shared_ptr<dcl::Buffer> Session::createBuffer(
        const std::shared_ptr<dcl::Context>& context,
        cl_mem_flags flags,
        size_t size,
        void *ptr) {
    auto buffer = std::make_shared<Buffer>(
            std::dynamic_pointer_cast<Context>(context), flags, size, ptr);
    _memoryObjects.insert(buffer);

	return buffer;
}

void Session::releaseMemObject(
        const std::shared_ptr<dcl::Memory>& memory) {
    if (_memoryObjects.erase(memory) != 1) {
        throw cl::Error(CL_INVALID_MEM_OBJECT);
    }
}

std::shared_ptr<dcl::Program> Session::createProgram(
        const std::shared_ptr<dcl::Context>& context,
        const char *source,
		size_t length) {
    auto program = std::make_shared<Program>(
            std::dynamic_pointer_cast<Context>(context), source, length);
	_programs.insert(program);

	return program;
}

std::shared_ptr<dcl::Program> Session::createProgram(
        const std::shared_ptr<dcl::Context>& context,
        const std::vector<dcl::Device *>& devices,
		const std::vector<size_t>& lengths,
		const unsigned char **binaries,
		std::vector<cl_int> *binary_status) {
    auto program = std::make_shared<Program>(
            std::dynamic_pointer_cast<Context>(context), devices, lengths, binaries, binary_status);
    _programs.insert(program);

	return program;
}

void Session::releaseProgram(
        const std::shared_ptr<dcl::Program>& program) {
    if (_programs.erase(program) != 1) {
        throw cl::Error(CL_INVALID_PROGRAM);
    }
}

std::shared_ptr<dcl::Kernel> Session::createKernel(
        const std::shared_ptr<dcl::Program>& program,
        const char *name) {
    auto kernel = std::make_shared<Kernel>(
            std::dynamic_pointer_cast<Program>(program), name);
    _kernels.insert(kernel);

	return kernel;
}

std::vector<std::shared_ptr<dcl::Kernel>> Session::createKernelsInProgram(
        const std::shared_ptr<dcl::Program>& program,
        cl_uint numKernels) {
    if (!program) throw cl::Error(CL_INVALID_PROGRAM);

    std::vector<std::shared_ptr<dcl::Kernel>> kernels;

	program->createKernels(kernels);
	if (numKernels != kernels.size()) throw cl::Error(CL_INVALID_VALUE);
	_kernels.insert(std::begin(kernels), std::end(kernels));

	return kernels;
}

void Session::releaseKernel(
        const std::shared_ptr<dcl::Kernel>& kernel) {
    if (_kernels.erase(kernel) != 1) {
        throw cl::Error(CL_INVALID_KERNEL);
    }
}

void Session::addEvent(
        const std::shared_ptr<dcl::Event>& event) {
    _events.insert(event);
}

std::shared_ptr<dcl::Event> Session::createEvent(
        dcl::object_id id,
        const std::shared_ptr<dcl::Context>& context,
        const std::vector<std::shared_ptr<dcl::Memory>>& memoryObjects) {
    std::vector<std::shared_ptr<Memory>> memoryObjectImpls;

    /* Verify memory objects */
    memoryObjectImpls.reserve(memoryObjects.size());
    for (auto memoryObject : memoryObjects) {
        memoryObjectImpls.push_back(std::dynamic_pointer_cast<Memory>(memoryObject));
    }

    /* Create event
     * Technically, events can only be created by enqueuing a command. However,
     * since the command has been enqueued on another compute node, a user
     * event is created as a substitute for that event. The status of the
     * substitute event is controlled by messages about execution status changes
     * of its associated command. */
    auto event = std::make_shared<RemoteEvent>(
            id, std::dynamic_pointer_cast<Context>(context), memoryObjectImpls);
    /* Add event to list */
    _events.insert(event);

    return event;
}

void Session::releaseEvent(
        const std::shared_ptr<dcl::Event>& event) {
    if (_events.erase(event) != 1) {
        throw cl::Error(CL_INVALID_EVENT);
    }
}

} /* namespace dcld */
