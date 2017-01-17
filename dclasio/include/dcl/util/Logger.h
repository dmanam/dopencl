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
 * \file Logger.h
 *
 * \date 2011-08-08
 * \author Michel Steuwer <michel.steuwer@uni-muenster.de>
 * \author Philipp Kegel <philipp.kegel@uni-muenster.de>
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <mutex>
#include <ostream>
#include <sstream>

namespace dcl {

namespace util {

enum class Severity {
    Error = 1,
    Warning = 2,
    Info = 3,
    Debug = 4,
    Verbose = 5
};

/*!
 * \brief A simple logger
 */
class LoggerImpl: public std::ostream {
public:
    LoggerImpl();

    void setOutput(std::ostream& output);

    void setLoggingLevel(Severity severity);

    void setDefaultSeverity(Severity severity);

    /*!
     * \brief Sets logging level until next flush
     */
    void setCurrentSeverity(Severity severity);

private:
    std::string severityToString(Severity severity);

    class LoggerBuffer: public std::stringbuf {
    public:
        LoggerBuffer(LoggerImpl& logger, std::ostream& stream);

        void setOutput(std::ostream& output);

        virtual int sync();
    private:
        LoggerImpl &_logger;
        std::ostream *_output;
        std::mutex _mutex; //!< Mutex to synchronize logging
    };

    Severity _currentSeverity;
    Severity _defaultSeverity;
    Severity _maxSeverity;
    LoggerBuffer _buffer;
};

/*
 * custom manipulators
 */

LoggerImpl& operator<<(
        LoggerImpl& logger,
        LoggerImpl& (*manipulator)(LoggerImpl&));

LoggerImpl& Error(LoggerImpl& logger);

LoggerImpl& Warning(LoggerImpl& logger);

LoggerImpl& Info(LoggerImpl& logger);

LoggerImpl& Debug(LoggerImpl& logger);

LoggerImpl& Verbose(LoggerImpl& logger);

extern LoggerImpl Logger;

} // namespace util

} // namespace dcl

#endif // LOGGER_H_
