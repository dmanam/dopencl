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
 * \file CLEventProcessor.h
 *
 * \date 2014-04-04
 * \author Philipp Kegel
 */

#ifndef CLEVENTPROCESSOR_H_
#define CLEVENTPROCESSOR_H_

#include <dclasio/message/CommandMessage.h>
#include <dclasio/message/EventSynchronizationMessage.h>

#include <dcl/BlockingQueue.h>
#include <dcl/CLObjectRegistry.h>
#include <dcl/DCLTypes.h>
#include <dcl/Process.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <functional>
#include <thread>

namespace dclasio {

class ComputeNodeCommunicationManagerImpl;
class CommunicationManagerImpl;
class HostImpl;
class SmartCLObjectRegistry;

namespace message {

class ContextErrorMessage;
class Message;
class ProgramBuildMessage;

} /* namespace message */

/* ****************************************************************************/

namespace comm {

/*!
 * \brief A processor for incoming application-level events.
 */
class CLEventProcessor {
public:
	virtual ~CLEventProcessor() { }

    virtual bool dispatch(
            const message::Message& message,
            dcl::process_id         pid) = 0;
};

/* ****************************************************************************/

/*!
 * \brief A processor for incoming application-level events from compute nodes.
 *
 * This processor is used on the host! It is called 'CLComputeNodeEventProcessor'
 * as it processes compute node event, i.e., events from compute nodes.
 *
 * This class should become the only implementation of an event processor.
 * Due to implementation issues of ID generation (each host uses its own ID
 * range) it currently has to be distinguished from event procesors used on
 * the compute node side.
 *
 * \see CLHostEventProcessor
 */
class CLComputeNodeEventProcessor: public CLEventProcessor {
public:
    /* A task is a nullary function */
    typedef std::function<void ()> Task;

    CLComputeNodeEventProcessor(
            const CommunicationManagerImpl& communicationManager,
            const dcl::CLObjectRegistry&    objectregistry);
    virtual ~CLComputeNodeEventProcessor();

    void run();

    bool dispatch(
            const message::Message& message,
            dcl::process_id         pid);

private:
    void contextError(
            const message::ContextErrorMessage& notification) const;

    void executionStatusChanged(
            const message::CommandExecutionStatusChangedMessage& notification);

    /*!
     * \brief Callback for an incoming event synchronization request
     *
     * \param[in]  notification the synchronization request
     * \param[in]  process      the process that requested the synchronization
     */
    void synchronizeEvent(
            const message::EventSynchronizationMessage& notification,
            dcl::Process&                               process) const;

    void programBuildComplete(
            const message::ProgramBuildMessage& notification) const;

    const CommunicationManagerImpl& _communicationManager;
    const dcl::CLObjectRegistry& _objectRegistry; //!< Registry for application objects

    dcl::BlockingQueue<Task> _taskList; //!< Task list for worker thread

    std::thread _worker; //!< Worker thread
};

/* ****************************************************************************/

/*!
 * \brief A processor for incoming application-level events from hosts.
 *
 * This processor is used on compute nodes! It is called 'CLHostEventProcessor'
 * as it processes host events, i.e., events from the host.
 *
 * Event messages from hosts have to be processed differently as the IDs within
 * these messages refer to a particular host and, thus, are not unique if
 * multiple hosts are connected to a compute nodes.
 * Hence, a different event processor is required for each host that only
 * processes event messages it received from this associated host.
 *
 * In future version of dOpenCL, UUIDs could be used to assign globally unique
 * IDs to objects, such that an event processor does not have to consider the
 * event source to resolve these IDs. Then, CLHostEventProcessor and
 * CLComputeNodeEventProcessor can be merged into CLEventProcessor.
 */
class CLHostEventProcessor: public CLEventProcessor {
public:
    CLHostEventProcessor(
            const ComputeNodeCommunicationManagerImpl& communicationManager);

    bool dispatch(
            const message::Message& message,
            dcl::process_id         pid);

private:
    SmartCLObjectRegistry& getObjectRegistry(
            HostImpl& host) const;

    void executionStatusChanged(
            const message::CommandExecutionStatusChangedMessage&    notification,
            HostImpl&                                               host) const;

    /*!
     * \brief Callback for an incoming event synchronization request
     *
     * \param[in]  notification the synchronization request
     * \param[in]  host         the host that requested the synchronization
     */
    void synchronizeEvent(
            const message::EventSynchronizationMessage& notification,
            HostImpl&                                   host) const;

    const ComputeNodeCommunicationManagerImpl& _communicationManager;
};

} /* namespace comm */

} /* namespace dclasio */

#endif /* CLEVENTPROCESSOR_H_ */
