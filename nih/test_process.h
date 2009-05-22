/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
 * This macro ensures that the child has begun exectution before the
 * parent is allowed to continue through the usual use of a pipe.
 *
 * A block of code should follow this macro, which is the code that will
 * be run in the child process; if the block ends, the child will abort.
 **/
#define TEST_CHILD(_pid) \
	do { \
		int _test_fds[2]; \
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
