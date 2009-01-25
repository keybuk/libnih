/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef NIH_TEST_DIVERT_H
#define NIH_TEST_DIVERT_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>

/**
 * TEST_DIVERT_STDOUT_FD:
 * @_fd: fd to send standard output to.
 *
 * This macro diverts standard output to a different file descriptor
 * for the duration of a block of code that should follow it.
 **/
#define TEST_DIVERT_STDOUT_FD(_fd) \
	for (int _test_stdout = 0, _test_oldstdout = dup (STDOUT_FILENO); \
	     _test_stdout < 3; _test_stdout++) \
		if (_test_stdout < 1) { \
			fflush (stdout); \
			assert (dup2 ((_fd), STDOUT_FILENO) >= 0); \
		} else if (_test_stdout > 1) { \
			fflush (stdout); \
			assert (dup2 (_test_oldstdout, STDOUT_FILENO) >= 0); \
			close (_test_oldstdout); \
		} else

/**
 * TEST_DIVERT_STDOUT:
 * @_file: FILE to send standard output to.
 *
 * This macro diverts standard output to a different file for the duration
 * of a block of code that should follow it.
 */
#define TEST_DIVERT_STDOUT(_file) \
	TEST_DIVERT_STDOUT_FD (fileno (_file))

/**
 * TEST_DIVERT_STDERR_FD:
 * @_fd: fd to send standard error to.
 *
 * This macro diverts standard error to a different file descriptor for the
 * duration of a block of code that should follow it.
 */
#define TEST_DIVERT_STDERR_FD(_fd) \
	for (int _test_stderr = 0, _test_oldstderr = dup (STDERR_FILENO); \
	     _test_stderr < 3; _test_stderr++) \
		if (_test_stderr < 1) { \
			fflush (stderr); \
			assert (dup2 ((_fd), STDERR_FILENO) >= 0); \
		} else if (_test_stderr > 1) { \
			fflush (stderr); \
			assert (dup2 (_test_oldstderr, STDERR_FILENO) >= 0); \
			close (_test_oldstderr); \
		} else

/**
 * TEST_DIVERT_STDERR:
 * @_file: FILE to send standard error to.
 *
 * This macro diverts standard error to a different file for the duration
 * of a block of code that should follow it.
 */
#define TEST_DIVERT_STDERR(_file) \
	TEST_DIVERT_STDERR_FD (fileno (_file))

#endif /* NIH_TEST_DIVERT_H */
