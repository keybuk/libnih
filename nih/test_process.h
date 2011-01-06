/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef NIH_TEST_PROCESS_H
#define NIH_TEST_PROCESS_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <sys/types.h>

#include <unistd.h>


/**
 * TEST_CHILD:
 * @_pid: variable to store pid in.
 *
 * Spawn a child in which a test can be performed without affecting the
 * main flow of the process.  The pid of the child is stored in @_pid.
 *
 * This macro ensures that the child has begun execution before the
 * parent is allowed to continue through the usual use of a pipe.
 *
 * A block of code should follow this macro, which is the code that will
 * be run in the child process; if the block ends, the child will abort.
 **/
#define TEST_CHILD(_pid) \
	do { \
		int _test_fds[2]; \
		fflush (stdout);	    \
		fflush (stderr);	    \
		assert0 (pipe (_test_fds)); \
		_pid = fork (); \
		if (_pid > 0) { \
			char _test_buf[1]; \
			close (_test_fds[1]); \
			assert (read (_test_fds[0], _test_buf, 1) == 1); \
			close (_test_fds[0]); \
		} else if (_pid == 0) { \
			close (_test_fds[0]); \
			assert (write (_test_fds[1], "\n", 1) == 1); \
			close (_test_fds[1]); \
		} \
	} while (0); \
	if (_pid == 0) \
		for (int _test_child = 0; _test_child < 2; _test_child++) \
			if (_test_child) { \
				abort (); \
			} else

/**
 * TEST_CHILD_WAIT:
 * @_pid: variable to store pid in,
 * @_fd: variable to store lock fd in.
 *
 * Spawn a child in which a test can be performed while the parent waits
 * in this macro for the child to reach the TEST_CHILD_RELEASE macro
 * or die.
 *
 * The pid of the child is stored in @_pid, a file descriptor is stored
 * in the @_fd variable which is used by the TEST_CHILD_RELEASE macro.
 *
 * A block of code should follow this macro, which is the code that will
 * be run in the child process; if the block ends, the child will abort.
 **/
#define TEST_CHILD_WAIT(_pid, _fd) \
	do { \
		int _test_fds[2]; \
		fflush (stdout);	    \
		fflush (stderr);	    \
		assert0 (pipe (_test_fds)); \
		_pid = fork (); \
		if (_pid > 0) { \
			char _test_buf[1]; \
			close (_test_fds[1]); \
			assert (read (_test_fds[0], _test_buf, 1) == 1); \
			close (_test_fds[0]); \
		} else if (_pid == 0) { \
			close (_test_fds[0]); \
			_fd = _test_fds[1]; \
		} \
	} while (0); \
	if (_pid == 0) \
		for (int _test_child = 0; _test_child < 2; _test_child++) \
			if (_test_child) { \
				abort (); \
			} else

/**
 * TEST_CHILD_RELEASE:
 * @_fd: variable lock fd stored in.
 *
 * Release the parent of a child spawned by TEST_CHILD_WAIT now that the
 * child has reached the critical point.
 **/
#define TEST_CHILD_RELEASE(_fd) \
	do { \
		assert (write ((_fd), "\n", 1) == 1);	\
		close (_fd); \
	} while (0)

#endif /* NIH_TEST_PROCESS_H */
