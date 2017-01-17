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
 * \file ProgramBuild.h
 *
 * \date 2012-08-04
 * \author Philipp Kegel
 */

#ifndef PROGRAMBUILD_H_
#define PROGRAMBUILD_H_

#include <dcl/ComputeNode.h>
#include <dcl/DCLTypes.h>
#include <dcl/ProgramBuildListener.h>
#include <dcl/Remote.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <algorithm>
#include <condition_variable>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>

namespace dclicd {

namespace detail {

/*!
 * \brief A pending program build operation.
 */
class ProgramBuild:
    public dcl::ProgramBuildListener, // implemented ProgramBuildListener
    public dcl::Remote {              // extends Remote
public:
    /*!
     * \brief A program build operation.
     */
    ProgramBuild(
            cl_program                          program,
            const std::vector<cl_device_id>&    devices,
            const char *                        options,
            void (*                             pfn_notify)(
                    cl_program, void *),
            void *                              user_data);
    virtual ~ProgramBuild();

    template<class ForwardIterator>
    bool includesAnyDeviceOf(ForwardIterator first, ForwardIterator last) const {
        return (std::find_first_of(std::begin(_devices), std::end(_devices),
                first, last) != std::end(_devices));
    }

    /*!
     * \brief Test if this program build is complete.
     *
     * \return \c true if this program build is complete, otherwise \c false
     */
    bool isComplete() const;

    /*!
     * \brief Test if there was a failure to build the program.
     *
     * \return \c true if this program build failed, otherwise \c false
     */
    bool hasFailed() const;

    /*!
     * \brief Awaits completion of this program build.
     */
    void wait();

    /*
     * Program build listener API
     */
    void onComplete(
            const std::vector<dcl::Device *>&   devices,
            const std::vector<cl_build_status>& buildStatus);

private:
    /*!
     * \brief Submits the program build to the target devices' compute nodes
     */
    void submit();

    /*!
     * \brief Test if this program build is complete.
     *
     * Unlike isComplete, this method is not thread-safe and is reserved for
     * internal use.
     *
     * \return \c true if this program build is complete, otherwise \c false
     */
    bool testComplete() const;

    std::vector<dcl::ComputeNode *> _computeNodes; //!< compute nodes executing this program build

    cl_program _program; //!< Program associated with this program build
    std::vector<cl_device_id> _devices; //!< Devices associated with this program build
    std::string _options;
    void (*_pfnNotify)(cl_program, void *);
    void *_userData;

    cl_build_status _buildStatus; //!< Status of this program build (aggregated status of all devices)
    mutable std::mutex _buildStatusMutex;
    mutable std::condition_variable_any _buildCompleted;
};

} /* namespace detail */

} /* namespace dclicd */

#endif /* PROGRAMBUILD_H_ */
