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
 * \file Program.h
 *
 * \date 2012-08-05
 * \author Philipp Kegel
 *
 * dOpenCL program API
 */

#ifndef DCL_PROGRAM_H_
#define DCL_PROGRAM_H_

#include <memory>
#include <vector>

namespace dcl {

class Device;
class Kernel;
class ProgramBuildListener;

/* ****************************************************************************/

/*!
 * \brief Remote interface of a program.
 */
class Program {
public:
    virtual ~Program() { }

    /* TODO Pass program build listener by raw pointer or reference */
    /*!
     * \brief Build a program
     *
     * \param[in]  deviceList           the devices to build the program for
     * \param[in]  options              build options
     * \param[in]  programBuildListener a proxy for a remote program build listener
     */
    virtual void build(
            const std::vector<Device *>&                    deviceList,
            const char *                                    options,
            const std::shared_ptr<ProgramBuildListener>&    programBuildListener) = 0;

    virtual void createKernels(
            std::vector<std::shared_ptr<Kernel>>&   kernels) = 0;

};

} /* namespace dcl */

#endif /* DCL_PROGRAM_H_ */
