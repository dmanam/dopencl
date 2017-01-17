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

/**
 * @file CommandQueue.cpp
 *
 * @date 2011-05-31
 * @author Karsten Jeschkies
 * @author Philipp Kegel
 */

#include "CommandQueue.h"

#include "Context.h"
#include "Device.h"
#include "Event.h"
#include "Kernel.h"
#include "Memory.h"
#include "Platform.h"
#include "Program.h"
#include "Retainable.h"

#include "dclicd/Buffer.h"
#include "dclicd/Error.h"
#include "dclicd/Event.h"
#include "dclicd/utility.h"

#include "dclicd/command/Command.h"
#include "dclicd/command/ReadWriteCommand.h"
#include "dclicd/command/MappingCommand.h"

#include <dclasio/message/CreateCommandQueue.h>
#include <dclasio/message/DeleteCommandQueue.h>
#include <dclasio/message/EnqueueNDRangeKernel.h>
#include <dclasio/message/EnqueueBarrier.h>
#include <dclasio/message/EnqueueBroadcastBuffer.h>
#include <dclasio/message/EnqueueCopyBuffer.h>
#include <dclasio/message/EnqueueMapBuffer.h>
#include <dclasio/message/EnqueueMarker.h>
#include <dclasio/message/EnqueueReadBuffer.h>
#include <dclasio/message/EnqueueReduceBuffer.h>
#include <dclasio/message/EnqueueUnmapBuffer.h>
#include <dclasio/message/EnqueueWriteBuffer.h>
#include <dclasio/message/EnqueueWaitForEvents.h>
#include <dclasio/message/ErrorResponse.h>
#include <dclasio/message/FinishRequest.h>
#include <dclasio/message/FlushRequest.h>

#include <dcl/CLError.h>
#include <dcl/CLObjectRegistry.h>
#include <dcl/ComputeNode.h>
#include <dcl/DataTransfer.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>
#include <dcl/Remote.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_collective.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_collective.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>


_cl_command_queue::_cl_command_queue(cl_context context, cl_device_id device,
		cl_command_queue_properties properties) :
	_context(context), _device(device), _properties(properties)
{
	if (!context) throw dclicd::Error(CL_INVALID_CONTEXT);
	if (!device) throw dclicd::Error(CL_INVALID_DEVICE);
	if (!context->hasDevice(device)) throw dclicd::Error(CL_INVALID_DEVICE);

	try {
		dclasio::message::CreateCommandQueue request(_context->remoteId(),
				_device->remote().getId(), _id, properties);
		_device->remote().getComputeNode().executeCommand(request);
		dcl::util::Logger << dcl::util::Info
				<< "Command queue created (ID=" << _id << ')' << std::endl;

        /* Register command queue as command queue listener listener */
		_context->getPlatform()->remote().objectRegistry().bind<dcl::CommandQueueListener>(_id, *this);
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}

	_context->retain();
}

_cl_command_queue::~_cl_command_queue() {
    dclicd::release(_context);
}

