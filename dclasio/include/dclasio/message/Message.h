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
 * \file Message.h
 *
 * \date 2014-03-20
 * \author Philipp Kegel
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <dcl/ByteBuffer.h>

#include <cstdint>
#include <map>

namespace dclasio {
namespace message {

/*!
 * \brief Message interface
 */
class Message {
public:
    typedef uint32_t size_type; //! message size
    typedef uint32_t class_type; //!< message type

    virtual ~Message() { }

    /*!
     * \brief Returns the message type
     *
     * \return the message type
     */
    virtual class_type get_type() const = 0;

    virtual void pack(
            dcl::ByteBuffer& buf) const = 0;
    virtual void unpack(
            dcl::ByteBuffer& buf) = 0;
};

/* ****************************************************************************/

/*!
 * \brief Abstract message base class providing the message type
 */
template<Message::class_type Type>
class BasicMessage : virtual public Message {
public:
    static const class_type TYPE = Type;

    class_type get_type() const {
        return TYPE;
    }
};

/* ****************************************************************************/

/*!
 * \brief Creates a default instance of a message of the specified type
 * \param[in]  type the type of message to create
 * \return a message of type \c type
 * \throw std::invalid_argument if the specified message type is unknown
 */
Message * createMessage(
        Message::class_type type);

} /* namespace message */
} /* namespace dclasio */

#endif /* MESSAGE_H_ */
