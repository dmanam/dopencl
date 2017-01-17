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
 * \file Remote.h
 *
 * \date 2011-04-12
 * \author Philipp Kegel
 */

#ifndef REMOTE_H_
#define REMOTE_H_

#include "DCLTypes.h"

namespace dcl {

/*!
 * \brief A base class for remote objects
 */
class Remote {
public:
    static object_id generateId();


    object_id remoteId() const;

protected:
    Remote();
    Remote(
            object_id id);
    ~Remote();

    /*!
     * \brief A (unique) object ID.
     *
     * Virtually each object in dOpenCL is assigned a unique ID for identifying
     * the object across a network.
     *
     * The ID 0 is reserved to identify \c NULL pointers or 'missing' objects.
     * Hence, this ID must not be associated with any object.
     */
    /* TODO Use UUIDs to ensure globally unique ID */
    const object_id _id;

private:
    static object_id objectCount;
};

} /* namespace dcl */

#endif /* REMOTE_H_ */
