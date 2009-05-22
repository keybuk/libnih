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

#ifndef NIH_TEST_FILES_H
#define NIH_TEST_FILES_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fnmatch.h>


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
 * TEST_FILE_MATCH:
 * @_file: FILE to read from,
 * @_pattern: pattern to expect.
 *
 * Check that the next line in the file @_file matches the glob pattern
 * @_pattern, which should include the terminating newline if one is expected.
 **/
#define TEST_FILE_MATCH(_file, _pattern) \
	do { \
		char _test_file[512]; \
		if (! fgets (_test_file, sizeof (_test_file), (_file))) \
			TEST_FAILED ("eof on file %p (%s), expected '%s'", \
				     (_file), #_file, (_pattern)); \
		if (fnmatch ((_pattern), _test_file, 0))		\
			TEST_FAILED ("wrong content in file %p (%s), expected '%s' got '%s'", \
			     (_file), #_file, (_pattern), _test_file); \
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
		if (! strcmp (_test_file, (_line))) \
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
		if (! strncmp (_test_file, (_line), strlen (_line))) \
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
		assert0 (ftruncate (fileno (_file), 0)); \
	} while (0)

#endif /* NIH_TEST_FILES_H */
