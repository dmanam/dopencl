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
 * \file Logger.cpp
 *
 * \date 2011-08-08
 * \author Michel Steuwer <michel.steuwer@uni-muenster.de>
 * \author Philipp Kegel <philipp.kegel@uni-muenster.de>
 */

#include <dcl/util/Logger.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <ostream>

namespace {

auto start = std::chrono::high_resolution_clock::now();

} // unnamed namespace

/******************************************************************************/

namespace dcl {

namespace util {

LoggerImpl Logger;

// LoggerImpl implementation

LoggerImpl::LoggerImpl() :
        std::ostream(&_buffer), _currentSeverity(Severity::Info), _defaultSeverity(
                Severity::Info), _maxSeverity(Severity::Warning), _buffer(*this,
                std::clog) {
}

void LoggerImpl::setOutput(std::ostream& output) {
    _buffer.setOutput(output);
}

/* TODO Distinguish logging level and message severity
 * Logging level may be set to 'NONE' to disable logging, but message severity
 * must not be 'NONE' */
void LoggerImpl::setLoggingLevel(Severity severity) {
    _maxSeverity = severity;
}

void LoggerImpl::setDefaultSeverity(Severity severity) {
    _defaultSeverity = severity;
    _currentSeverity = severity;
}

void LoggerImpl::setCurrentSeverity(Severity severity) {
    _currentSeverity = severity;
}

std::string LoggerImpl::severityToString(Severity severity) {
    switch (severity) {
    case Severity::Error:   return "ERROR  ";
    case Severity::Warning: return "WARNING";
    case Severity::Info:    return "INFO   ";
    case Severity::Debug:   return "DEBUG  ";
    case Severity::Verbose: return "VERBOSE";
    default:                return "       ";
    }
}

LoggerImpl& operator<<(
        LoggerImpl& logger,
        LoggerImpl& (*manipulator)(LoggerImpl&)) {
    return manipulator(logger);
}

// LoggerBuffer implementation

LoggerImpl::LoggerBuffer::LoggerBuffer(LoggerImpl& logger, std::ostream& stream) :
        std::stringbuf(), _logger(logger), _output(&stream) {
}

void LoggerImpl::LoggerBuffer::setOutput(std::ostream& output) {
    _output = &output;
}

int LoggerImpl::LoggerBuffer::sync() {
    std::lock_guard<std::mutex> lock(_mutex);

    if (_logger._currentSeverity <= _logger._maxSeverity) {
        auto time = std::chrono::high_resolution_clock::now() - start;
        (*_output) << _logger.severityToString(_logger._currentSeverity) << " ["
                << std::chrono::duration_cast<std::chrono::seconds>(time).count()
                << ':' << std::setw(6) << std::setfill('0')
                << (std::chrono::duration_cast<std::chrono::microseconds>(time).count() % 1000000)
                << "] " << str();
    }
    // clear buffer content
    str("");
    // flush output
    _output->flush();
    // reset logging level
    _logger.setCurrentSeverity(_logger._defaultSeverity);
    return 0;
}

/* FIXME Logger manipulators are not thread-safe
 * While one thread sets _currentSeverity for a message to be logged
 * subsequently, a concurrent thread may change _currentSeverity before the
 * message will be logged.
 * Possible fix: use thread-local storage to set _currentSeverity for each
 * thread. */

// LoggerImpl manipulators
LoggerImpl& Error(LoggerImpl& logger) {
    logger.setCurrentSeverity(Severity::Error);
    return logger;
}

LoggerImpl& Warning(LoggerImpl& logger) {
    logger.setCurrentSeverity(Severity::Warning);
    return logger;
}

LoggerImpl& Info(LoggerImpl& logger) {
    logger.setCurrentSeverity(Severity::Info);
    return logger;
}

LoggerImpl& Debug(LoggerImpl& logger) {
    logger.setCurrentSeverity(Severity::Debug);
    return logger;
}

LoggerImpl& Verbose(LoggerImpl& logger) {
    logger.setCurrentSeverity(Severity::Verbose);
    return logger;
}

} // namespace util

} // namespacce dcl