void _cl_command_queue::destroy() {
	assert(_ref_count == 0);

	/* A command queue must only be deleted if their reference count is 0 *and*
	 * all commands enqueued to it have finished.
	 * 
	 * Note that the command queue on the host does not have to be retained
	 * for commands that have only been enqueued remotely as the remote command
	 * queue is implicitly retained by the compute node's OpenCL implementation. */
	finishLocally();
	
	try {
		dclasio::message::DeleteCommandQueue request(_id);
		_device->remote().getComputeNode().executeCommand(request);

        /* Remove this command queue from list of command queue listeners */
        _context->getPlatform()->remote().objectRegistry().unbind<dcl::CommandQueueListener>(_id);

		dcl::util::Logger << dcl::util::Info
				<< "Command queue deleted (ID=" << _id << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

dcl::ComputeNode& _cl_command_queue::computeNode() const {
	return _device->remote().getComputeNode();
}

void _cl_command_queue::getInfo(
		cl_command_queue_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) const {
	switch (param_name) {
	case CL_QUEUE_CONTEXT:
		dclicd::copy_info(_context, param_value_size,
				param_value, param_value_size_ret);
		break;
	case CL_QUEUE_DEVICE:
		dclicd::copy_info(_device, param_value_size,
				param_value, param_value_size_ret);
		break;
	case CL_QUEUE_REFERENCE_COUNT:
		dclicd::copy_info(_ref_count, param_value_size,
				param_value, param_value_size_ret);
		break;
	case CL_QUEUE_PROPERTIES:
		dclicd::copy_info(_properties, param_value_size,
				param_value, param_value_size_ret);
		break;
	default:
		throw dclicd::Error(CL_INVALID_VALUE);
	}
}

void _cl_command_queue::finish() {
	try {
		dclasio::message::FinishRequest request(_id);

		/* TODO Make _cl_command_queue::finish a non-blocking operation.
		 * Finishing may block for a long time on the compute node. Hence, the
		 * client driver must not report an IO exception, if no response is
		 * received from the compute node immediately. */
//        _device->remote().getComputeNode().executeCommand(request, dclasio::message::Request::BLOCKING);
		_device->remote().getComputeNode().executeCommand(request);
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}

	/* TODO Make compute node call _cl_command_queue::onFinish */
	onFinish();

    dcl::util::Logger << dcl::util::Info
            << "Finished command queue (ID=" << _id << ')' << std::endl;
}

void _cl_command_queue::onFinish() {
    finishLocally();
}

void _cl_command_queue::flush() {
	try {
		dclasio::message::FlushRequest request(_id);
		_device->remote().getComputeNode().executeCommand(request);
		dcl::util::Logger << dcl::util::Info
				<< "Flushed command queue (ID=" << _id << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

void _cl_command_queue::createEventIdWaitList(
		const std::vector<cl_event>& event_wait_list,
		std::vector<dcl::object_id>& eventIds) {
	eventIds.resize(event_wait_list.size());
	for (unsigned int i = 0; i < event_wait_list.size(); ++i) {
		cl_event event = event_wait_list[i];
		cl_context context;

		if (!event) throw dclicd::Error(CL_INVALID_EVENT_WAIT_LIST);
		/* Command queue and event must be associated with the same context */
		event->getInfo(CL_EVENT_CONTEXT, sizeof(context), &context, nullptr);
		if (context != _context) throw dclicd::Error(CL_INVALID_CONTEXT);
		eventIds[i] = event->remoteId();
	}
}

void _cl_command_queue::enqueueCommand(
		const std::shared_ptr<dclicd::command::Command>& command) {
	std::lock_guard<std::mutex> lock(_commandsMutex);
	/* Remove completed commands from list */
	_commands.erase(
			std::remove_if(std::begin(_commands), std::end(_commands),
					std::bind(&dclicd::command::Command::isComplete, std::placeholders::_1)),
			std::end(_commands));
	/* Add command to list */
	_commands.push_back(command);
}

void _cl_command_queue::finishLocally() {
    std::vector<std::shared_ptr<dclicd::command::Command>> commands;

    /* Clean up command queue */
    {
        std::lock_guard<std::mutex> lock(_commandsMutex);
        /* clear list of enqueued command */
        commands.swap(_commands);
    }

    /* Wait until all pending commands have finished */
    dcl::util::Logger << dcl::util::Debug
            << "Waiting for " << commands.size() << " commands in queue (ID=" << _id << ')'
            << std::endl;
    for (auto command : commands) {
        command->wait();
    }
}

#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
void _cl_command_queue::enqueueWaitForEvents(
		const std::vector<cl_event>& eventList) {
	std::vector<dcl::object_id> eventIds;

	if (eventList.empty()) throw dclicd::Error(CL_INVALID_VALUE);

	/*
	 * Convert event list
	 * Unlike an event wait list, the event list of this method must not contain
	 * user events. Moreover, it throws CL_INVALID_EVENT rather than
	 * CL_INVALID_EVENT_WAIT_LIST if the event list contains an invalid event.
	 */
	eventIds.resize(eventList.size());
	for (unsigned int i = 0; i < eventList.size(); ++i) {
		cl_event event = eventList[i];
		cl_context context;

		if (!event) throw dclicd::Error(CL_INVALID_EVENT);
		/* Command queue and event must be associated with the same context */
		event->getInfo(CL_EVENT_CONTEXT, sizeof(context), &context, nullptr);
		if (context != _context) throw dclicd::Error(CL_INVALID_CONTEXT);
		eventIds[i] = event->remoteId();
	}

	/*
	 * Enqueue wait for events command on command queue's compute node
	 */
	try {
		dclasio::message::EnqueueWaitForEvents request(_id, eventIds);
		_device->remote().getComputeNode().executeCommand(request);
		dcl::util::Logger << dcl::util::Info
				<< "Enqueued wait for events (command queue ID=" << _id << ')'
				<< std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

void _cl_command_queue::enqueueMarker(
		const std::vector<cl_event>& event_wait_list, cl_event *event) {
    std::vector<dcl::object_id> eventIds;

    /* Convert event wait list */
    createEventIdWaitList(event_wait_list, eventIds);

    /* Create event */
    if (event) {
        std::shared_ptr<dclicd::command::Command> marker(
                std::make_shared<dclicd::command::Command>(CL_COMMAND_MARKER, this));
        enqueueCommand(marker);
        *event = new dclicd::Event(_context, marker);
//        *event = new dclicd::Event(_context, this, CL_COMMAND_MARKER);
    }

    /*
     * Enqueue marker on command queue's compute node
     */
    try {
        dclasio::message::EnqueueMarker request(_id, (event ? (*event)->remoteId() : 0),
                &eventIds, (event != nullptr));
        _device->remote().getComputeNode().executeCommand(request);
        dcl::util::Logger << dcl::util::Info
                << "Enqueued marker (command queue ID=" << _id
                << ", command ID=" << (event ? (*event)->remoteId() : 0)
                << ')' << std::endl;
    } catch (const dcl::CLError& err) {
        throw dclicd::Error(err);
    } catch (const dcl::IOException& err) {
        throw dclicd::Error(err);
    } catch (const dcl::ProtocolException& err) {
        throw dclicd::Error(err);
    }
}

void _cl_command_queue::enqueueBarrier(
		const std::vector<cl_event>& event_wait_list, cl_event *event) {
    std::vector<dcl::object_id> eventIds;

    /* Convert event wait list */
    createEventIdWaitList(event_wait_list, eventIds);

    /* Create event */
    if (event) {
        std::shared_ptr<dclicd::command::Command> barrier(
                std::make_shared<dclicd::command::Command>(CL_COMMAND_BARRIER, this));
        enqueueCommand(barrier);
        *event = new dclicd::Event(_context, barrier);
//        *event = new dclicd::Event(_context, this, CL_COMMAND_BARRIER);
    }

	try {
        dclasio::message::EnqueueBarrier request(_id, (event ? (*event)->remoteId() : 0),
                &eventIds, (event != nullptr));
        _device->remote().getComputeNode().executeCommand(request);
        dcl::util::Logger << dcl::util::Info
                << "Enqueued barrier (command queue ID=" << _id
                << ", command ID=" << (event ? (*event)->remoteId() : 0)
                << ')' << std::endl;
    } catch (const dcl::CLError& err) {
        throw dclicd::Error(err);
    } catch (const dcl::IOException& err) {
        throw dclicd::Error(err);
    } catch (const dcl::ProtocolException& err) {
        throw dclicd::Error(err);
    }
}

void _cl_command_queue::enqueueRead(
		dclicd::Buffer *buffer,
		cl_bool blocking_read,
		size_t offset,
		size_t cb,
		void *ptr,
		const std::vector<cl_event>& event_wait_list,
		cl_event *event) {
	std::shared_ptr<dclicd::command::Command> readBuffer;
	std::vector<dcl::object_id> eventIds;

    if (!buffer) throw dclicd::Error(CL_INVALID_MEM_OBJECT);
	// Command queue and buffer must be associated with the same context
	if (buffer->context() != _context) throw dclicd::Error(CL_INVALID_CONTEXT);

	// Convert event wait list
	createEventIdWaitList(event_wait_list, eventIds);

	// Enqueue read buffer command locally
	readBuffer = std::make_shared<dclicd::command::ReadMemoryCommand>(
            CL_COMMAND_READ_BUFFER, this, cb, ptr);
	enqueueCommand(readBuffer);

	// Create event
	if (event) {
		*event = new dclicd::Event(_context, readBuffer);
	}

	// Enqueue read buffer command on command queue's compute node
	try {
		dclasio::message::EnqueueReadBuffer request(_id, readBuffer->remoteId(),
				buffer->remoteId(), blocking_read, offset, cb, &eventIds,
				(event != nullptr));
		_device->remote().getComputeNode().executeCommand(request);
		dcl::util::Logger << dcl::util::Info
				<< "Enqueued data download from buffer (command queue ID="
				<< _id << ", buffer ID=" << buffer->remoteId()
				<< ", size=" << cb
				<< ", command ID=" << readBuffer->remoteId()
				<< ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}

	if (blocking_read) {
		/* Wait for completion of command
		 * This blocking operation performs an implicit flush */
		readBuffer->wait();
	}
}

void _cl_command_queue::enqueueWrite(
		dclicd::Buffer *buffer,
		cl_bool blocking_write,
		size_t offset,
		size_t cb,
		const void *ptr,
		const std::vector<cl_event>& event_wait_list,
		cl_event *event) {
	std::shared_ptr<dclicd::command::Command> writeBuffer;
	std::vector<dcl::object_id> eventIds;

    if (!buffer) throw dclicd::Error(CL_INVALID_MEM_OBJECT);
	// Command queue and buffer must be associated with the same context
	if (buffer->context() != _context) throw dclicd::Error(CL_INVALID_CONTEXT);

	// Convert event wait list
	createEventIdWaitList(event_wait_list, eventIds);

	// Enqueue write buffer command locally
	writeBuffer = std::make_shared<dclicd::command::WriteMemoryCommand>(
            CL_COMMAND_WRITE_BUFFER, this, cb, ptr);
	enqueueCommand(writeBuffer);

	// Create event
	if (event) {
		*event = new dclicd::Event(_context, writeBuffer, std::vector<cl_mem>(1, buffer));
	}

	// Enqueue write buffer command on command queue's compute node
	try {
		dclasio::message::EnqueueWriteBuffer enqueueWriteBuffer(_id,
				writeBuffer->remoteId(), buffer->remoteId(), blocking_write,
				offset, cb, &eventIds, (event != nullptr));
		_device->remote().getComputeNode().executeCommand(enqueueWriteBuffer);
		dcl::util::Logger << dcl::util::Info
				<< "Enqueued data upload to buffer (command queue ID=" << _id
				<< ", buffer ID=" << buffer->remoteId()
				<< ", size=" << cb
                << ", command ID=" << writeBuffer->remoteId()
				<< ')' << std::endl;
	} catch (const dcl::CLError& err) {
		/* TODO Delete user events on other compute nodes */
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}

	if (blocking_write) {
		/* Wait for completion of command
		 * This blocking operation performs an implicit flush */
		writeBuffer->wait();
	}
}

void _cl_command_queue::enqueueCopy(
		dclicd::Buffer *src,
		dclicd::Buffer *dst,
		size_t src_offset,
		size_t dst_offset,
		size_t cb,
		const std::vector<cl_event>& event_wait_list,
		cl_event *event) {
	std::vector<dcl::object_id> eventIds;

    if (!src) throw dclicd::Error(CL_INVALID_MEM_OBJECT);
    if (!dst) throw dclicd::Error(CL_INVALID_MEM_OBJECT);
	// Command queue and buffer must be associated with the same context
	if (src->context() != _context || dst->context() != _context) {
		throw dclicd::Error(CL_INVALID_CONTEXT);
	}

	// Convert event wait list
	createEventIdWaitList(event_wait_list, eventIds);

	// Create event
	if (event) {
        std::shared_ptr<dclicd::command::Command> copyBuffer(
                std::make_shared<dclicd::command::Command>(CL_COMMAND_COPY_BUFFER, this));
        enqueueCommand(copyBuffer);
		*event = new dclicd::Event(_context, copyBuffer, std::vector<cl_mem>(1, dst));
//        *event = new dclicd::Event(_context, this, CL_COMMAND_COPY_BUFFER);
	}

	try {
		dclasio::message::EnqueueCopyBuffer request(_id, (event ? (*event)->remoteId() : 0),
				src->remoteId(), dst->remoteId(), src_offset, dst_offset, cb,
				&eventIds, (event != nullptr));
		_device->remote().getComputeNode().executeCommand(request);
		dcl::util::Logger << dcl::util::Info
				<< "Enqueued copy buffer (command queue ID=" << _id
				<< ", src buffer ID=" << src->remoteId()
				<< ", dst buffer ID=" << dst->remoteId()
                << ", command ID=" << (event ? (*event)->remoteId() : 0)
				<< ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

void * _cl_command_queue::enqueueMap(
		dclicd::Buffer *buffer,
		cl_bool blocking_map,
		cl_map_flags map_flags,
		size_t offset,
		size_t cb,
		const std::vector<cl_event>& event_wait_list,
		cl_event *event) {
	std::shared_ptr<dclicd::command::Command> mapBuffer;
	std::vector<dcl::object_id> eventIds;
	void *ptr;

	if (!buffer) throw dclicd::Error(CL_INVALID_MEM_OBJECT);
    // Command queue and buffer must be associated with the same context
    if (buffer->context() != _context) throw dclicd::Error(CL_INVALID_CONTEXT);

	/* Create pointer to mapped region of buffer
	 * This operation allocates memory for mapping (if required) and updates the
	 * buffer's map count, but does not copy its data. */
	ptr = buffer->map(map_flags, offset, cb);
	// FIXME Unmap memory in case of an error

    // Convert event wait list
    createEventIdWaitList(event_wait_list, eventIds);

	// Enqueue map buffer command locally
	mapBuffer = std::make_shared<dclicd::command::MapBufferCommand>(
            this, buffer, map_flags, cb, ptr);
	enqueueCommand(mapBuffer);

	// Create event
	if (event) {
		*event = new dclicd::Event(_context, mapBuffer);
	}

	try {
        dclasio::message::EnqueueMapBuffer request(_id, mapBuffer->remoteId(),
                buffer->remoteId(), blocking_map, map_flags,
                offset, cb,
                &eventIds, (event != nullptr));
        _device->remote().getComputeNode().executeCommand(request);
        dcl::util::Logger << dcl::util::Info
                << "Enqueued map buffer (command queue ID=" << _id
                << ", buffer ID=" << buffer->remoteId()
                << ", command ID=" << mapBuffer->remoteId()
                << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}

	if (blocking_map) {
		/* Wait for completion of command
		 * This blocking operation performs an implicit flush */
		mapBuffer->wait();
	}

	return ptr;
}

void _cl_command_queue::enqueueUnmap(
		cl_mem memobj,
		void *mapped_ptr,
		const std::vector<cl_event>& event_wait_list,
		cl_event *event) {
	std::shared_ptr<dclicd::command::Command> unmapMemory;
	std::vector<dcl::object_id> eventIds;
    cl_mem_object_type type;

    memobj->getInfo(CL_MEM_TYPE, sizeof(type), &type, nullptr);

    if (!memobj) throw dclicd::Error(CL_INVALID_MEM_OBJECT);
    // Command queue and memory object must be associated with the same context
    if (memobj->context() != _context) throw dclicd::Error(CL_INVALID_CONTEXT);

	// Obtain mapping data
    /* TODO Generalize _cl_command_queue::enqueueUnmap for memory objects
    auto mapping = memobj->findMapping(mapped_ptr);
     */
    auto buffer = static_cast<dclicd::Buffer *>(memobj);
    assert(buffer && "Memory object is no buffer");
    auto mapping = buffer->findMapping(mapped_ptr);
    if (!mapping) {
        /* mapped_ptr is not a valid pointer returned by clEnqueueMapBuffer or
         * clEnqueueMapImage for this memory object */
        throw dclicd::Error(CL_INVALID_VALUE);
    }

	// Convert event wait list
	createEventIdWaitList(event_wait_list, eventIds);

	// Enqueue unmap memory object command locally
	unmapMemory = std::make_shared<dclicd::command::UnmapBufferCommand>(
	        this, buffer, mapping->flags(), mapping->cb(), mapped_ptr);
	enqueueCommand(unmapMemory);

	// Create event
	if (event) {
        if (mapping->flags() & CL_MAP_WRITE) {
            /* The memory object had been mapped for writing. Thus, the unmap
             * operation modifies the memory object which is therefore
             * associated with the unmap event. */
            *event = new dclicd::Event(_context, unmapMemory, std::vector<cl_mem>(1, memobj));
        } else {
            *event = new dclicd::Event(_context, unmapMemory);
        }
	}

	try {
        switch (type) {
        case CL_MEM_OBJECT_BUFFER:
        {
            dclasio::message::EnqueueUnmapBuffer request(_id, unmapMemory->remoteId(),
                    memobj->remoteId(), mapping->flags(),
                    mapping->offset(), mapping->cb(),
                    &eventIds, (event != nullptr));
            _device->remote().getComputeNode().executeCommand(request);
            break;
        }
        case CL_MEM_OBJECT_IMAGE2D:
        case CL_MEM_OBJECT_IMAGE3D:
            assert(!"clUnmapMemObject not implemented for image");
            break;
        default:
            throw dclicd::Error(CL_INVALID_MEM_OBJECT);
            // no break
        }

        dcl::util::Logger << dcl::util::Info
                << "Enqueued unmapping memory object (command queue ID=" << _id
                << ", memory object ID=" << memobj->remoteId()
                << ", command ID=" << unmapMemory->remoteId()
                << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

#if defined(CL_VERSION_1_2)
void _cl_command_queue::enqueueMigrateMemObjects(
        const std::vector<cl_mem>& mem_objects,
        cl_mem_migration_flags flags,
        const std::vector<cl_event>& event_wait_list,
        cl_event *event) {
    assert(!"_cl_command_queue::enqueueMigrateMemObjects not implemented");
}
#endif // #if defined(CL_VERSION_1_2)

void _cl_command_queue::enqueueNDRangeKernel(
		cl_kernel kernel,
		const std::vector<size_t>& offset,
		const std::vector<size_t>& global,
		const std::vector<size_t>& local,
		const std::vector<cl_event>& event_wait_list,
		cl_event *event) {
	std::vector<dcl::object_id> eventIds;

	if (!kernel) throw dclicd::Error(CL_INVALID_KERNEL);
	/* Command queue and kernel must be associated with the same context */
	if (kernel->program()->context() != _context) throw dclicd::Error(CL_INVALID_CONTEXT);

	/* Convert event wait list */
	createEventIdWaitList(event_wait_list, eventIds);

	if (event) {
        std::shared_ptr<dclicd::command::Command> nDRangeKernel(
                std::make_shared<dclicd::command::Command>(CL_COMMAND_NDRANGE_KERNEL, this));
        enqueueCommand(nDRangeKernel);
		*event = new dclicd::Event(_context, nDRangeKernel, kernel->writeMemoryObjects());
//        *event = new dclicd::Event(_context, this, CL_COMMAND_NDRANGE_KERNEL);
	}

	/*
	 * Enqueue kernel (remote operation)
	 */
	try {
		dclasio::message::EnqueueNDRangeKernel request(
				_id, (event ? (*event)->remoteId() : 0), kernel->remoteId(),
				offset, global, local, &eventIds, (event != nullptr));
		_device->remote().getComputeNode().executeCommand(request);
		dcl::util::Logger << dcl::util::Info
				<< "Enqueued ND range kernel (command queue ID=" << _id
				<< ", kernel ID=" << kernel->remoteId()
                << ", command ID=" << (event ? (*event)->remoteId() : 0)
				<< ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

void _cl_command_queue::enqueueTask(
        cl_kernel kernel,
        const std::vector<cl_event>& event_wait_list,
        cl_event *event) {
    std::vector<dcl::object_id> eventIds;

    if (!kernel) throw dclicd::Error(CL_INVALID_KERNEL);
    /* Command queue and kernel must be associated with the same context */
    if (kernel->program()->context() != _context) throw dclicd::Error(CL_INVALID_CONTEXT);

    /* Convert event wait list */
    createEventIdWaitList(event_wait_list, eventIds);

    if (event) {
        std::shared_ptr<dclicd::command::Command> task(
                std::make_shared<dclicd::command::Command>(CL_COMMAND_TASK, this));
        enqueueCommand(task);
        *event = new dclicd::Event(_context, task, kernel->writeMemoryObjects());
//        *event = new dclicd::Event(_context, this, CL_COMMAND_TASK);
    }

    /*
     * Enqueue task (remote operation)
     * clEnqueueTask is equivalent to calling clEnqueueNDRangeKernel with
     * work_dim = 1, global_work_offset = NULL, global_work_size[0] set to 1,
     * and local_work_size[0] set to 1.
     */
    try {
        dclasio::message::EnqueueNDRangeKernel request(
                _id, (event ? (*event)->remoteId() : 0), kernel->remoteId(),
                std::vector<size_t>(), std::vector<size_t>(1, 1), std::vector<size_t>(1, 1),
                &eventIds, (event != nullptr));
        _device->remote().getComputeNode().executeCommand(request);
        dcl::util::Logger << dcl::util::Info
                << "Enqueued task (command queue ID=" << _id
                << ", kernel ID=" << kernel->remoteId()
                << ", command ID=" << (event ? (*event)->remoteId() : 0)
                << ')' << std::endl;
    } catch (const dcl::CLError& err) {
        throw dclicd::Error(err);
    } catch (const dcl::IOException& err) {
        throw dclicd::Error(err);
    } catch (const dcl::ProtocolException& err) {
        throw dclicd::Error(err);
    }
}

void _cl_command_queue::enqueueBroadcast(
		std::vector<cl_command_queue> commandQueueList,
		dclicd::Buffer *src,
		std::vector<dclicd::Buffer *> dsts,
		size_t srcOffset,
		const std::vector<size_t> dstOffsets,
		size_t cb,
		const std::vector<cl_event>& event_wait_list,
		cl_event *event) {
	cl_context context;
	std::map<dcl::ComputeNode *, std::vector<dcl::object_id>> nodeCommandQueueIds;
	std::map<dcl::ComputeNode *, std::set<dcl::object_id>> nodeDstIds;
    std::map<dcl::ComputeNode *, std::vector<size_t>> nodeDstOffsets;
	std::vector<dcl::object_id> eventIds;

	if (!src) throw dclicd::Error(CL_INVALID_MEM_OBJECT);
	if (commandQueueList.empty() || dsts.empty()) {
		throw dclicd::Error(CL_INVALID_VALUE);
	}

	/* Command queues and buffers must be associated with the same context */
	context = src->context();

	/*
	 * Validate command queues and source buffers
	 * Create ID lists
	 */
    assert(commandQueueList.size() == dsts.size());
	std::vector<cl_command_queue>::const_iterator i = std::begin(commandQueueList);
	std::vector<dclicd::Buffer *>::const_iterator j = std::begin(dsts);
    std::vector<size_t>::const_iterator k = std::begin(dstOffsets);
	for (; i != std::end(commandQueueList) && j != std::end(dsts) && k != std::end(dstOffsets);
	        ++i, ++j, ++k) {
		cl_command_queue queue = *i;
		cl_mem dst = *j;
		size_t offset = *k;
		dcl::ComputeNode *computeNode;
		bool inserted;

		if (!queue) throw dclicd::Error(CL_INVALID_COMMAND_QUEUE);
		if (queue->_context != context) throw dclicd::Error(CL_INVALID_CONTEXT);
		if (!dst) throw dclicd::Error(CL_INVALID_MEM_OBJECT);
		if (dst->context() != context) throw dclicd::Error(CL_INVALID_CONTEXT);

		/* TODO Offset of destination buffer must specify valid buffer region */

		computeNode = &queue->computeNode();

		nodeCommandQueueIds[computeNode].push_back(queue->remoteId());
		inserted = nodeDstIds[computeNode].insert(dst->remoteId()).second;
		if (!inserted) {
		    /* destination buffer specified more than once */
		    throw dclicd::Error(CL_INVALID_VALUE);
		}
		nodeDstOffsets[computeNode].push_back(offset);
	}

	/* Convert event wait list */
	commandQueueList[0]->createEventIdWaitList(event_wait_list, eventIds);

//	if (event) {
//	    try {
//	        /* TODO Create broadcast command */
//	        *event = new dclicd::Event(context, broadcast);
//	    } catch (const std::bad_alloc&) {
//	        throw dclicd::Error(CL_OUT_OF_RESOURCES);
//	    }
//	}

	/*
	 * Enqueue broadcast (remote operation)
	 */
	try {
	    auto i = std::begin(nodeCommandQueueIds);
        auto j = std::begin(nodeDstIds);
        auto k = std::begin(nodeDstOffsets);
        std::vector<std::pair<dcl::ComputeNode *, dclasio::message::EnqueueBroadcastBuffer>> requests;

        /*
         * Create and send requests
         */
        for (; i != std::end(nodeCommandQueueIds) && j != std::end(nodeDstIds); ++i, ++j) {
            auto computeNode = i->first;
            /* TODO Avoid copying 'enqueue broadcast buffer' requests */
            dclasio::message::EnqueueBroadcastBuffer request(
                    i->second, (event ? (*event)->remoteId() : 0),
                    src->remoteId(),
                    std::vector<dcl::object_id>(std::begin(j->second), std::end(j->second)),
                    srcOffset, k->second, cb,
                    &eventIds, (event != nullptr));
            computeNode->sendRequest(request);

            requests.push_back(std::make_pair(computeNode, request)); // save pending request
        }

        /*
         * Await responses from all compute nodes
         */
        for (auto request : requests) {
            request.first->awaitResponse(request.second);
            /* TODO Receive responses from *all* compute nodes, i.e. do not stop receipt on first failure */
        }

		dcl::util::Logger << dcl::util::Info
				<< "Enqueued broadcast buffer (src buffer ID=" << src->remoteId()
                << ", command ID=" << (event ? (*event)->remoteId() : 0)
				<< ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

void _cl_command_queue::enqueueReduce(
		std::vector<dclicd::Buffer *> srcs,
		dclicd::Buffer *dst,
//			const std::vector<size_t> srcOffsets,
//			size_t dstOffset,
//			size_t cb,
		cl_kernel kernel,
		const std::vector<size_t>& offset,
		const std::vector<size_t>& global,
		const std::vector<size_t>& local,
//			cl_reduce_operator_WWU oper,
		const std::vector<cl_event>& event_wait_list,
		cl_event *event) {
	cl_context context;
	std::vector<dcl::object_id> srcIds;
	std::vector<dcl::object_id> eventIds;

    if (!dst) throw dclicd::Error(CL_INVALID_MEM_OBJECT);

	/* Command queues, buffers, and kernel must be associated with the same context */
	context = dst->context();

	/*
	 * Validate source buffers
	 * Create ID lists
	 */
	for (auto src : srcs) {
		if (!src) throw dclicd::Error(CL_INVALID_MEM_OBJECT);
		if (src->context() != context) throw dclicd::Error(CL_INVALID_CONTEXT);

		srcIds.push_back(src->remoteId());
	}

	/*
	 * Validate kernel
	 */
	if (kernel->program()->context() != context) throw dclicd::Error(CL_INVALID_CONTEXT);

	/* Convert event wait list */
    createEventIdWaitList(event_wait_list, eventIds);

//    if (event) {
//        try {
//            /* TODO Create reduce command */
//            *event = new dclicd::Event(context, *reduce);
//        } catch (const std::bad_alloc&) {
//            throw dclicd::Error(CL_OUT_OF_RESOURCES);
//        }
//    }

    /*
	 * Enqueue reduction (remote operation)
	 */
	try {
		dclasio::message::EnqueueReduceBuffer request(
				_id, (event ? (*event)->remoteId() : 0),
				srcIds, dst->remoteId(),
				kernel->remoteId(), offset, global, local,
				&eventIds, (event != nullptr));
		executeCommand(context->computeNodes(), request);
		dcl::util::Logger << dcl::util::Info
				<< "Enqueued reduce buffer (dst buffer ID=" << dst->remoteId()
                << ", command ID=" << (event ? (*event)->remoteId() : 0)
				<< ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}
