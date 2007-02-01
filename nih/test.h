/* libnih
 *
 * Copyright © 2007 Scott James Remnant <scott@netsplit.com>.
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

#ifndef NIH_TEST_H
#define NIH_TEST_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>


/**
 * assert0:
 * @_expr: expression to check.
 *
 * Wrapper around the usual assert() function that handles the common case
 * of asserting that @_expr returns zero, rather than a TRUE value.
 **/
#define assert0(_expr) \
	assert ((_expr) == 0)

/**
 * TEST_FUNCTION:
 * @_func: name of function being tested.
 *
 * Output a message indicating that tests of the function named @_func are
 * being performed.
 **/
#define TEST_FUNCTION(_func) \
	printf ("Testing %s()\n", _func)

/**
 * TEST_FEATURE:
 * @_feat: name of function feature being tested.
 *
 * Output a message indicating that a sub-test of a function is being
 * performed, specifically the feature named _feat.
 **/
#define TEST_FEATURE(_feat) \
	printf ("...%s\n", _feat);

/**
 * TEST_FAILED:
 * @_fmt: format string.
 *
 * Output a formatted message indicating that a test has failed, including
 * the file, line number and function where the failure happened.
 **/
#define TEST_FAILED(_fmt, ...) \
	do { \
		printf ("BAD: " _fmt "\n\tat %s:%d (%s).\n", \
			##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__); \
		abort (); \
	} while (0)


/**
 * TEST_TRUE:
 * @_expr: value or expression to test.
 *
 * Check that the expression @_expr evaluates to TRUE.
 **/
#define TEST_TRUE(_expr) \
	if (! (_expr)) \
		TEST_FAILED ("wrong value for %s, expected TRUE got FALSE", \
			     #_expr)

/**
 * TEST_FALSE:
 * @_expr: value or expression to test.
 *
 * Check that the expression @_expr evaluates to FALSE.
 **/
#define TEST_FALSE(_expr) \
	if (_expr) \
		TEST_FAILED ("wrong value for %s, expected FALSE got TRUE", \
			     #_expr)

/**
 * TEST_EQ:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the two numeric values @_a and @_b are equal, they are cast
 * to ssize_t for display purposes.
 **/
#define TEST_EQ(_a, _b) \
	if ((_a) != (_b)) \
		TEST_FAILED ("wrong value for %s, expected %zi got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))

/**
 * TEST_EQ_U:
 * @_a: first unsigned integer,
 * @_b: second unsigned integer.
 *
 * Check that the two numeric values @_a and @_b are equal, they are cast
 * to size_t for display purposes.
 **/
#define TEST_EQ_U(_a, _b) \
	if ((_a) != (_b)) \
		TEST_FAILED ("wrong value for %s, expected %zu got %zu", \
			     #_a, (size_t)(_b), (size_t)(_a))

/**
 * TEST_EQ_P:
 * @_a: first pointer,
 * @_b: second pointer.
 *
 * Check that the two pointers @_a and @_b are equal.
 **/
#define TEST_EQ_P(_a, _b) \
	if ((_a) != (_b)) \
		TEST_FAILED ("wrong value for %s, expected %p got %p", \
			     #_a, (_b), (_a))

/**
 * TEST_EQ_STR:
 * @_a: first string,
 * @_b: second string.
 *
 * Check that the two strings @_a and @_b are equal.
 **/
#define TEST_EQ_STR(_a, _b) \
	if (strcmp ((_a), (_b))) \
		TEST_FAILED ("wrong value for %s, expected '%s' got '%s'", \
			     #_a, (_b), (_a))

/**
 * TEST_EQ_STRN:
 * @_a: first string,
 * @_b: second string.
 *
 * Check that the two strings @_a and @_b are equal, up to the length of
 * the second string.
 **/
#define TEST_EQ_STRN(_a, _b) \
	if (strncmp ((_a), (_b), strlen (_b))) \
		TEST_FAILED ("wrong value for %s, expected '%.*s' got '%.*s'", \
			     #_a, (int)strlen (_b), (_b), \
			     (int)strlen (_b), (_a))

/**
 * TEST_EQ_MEM:
 * @_a: first memory area,
 * @_b: second memory area,
 * @_l: length of @_a and @_b.
 *
 * Check that the two @_l byte long areas of memory at @_a and @_b are
 * identical.
 **/
#define TEST_EQ_MEM(_a, _b, _l) \
	if (memcmp ((_a), (_b), (_l))) \
		TEST_FAILED ("wrong %zu bytes at %p (%s), expected %p (%s)", \
			     (size_t)(_l), (_a), #_a, (_b), #_b)

/**
 * TEST_NE:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the two numeric values @_a and @_b are not equal, they are
 * cast to ssize_t for display purposes.
 **/
#define TEST_NE(_a, _b) \
	if ((_a) == (_b)) \
		TEST_FAILED ("wrong value for %s, got unexpected %zi", \
			     #_a, (ssize_t)(_b))

/**
 * TEST_NE_U:
 * @_a: first unsigned integer,
 * @_b: second unsigned integer.
 *
 * Check that the two numeric values @_a and @_b are not equal, they are
 * cast to size_t for display purposes.
 **/
#define TEST_NE_U(_a, _b) \
	if ((_a) == (_b)) \
		TEST_FAILED ("wrong value for %s, got unexpected %zu", \
			     #_a, (size_t)(_b))

/**
 * TEST_NE_P:
 * @_a: first pointer,
 * @_b: second pointer.
 *
 * Check that the two pointers @_a and @_b are not equal.
 **/
#define TEST_NE_P(_a, _b) \
	if ((_a) == (_b)) \
		TEST_FAILED ("wrong value for %s, got unexpected %p", \
			     #_a, (_b))

/**
 * TEST_NE_STR:
 * @_a: first string,
 * @_b: second string.
 *
 * Check that the two strings @_a and @_b are not equal.
 **/
#define TEST_NE_STR(_a, _b) \
	if (! strcmp ((_a), (_b))) \
		TEST_FAILED ("wrong value for %s, got unexpected '%s'", \
			     #_a, (_b))

/**
 * TEST_NE_STRN:
 * @_a: first string,
 * @_b: second string.
 *
 * Check that the two strings @_a and @_b are not equal, up to the length
 * of the second string.
 **/
#define TEST_NE_STRN(_a, _b) \
	if (! strncmp ((_a), (_b), strlen (_b))) \
		TEST_FAILED ("wrong value for %s, got unexpected '%.*s'", \
			     #_a, (int)strlen (_b), (_b))

/**
 * TEST_NE_MEM:
 * @_a: first memory area,
 * @_b: second memory area,
 * @_l: length of @_a and @_b.
 *
 * Check that the two @_l byte long areas of memory at @_a and @_b are
 * different.
 **/
#define TEST_NE_MEM(_a, _b, _l) \
	if (! memcmp ((_a), (_b), (_l))) \
		TEST_FAILED ("wrong %zu bytes at %p (%s), got unexpected %p (%s)", \
			     (size_t)(_l), (_a), #_a, (_b), #_b)

/**
 * TEST_LT:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the numeric value @_a is less than the numeric value @_b,
 * they are cast to ssize_t for display purposes.
 **/
#define TEST_LT(_a, _b) \
	if ((_a) >= (_b)) \
		TEST_FAILED ("wrong value for %s, expected less than %zi got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))

/**
 * TEST_LE:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the numeric value @_a is less than or equal to the numeric
 * value @_b, they are cast to ssize_t for display purposes.
 **/
#define TEST_LE(_a, _b) \
	if ((_a) > (_b)) \
		TEST_FAILED ("wrong value for %s, expected %zi or lower got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))

/**
 * TEST_GT:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the numeric value @_a is greater than the numeric value @_b,
 * they are cast to ssize_t for display purposes.
 **/
#define TEST_GT(_a, _b) \
	if ((_a) <= (_b)) \
		TEST_FAILED ("wrong value for %s, expected greater than %zi got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))

/**
 * TEST_GE:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the numeric value @_a is greater than or equal to the numeric
 * value @_b, they are cast to ssize_t for display purposes.
 **/
#define TEST_GE(_a, _b) \
	if ((_a) < (_b)) \
		TEST_FAILED ("wrong value for %s, expected %zi or greater got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))


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
		pipe (_test_fds); \
		_pid = fork (); \
		if (_pid > 0) { \
			char _test_buf[1]; \
			close (_test_fds[1]); \
			read (_test_fds[0], _test_buf, 1); \
			close (_test_fds[0]); \
		} else if (_pid == 0) { \
			close (_test_fds[0]); \
			write (_test_fds[1], "\n", 1); \
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
		pipe (_test_fds); \
		_pid = fork (); \
		if (_pid > 0) { \
			char _test_buf[1]; \
			close (_test_fds[1]); \
			read (_test_fds[0], _test_buf, 1); \
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
		write ((_fd), "\n", 1); \
		close (_fd); \
	} while (0)

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
			dup2 ((_fd), STDOUT_FILENO); \
		} else if (_test_stdout > 1) { \
			fflush (stdout); \
			dup2 (_test_oldstdout, STDOUT_FILENO); \
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
			dup2 ((_fd), STDERR_FILENO); \
		} else if (_test_stderr > 1) { \
			fflush (stderr); \
			dup2 (_test_oldstderr, STDERR_FILENO); \
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

/**
 * TEST_FILENAME:
 * @_var: variable to store filename in.
 *
 * Generate a filename that may be used for testing, it's unlinked it if
 * exists and it's up to you to unlink it when done.  @_var should be at
 * least PATH_MAX long.
 **/
#define TEST_FILENAME(_var) \
	do { \
		snprintf ((_var), sizeof (_var), "/tmp/%s-%s-%d-%d", \
			  strrchr (__FILE__, '/') ? strrchr (__FILE__, '/') + 1 : __FILE__, \
			  __FUNCTION__, __LINE__, getpid ()); \
		unlink (_var); \
	} while (0)

/**
 * TEST_FILE_EQ:
 * @_file: FILE to read from,
 * @_line: line to expect.
 *
 * Check that the next line in the file @_file is @_line, which should
 * include the terminating newline if one is expected.
 **/
#define TEST_FILE_EQ(_file, _line) \
	do { \
		char _test_file[512]; \
		if (! fgets (_test_file, sizeof (_test_file), (_file))) \
			TEST_FAILED ("eof on file %p (%s), expected '%s'", \
				     (_file), #_file, (_line)); \
		if (strcmp (_test_file, (_line))) \
			TEST_FAILED ("wrong content in file %p (%s), expected '%s' got '%s'", \
			     (_file), #_file, (_line), _test_file); \
	} while (0)

/**
 * TEST_FILE_EQ_N:
 * @_file: FILE to read from,
 * @_line: line to expect.
 *
 * Check that the start of the next line in the file @_file is @_line, up to
 * the length of that argument.
 **/
#define TEST_FILE_EQ_N(_file, _line) \
	do { \
		char _test_file[512]; \
		if (! fgets (_test_file, sizeof (_test_file), (_file))) \
			TEST_FAILED ("eof on file %p (%s), expected '%s'", \
				     (_file), #_file, (_line)); \
		if (strncmp (_test_file, (_line), strlen (_line))) \
			TEST_FAILED ("wrong content in file %p (%s), expected '%.*s' got '%.*s'", \
			     (_file), #_file, (int)strlen (_line), (_line), \
			     (int)strlen (_line), _test_file); \
	} while (0)

/**
 * TEST_FILE_NE:
 * @_file: FILE to read from,
 * @_line: line to expect.
 *
 * Check that the next line in the file @_file is not @_line, but also not
 * end of file.
 **/
#define TEST_FILE_NE(_file, _line) \
	do { \
		char _test_file[512]; \
		if (! fgets (_test_file, sizeof (_test_file), (_file))) \
			TEST_FAILED ("eof on file %p (%s), expected line other than '%s'", \
				     (_file), #_file, (_line)); \
		if (strcmp (_test_file, (_line))) \
			TEST_FAILED ("wrong content in file %p (%s), got unexpected '%s'", \
			     (_file), #_file, (_line)); \
	} while (0)

/**
 * TEST_FILE_NE_N:
 * @_file: FILE to read from,
 * @_line: line to expect.
 *
 * Check that the next line in the file @_file does not start with @_line,
 * up to the length of that argument; but also not end of file.
 **/
#define TEST_FILE_NE_N(_file, _line) \
	do { \
		char _test_file[512]; \
		if (! fgets (_test_file, sizeof (_test_file), (_file))) \
			TEST_FAILED ("eof on file %p (%s), expected line other than '%s'", \
				     (_file), #_file, (_line)); \
		if (strncmp (_test_file, (_line), strlen (_line))) \
			TEST_FAILED ("wrong content in file %p (%s), got unexpected '%.*s'", \
			     (_file), #_file, (int)strlen (_line), (_line)); \
	} while (0)

/**
 * TEST_FILE_END:
 * @_file: FILE to check.
 *
 * Check that the end of the file @_file has been reached, and that there
 * are no more lines to read.
 **/
#define TEST_FILE_END(_file) \
	do { \
		char _test_file[512];\
		if (fgets (_test_file, sizeof (_test_file), (_file))) \
			TEST_FAILED ("wrong content in file %p (%s), expected eof got '%s'", \
				     (_file), #_file, _test_file); \
	} while (0)

/**
 * TEST_FILE_RESET:
 * @_file: FILE to reset.
 *
 * This macro may be used to reset a temporary file such that it can be
 * treated as a new one.
 **/
#define TEST_FILE_RESET(_file) \
	do { \
		fflush (_file); \
		rewind (_file); \
		ftruncate (fileno (_file), 0); \
	} while (0)


/**
 * TEST_ALLOC_SIZE:
 * @_ptr: allocated pointer,
 * @_sz: expected size.
 *
 * Check that the pointer @_ptr was allocated with nih_alloc(), and is @_sz
 * bytes in length (which includes the context).
 **/
#define TEST_ALLOC_SIZE(_ptr, _sz) \
	if (nih_alloc_size (_ptr) != (_sz)) \
		TEST_FAILED ("wrong size of block %p (%s), expected %zu got %zu", \
			     (_ptr), #_ptr, (size_t)(_sz), \
			     nih_alloc_size (_ptr))

/**
 * TEST_ALLOC_PARENT:
 * @_ptr: allocated pointer,
 * @_parent: expected parent.
 *
 * Check that the pointer @_ptr was allocated with nih_alloc() and has
 * the other block @_parent as a parent.
 **/
#define TEST_ALLOC_PARENT(_ptr, _parent) \
	if (nih_alloc_parent (_ptr) != (_parent)) \
		TEST_FAILED ("wrong parent of block %p (%s), expected %p (%s) got %p", \
			     (_ptr), #_ptr, (_parent), #_parent, \
			     nih_alloc_parent (_ptr))

/**
 * test_alloc_failed:
 *
 * Variable used by TEST_ALLOC_FAIL as the loop counter.
 **/
static int test_alloc_failed = 0;

/**
 * _test_alloc_count:
 *
 * Number of times malloc is called by the TEST_ALLOC_FAIL macro.
 **/
static int _test_alloc_count = 0;

/**
 * _test_alloc_call:
 *
 * Number of times malloc has been called during each cycle.
 **/
static int _test_alloc_call = 0;

/**
 * _test_allocator:
 *
 * Allocator used by TEST_ALLOC_FAIL; when test_alloc_failed is zero, it
 * increments test_alloc_count and returns whatever realloc does.  Otherwise
 * it internally counts the number of times it is called, and if that matches
 * test_alloc_failed, then it returns NULL.
 **/
static inline void *
_test_allocator (void   *ptr,
		 size_t  size)
{
	if (! size)
		return realloc (ptr, size);

	if (! test_alloc_failed) {
		_test_alloc_count++;

		return realloc (ptr, size);
	}

	_test_alloc_call++;
	if (test_alloc_failed == _test_alloc_call) {
		errno = ENOMEM;
		return NULL;
	} else {
		return realloc (ptr, size);
	}
}

/**
 * TEST_ALLOC_FAIL:
 *
 * This macro expands to code that runs the following block repeatedly; the
 * first time (when the special test_alloc_failed variable is zero) is
 * used to determine how many allocations are performed by the following block;
 * subsequent calls (when test_alloc_failed is a positive integer) mean that
 * the test_alloc_failedth call to realloc has failed.
 *
 * This cannot be nested as it relies on setting an alternate allocator
 * and sharing a global state.
 **/
#define TEST_ALLOC_FAIL \
	for (test_alloc_failed = -1; \
	     test_alloc_failed <= (_test_alloc_count + 1); \
	     test_alloc_failed++, _test_alloc_call = 0) \
		if (test_alloc_failed < 0) { \
			_test_alloc_count = 0; \
			nih_alloc_set_allocator (_test_allocator); \
		} else if (test_alloc_failed \
			   && (test_alloc_failed == \
			       (_test_alloc_count + 1))) { \
			nih_alloc_set_allocator (realloc); \
		} else

/**
 * TEST_ALLOC_SAFE:
 *
 * This macro may be used within a TEST_ALLOC_FAIL block to guard the
 * following block of code from failing allocation.
 **/
#define TEST_ALLOC_SAFE \
	for (int _test_alloc_safe = 0; _test_alloc_safe < 3; \
	     _test_alloc_safe++) \
		if (_test_alloc_safe < 1) { \
			nih_alloc_set_allocator (realloc); \
		} else if (_test_alloc_safe > 1) { \
			nih_alloc_set_allocator (_test_allocator); \
		} else

/**
 * TEST_LIST_EMPTY:
 * @_list: entry in list.
 *
 * Check that the list of which @_list is a member is empty, ie. that
 * @_list is the sole member.
 **/
#define TEST_LIST_EMPTY(_list) \
	if (! NIH_LIST_EMPTY (_list)) \
		TEST_FAILED ("list %p (%s) not empty as expected", \
			     (_list), #_list)

/**
 * TEST_LIST_NOT_EMPTY:
 * @_list: entry in list.
 *
 * Check that the list of which @_list is a member is not empty, ie. that
 * there are more members than just @_list.
 **/
#define TEST_LIST_NOT_EMPTY(_list) \
	if (NIH_LIST_EMPTY (_list)) \
		TEST_FAILED ("list %p (%s) empty, expected multiple members", \
			     (_list), #_list)

#endif /* NIH_TEST_H */
