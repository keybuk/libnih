/* libnih
 *
 * Copyright © 2011 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2011 Canonical Ltd.
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

#ifndef NIH_TEST_FILES_H
#define NIH_TEST_FILES_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fnmatch.h>


/**
 * TEST_FILENAME:
 * @_var: variable to store filename in.
 *
 * Generate a filename that may be used for testing, it's unlinked if it
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


/**
 * TEST_EXPECTED_STR:
 * @_str: string to check,
 * @_filename: filename to compare against.
 *
 * Check that the string @_str exactly matches the contents of the
 * file @_filename, which is local to the expected directory.
 **/
#define TEST_EXPECTED_STR(_str, _filename)				\
	do {								\
		char        _test_file[512];				\
		char *      _test_basename;				\
		int         _test_fd;					\
		struct stat _test_stat;					\
		char *      _test_buf;					\
									\
		strcpy (_test_file, __FILE__);				\
		_test_basename = strrchr (_test_file, '/');		\
		strcpy (_test_basename ? _test_basename + 1 : _test_file, "expected/"); \
		strcat (_test_file, (_filename));			\
									\
		_test_fd = open (_test_file, O_RDONLY);			\
		assert (_test_fd >= 0);					\
									\
		assert (fstat (_test_fd, &_test_stat) == 0);		\
									\
		_test_buf = mmap (NULL, _test_stat.st_size, PROT_READ,	\
				  MAP_SHARED, _test_fd, 0);		\
		assert (_test_buf != MAP_FAILED);			\
									\
		if ((_str) == NULL) {					\
			TEST_FAILED ("wrong value for %s, expected '%.*s' got NULL", \
				     #_str, (int)_test_stat.st_size, _test_buf); \
		} else if ((strlen (_str) != (size_t)_test_stat.st_size)	\
			   || strncmp ((_str), _test_buf, _test_stat.st_size)) \
			TEST_FAILED ("wrong value for %s, expected '%.*s' got '%s'", \
				     #_str, (int)_test_stat.st_size, _test_buf, \
				     (_str));	\
									\
		assert (munmap (_test_buf, _test_stat.st_size) == 0);	\
		assert (close (_test_fd) == 0);				\
	} while (0)

/**
 * TEST_EXPECTED_FILE:
 * @_file: open file to check,
 * @_filename: filename to compare against.
 *
 * Check that the contents of file @_file exactly matches the contents of
 * the file @_filename, which is local to the expected directory.
 **/
#define TEST_EXPECTED_FILE(_file, _filename)				\
	do {								\
		char        _test_file[512];				\
		char *      _test_basename;				\
		int         _test_fd;					\
		struct stat _test_stat_a;				\
		struct stat _test_stat_b;				\
		char *      _test_buf_a;				\
		char *      _test_buf_b;				\
									\
		strcpy (_test_file, __FILE__);				\
		_test_basename = strrchr (_test_file, '/');		\
		strcpy (_test_basename ? _test_basename + 1 : _test_file, "expected/"); \
		strcat (_test_file, (_filename));			\
									\
		_test_fd = open (_test_file, O_RDONLY);			\
		assert (_test_fd >= 0);					\
									\
		assert (fstat (_test_fd, &_test_stat_a) == 0);		\
		assert (fstat (fileno (_file), &_test_stat_b) == 0);	\
									\
		_test_buf_a = mmap (NULL, _test_stat_a.st_size, PROT_READ, \
				    MAP_SHARED, _test_fd, 0);		\
		assert (_test_buf_a != MAP_FAILED);			\
									\
		_test_buf_b = mmap (NULL, _test_stat_b.st_size, PROT_READ, \
				    MAP_SHARED, fileno (_file), 0);	\
		assert (_test_buf_b != MAP_FAILED);			\
									\
		if ((_test_stat_a.st_size != _test_stat_b.st_size)	\
		    || strncmp (_test_buf_a, _test_buf_b,			\
				_test_stat_a.st_size)) \
			TEST_FAILED ("wrong value for %s, expected '%.*s' got '%.*s'", \
				     #_file, (int)_test_stat_a.st_size, _test_buf_a, \
				     (int)_test_stat_b.st_size, _test_buf_b); \
									\
		assert (munmap (_test_buf_a, _test_stat_a.st_size) == 0); \
		assert (munmap (_test_buf_b, _test_stat_b.st_size) == 0); \
		assert (close (_test_fd) == 0);				\
	} while (0)

#endif /* NIH_TEST_FILES_H */
