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
 * \file main.cpp
 *
 * \date 2011-01-16
 * \author Philipp Kegel
 *
 * Start program of dOpenCL daemon
 */

#ifdef DAEMON
// feature test macro for lockf
#define _BSD_SOURCE
#endif
// feature test macro for sigaction
#define _POSIX_SOURCE

#include "dOpenCLd.h"

#include <dcl/DCLException.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <boost/program_options.hpp>

#ifdef DAEMON
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <signal.h>
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#include <unistd.h>
#endif

std::unique_ptr<dcld::dOpenCLd> dcl_daemon; //!< dOpenCL daemon

#if defined(_POSIX_VERSION)
void install_signal_handlers();
#endif

/* ****************************************************************************/

void terminate(int signum) {
    assert(signum == SIGINT || signum == SIGTERM);
	if (!dcl_daemon) return;
	dcl_daemon->terminate();
}

int main(int argc, char **argv) {
    boost::program_options::variables_map vm;
	std::string platform;
	std::string url;

	try {
	    boost::program_options::options_description options("Allowed options");
	    boost::program_options::options_description arguments("Allowed arguments");
	    boost::program_options::options_description all("Usage: dcld [options] <host name>");
	    boost::program_options::positional_options_description p;

        // specify program options
        options.add_options()
            ("help", "produce help message")
            ("platform,p", boost::program_options::value<std::string>(&platform),
                    "OpenCL platform to use")
            ;
        arguments.add_options()
            ("hostname", boost::program_options::value<std::string>(&url),
                    "daemon interface")
            ;
        all.add(options).add(arguments);

        // specify positional arguments
        p.add("hostname", 1);

        // parse program options
        boost::program_options::store(boost::program_options::command_line_parser(
                argc, argv).options(all).positional(p).run(), vm);

        if (vm.count("help")) {
            std::cout << all << std::endl;
            return EXIT_SUCCESS;
        }

        boost::program_options::notify(vm);
	} catch (const boost::program_options::error& err) {
	    std::cerr << err.what() << std::endl;
	    return EXIT_FAILURE;
	}

#ifdef DAEMON
    /* TODO Create a true daemon; see
     *   http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html
     *   http://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/example/fork/daemon.cpp
     *   http://openbook.galileocomputing.de/linux_unix_programmierung/Kap07-011.htm#RxxKap07011040002021F048100
     */

    // Fork off the parent process
    if (pid_t pid = fork()) {
        if (pid < 0) {
            syslog(LOG_ERR | LOG_USER, "Cannot fork daemon process: %m");
            return EXIT_FAILURE;
        } else {
            /* exit the parent process */
            return EXIT_SUCCESS;
        }
    }

    /*
     * now we are in the first child process
     */

    /* Make the process the leader of a new session in order to detach it from
     * the controlling terminal */
    if (setsid() < 0) {
        syslog(LOG_ERR | LOG_USER, "Failed to create session: %m");
        return EXIT_FAILURE;
    }

    /* Fork off the parent process again to ensure that the new process cannot
     * acquire a controlling terminal */
    if (pid_t pid = fork()) {
        if (pid < 0) {
            syslog(LOG_ERR | LOG_USER, "Second fork failed: %m");
            return EXIT_FAILURE;
        } else {
            return EXIT_SUCCESS; // exit the parent process
        }
    }

    /*
     * now we are in the second child process, which is the actual daemon process
     */

    // change the current working directory
    if ((chdir("/tmp")) < 0) {
        syslog(LOG_ERR | LOG_USER, "Cannot change working directory: %m");
        return EXIT_FAILURE;
    }

    umask(0); // change the file mode mask

    // use file lock to ensure mutual exclusion of multiple daemon instances
    int lock_file = open("/tmp/dcld.lock", O_RDWR | O_CREAT, 0640);
    if (lock_file < 0) {
        syslog(LOG_ERR | LOG_USER, "Cannot open lock file: %m");
        return EXIT_FAILURE;
    }
    if (lockf(lock_file, F_TLOCK, 0) < 0) {
        // another daemon instance is already running
        return EXIT_SUCCESS;
    }
    // write PID to lock file
//    sprintf(str, "%d\n", getpid());
//    write(lock_file, str, strlen(str));

    // close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // reopen standard file descriptors and forward to /dev/null
    int fd = open("/dev/null", O_RDWR); // STDIN
    dup(fd); // STDOUT
    dup(fd); // STDERR

    /* open system log
     * Use system log for standard messages and log all the rest into dedicated
     * log files */
//    openlog("dcld", LOG_PID | LOG_CONS | LOGNDELAY, LOG_DAEMON);
#endif

    // install event handlers
#if defined(_WIN32)
    // TODO Use messages for event handling in Windows
#else
#if defined(_POSIX_VERSION)
    // Use signals for event handling in POSIX-compliant OS's
    install_signal_handlers();
#endif
#endif

    /*
     * start daemon
     */
    try {
        // create daemon instance
        dcl_daemon.reset(new dcld::dOpenCLd(url,
                (vm.count("platform") ? &platform : nullptr)));
		dcl_daemon->run();
		dcl_daemon.reset(); // destroy daemon
	} catch (const dcl::DCLException& err) {
		std::cerr << err.what() << std::endl;
		return EXIT_FAILURE;
	} catch (const cl::Error& err) {
		switch (err.err()) {
		case CL_PLATFORM_NOT_FOUND_KHR: // no platforms were found
			std::cerr << "Platform not found." << std::endl;
			break;
		default:
			std::cerr << "OpenCL error: " << err.err() << std::endl;
			// no break
		}
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/* ****************************************************************************/

#if defined(_POSIX_VERSION)
void install_signal_handlers() {
    struct sigaction no_action, terminate_action;

    no_action.sa_handler = SIG_IGN;
    sigemptyset(&no_action.sa_mask);
    no_action.sa_flags = 0;

    terminate_action.sa_handler = &terminate;
    sigemptyset(&terminate_action.sa_mask);
    sigaddset(&terminate_action.sa_mask, SIGINT); // block INT signal during termination
    sigaddset(&terminate_action.sa_mask, SIGTERM); // block TERM signal during termination
    terminate_action.sa_flags = 0;

    sigaction(SIGHUP, &no_action, nullptr); // ignore SIGHUP, no config file to read
    sigaction(SIGINT, &terminate_action, nullptr); // 'program interrupt', e.g., user pressed Ctrl-C
    sigaction(SIGTERM, &terminate_action, nullptr); // default signal to terminate program
}
#endif /* defined(_POSIX_VERSION) */
