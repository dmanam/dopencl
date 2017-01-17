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
 * @file Program.h
 *
 * @date 2012-06-07
 * @author Karsten Jeschkies
 * @author Philipp Kegel
 */

#ifndef CL_PROGRAM_H_
#define CL_PROGRAM_H_

#include "Retainable.h"

#include "dclicd/detail/ProgramBuild.h"
#include "dclicd/detail/ProgramBuildInfo.h"

#include <dcl/ComputeNode.h>
#include <dcl/Device.h>
#include <dcl/Remote.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>


class _cl_program:
	public _cl_retainable,
	public dcl::Remote {
public:
	typedef std::vector<std::pair<const unsigned char *, size_t>> Binaries;
	typedef std::vector<std::pair<const char *, size_t>> Sources;
    typedef std::map<const char *, cl_program> Headers;


	_cl_program(
			cl_context      context,
			const Sources&  sources);
	_cl_program(
			cl_context                          context,
			const std::vector<cl_device_id>&    devices,
			const Binaries&                     binaries,
			std::vector<cl_int> *               binaryStatus);
	virtual ~_cl_program();

	/**
	 * @brief Obtain a list of all compute nodes belonging to devices the program is built for.
	 *
	 * @return a list of compute nodes
	 */
	const std::vector<dcl::ComputeNode *>& computeNodes() const;

	/**
	 * @brief Builds (compiles and links) a program executable from the program source or binary.
	 *
	 * No changes to the program executable are allowed while there are kernels
	 * associated with a program object. Therefore, calls to this method must
	 * return CL_INVALID_OPERATION if there are kernels attached to this
	 * program.
	 */
	void build(
			const std::vector<cl_device_id> *   deviceList,
			const char *                        options,
			void (*pfn_notify)(
					cl_program, void *),
			void *                              user_data);

	/**
	 * @brief Compiles a program's source for all the devices or a specific device(s) in the OpenCL context associated with @c program.
	 *
	 * @param deviceList
	 * @param options
	 * @param inputHeaders
	 * @param pfn_notify
	 * @param user_data
	 */
	void compile(
            const std::vector<cl_device_id> *devices,
            const char *                     options,
            const Headers *                  inputHeaders,
            void (*pfn_notify)(
                    cl_program, void *),
            void *                           user_data);

	cl_context context() const;
    const std::vector<cl_device_id>& devices() const;
	void getInfo(
            cl_program_info	param_name,
            size_t			param_value_size,
            void *			param_value,
            size_t *		param_value_size_ret) const;

	void getBuildInfo(
			cl_device_id,
            cl_program_build_info	param_name,
            size_t					param_value_size,
            void *					param_value,
            size_t *				param_value_size_ret) const;

    bool hasDevice(
            cl_device_id device) const;

	/**
	 * @brief Callback method to indicate build status changes.
	 *
	 * This method is called by a ProgramBuild object to add build info to this
	 * program.
	 *
	 * @param[in]  device   the device this program has been build for
     * @param[in]  status   build status
	 * @param[in]  options  build options
	 */
	void onBuildStatusChanged(
	        cl_device_id       device,
	        cl_build_status    status,
	        const std::string& options);

protected:
	void destroy();

private:
	void init();

	std::vector<dcl::ComputeNode *> _computeNodes; /**< compute nodes hosting this program */

	cl_context _context; /**< Context associated with this program */
	std::string _source; /**< Concatenated program sources */
	std::vector<cl_device_id> _devices; /**< Devices associated with this program */
	Binaries _binaries; /**< Program binaries */
	/**
	 * @brief Build status
	 * @c true, if program a program executable has been built successfully for
	 * at least one device in the list of devices associated with program. */
	bool _isBuilt;
	cl_uint _numKernels; /**< number of kernels in program */
#ifdef CL_VERSION_1_2
	std::string _kernelNames; /**< a semi-colon separated list of kernel names in program */
#endif

	std::list<std::unique_ptr<dclicd::detail::ProgramBuild>> _programBuilds; /**< Pending programBuilds */
	
    mutable std::map<cl_device_id, dclicd::detail::ProgramBuildInfo> _buildInfo; /**< Build info of program (cached) */
	mutable std::mutex _buildStatusMutex;
};

#endif /* CL_PROGRAM_H_ */
