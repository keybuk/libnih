/* libnih
 *
 * test_error.c - test suite for nih/error.c
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
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

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <nih/alloc.h>
#include <nih/error.h>
#include <nih/logging.h>


int
test_raise (void)
{
	NihError *error;
	int       ret = 0;

	printf ("Testing nih_error_raise()\n");
	nih_error_raise (0x20001, "Test error");
	error = nih_error_get ();

	/* Error number should be what we gave */
	if (error->number != 0x20001) {
		printf ("BAD: error number incorrect.\n");
		ret = 1;
	}

	/* Error message should be what we gave */
	if (strcmp (error->message, "Test error")) {
		printf ("BAD: error message incorrect.\n");
		ret = 1;
	}

	nih_free (error);

	return ret;
}

int
test_raise_printf (void)
{
	NihError *error;
	int       ret = 0;

	printf ("Testing nih_error_raise_printf()\n");
	nih_error_raise_printf (0x20002, "This is a %s error %d", "test", 123);
	error = nih_error_get ();

	/* Error number should be what we gave */
	if (error->number != 0x20002) {
		printf ("BAD: error number incorrect.\n");
		ret = 1;
	}

	/* Error message should be formatted */
	if (strcmp (error->message, "This is a test error 123")) {
		printf ("BAD: error message incorrect.\n");
		ret = 1;
	}

	/* Error message should be child of error */
	if (nih_alloc_parent (error->message) != error) {
		printf ("BAD: error message parent incorrect.\n");
		ret = 1;
	}

	nih_free (error);

	return ret;
}

int
test_raise_system (void)
{
	NihError *error;
	int       ret = 0;

	printf ("Testing nih_error_raise_system()\n");
	errno = ENOENT;
	nih_error_raise_system ();
	error = nih_error_get ();

	/* Error number should be the original errno */
	if (error->number != ENOENT) {
		printf ("BAD: error number incorrect.\n");
		ret = 1;
	}

	/* Error message should be result of strerror */
	if (strcmp (error->message, strerror (ENOENT))) {
		printf ("BAD: error message incorrect.\n");
		ret = 1;
	}

	/* Error message should be child of error */
	if (nih_alloc_parent (error->message) != error) {
		printf ("BAD: error message parent incorrect.\n");
		ret = 1;
	}

	nih_free (error);

	return ret;
}

static int was_logged;
static int was_destroyed;

static int
logger_called (NihLogLevel  priority,
	       const char  *message)
{
	was_logged++;

	return 0;
}

static int
destructor_called (void *ptr)
{
	was_destroyed++;

	return 2;
}

int
test_raise_again (void)
{
	NihError *error1, *error2, *error3;
	int       ret = 0;

	printf ("Testing nih_error_raise_again()\n");

	printf ("...with no current error\n");
	error1 = nih_new (NULL, NihError);
	error1->number = ENOENT;
	error1->message = strerror (ENOENT);
	nih_error_raise_again (error1);
	error2 = nih_error_get ();

	/* Error returned should be error raised */
	if (error2 != error1) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}


	printf ("...with current error\n");
	was_destroyed = 0;
	nih_alloc_set_destructor (error1, destructor_called);
	nih_error_raise_again (error1);

	error2 = nih_new (NULL, NihError);
	error2->number = ENODEV;
	error2->message = strerror (ENODEV);

	was_logged = 0;
	nih_log_set_priority (NIH_LOG_WARN);
	nih_log_set_logger (logger_called);

	nih_error_raise_again (error2);
	error3 = nih_error_get ();

	/* Error returned should be new error */
	if (error3 != error2) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}

	/* A log message should have been emitted */
	if (! was_logged) {
		printf ("BAD: logger not called.\n");
		ret = 1;
	}

	/* Original error should have been freed */
	if (! was_destroyed) {
		printf ("BAD: original error was not freed.\n");
		ret = 1;
	}

	nih_free (error3);

	nih_log_set_logger (nih_logger_printf);

	return ret;
}

static int
call_return_error (int         retval,
		   int         number,
		   const char *message)
{
	nih_return_error (retval, number, message);
}

int
test_return_error (void)
{
	NihError *error;
	int       ret = 0, retval;

	printf ("Testing nih_return_error()\n");
	retval = call_return_error (-1, 0x20001, "Test error");
	error = nih_error_get ();

	/* Return value should be first argument */
	if (retval != -1) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}

	/* Error number should be what we gave */
	if (error->number != 0x20001) {
		printf ("BAD: error number incorrect.\n");
		ret = 1;
	}

	/* Error message should be what we gave */
	if (strcmp (error->message, "Test error")) {
		printf ("BAD: error message incorrect.\n");
		ret = 1;
	}

	nih_free (error);

	return ret;
}

static int
call_return_system_error (int retval)
{
	nih_return_system_error (retval);
}

int
test_return_system_error (void)
{
	NihError *error;
	int       ret = 0, retval;

	printf ("Testing nih_return_system_error()\n");
	errno = ENOENT;
	retval = call_return_system_error (-1);
	error = nih_error_get ();

	/* Return value should be first argument */
	if (retval != -1) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}

	/* Error number should be what we set */
	if (error->number != ENOENT) {
		printf ("BAD: error number incorrect.\n");
		ret = 1;
	}

	/* Error message should be from strerror */
	if (strcmp (error->message, "No such file or directory")) {
		printf ("BAD: error message incorrect.\n");
		ret = 1;
	}

	nih_free (error);

	return ret;
}


int
test_push_context (void)
{
	NihError *error;
	int       ret = 0;

	printf ("Testing nih_error_push_context()\n");
	nih_error_raise (0x20003, "Error in default context");
	nih_error_push_context ();
	nih_error_raise (0x20004, "Error in new context");
	error = nih_error_get ();

	/* Error returned should be from new context */
	if (error->number != 0x20004) {
		printf ("BAD: incorrect error returned.\n");
		ret = 1;
	}

	nih_free (error);

	/* Should be able to retrieve original error after pop */
	nih_error_pop_context ();
	error = nih_error_get ();

	if (error->number != 0x20003) {
		printf ("BAD: incorrect error returned.\n");
		ret = 1;
	}

	nih_free (error);

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_raise ();
	ret |= test_raise_printf ();
	ret |= test_raise_system ();
	ret |= test_raise_again ();
	ret |= test_return_error ();
	ret |= test_return_system_error ();
	ret |= test_push_context();

	return ret;
}
