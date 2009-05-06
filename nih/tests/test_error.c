/* libnih
 *
 * test_error.c - test suite for nih/error.c
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

#include <nih/test.h>

#include <errno.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/error.h>
#include <nih/logging.h>


void
test_raise (void)
{
	NihError *error;

	/* Check that after raising an error, we can get it again, and that
	 * the number and message are what we gave.
	 */
	TEST_FUNCTION ("nih_error_raise");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		nih_error_raise (0x20001, "Test error");
		error = nih_error_get ();

		TEST_EQ (error->number, 0x20001);
		TEST_EQ_STR (error->message, "Test error");

		nih_free (error);
	}
	nih_error_pop_context ();
}

void
test_raise_printf (void)
{
	NihError *error;

	/* Check that we can raise an error with a formatted string, and
	 * that when we get it, the message is formatted appropriately and
	 * that the string is a child of the error object.
	 */
	TEST_FUNCTION ("nih_error_raise_printf");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		nih_error_raise_printf (0x20002, "This is a %s error %d",
					"test", 123);
		error = nih_error_get ();

		TEST_EQ (error->number, 0x20002);
		TEST_EQ_STR (error->message, "This is a test error 123");
		TEST_ALLOC_PARENT (error->message, error);

		nih_free (error);
	}
	nih_error_pop_context ();
}

void
test_raise_system (void)
{
	NihError *error;

	/* Check that we can raise a system error, which takes the number and
	 * message from the errno table.
	 */
	TEST_FUNCTION ("nih_error_raise_system");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		errno = ENOENT;
		nih_error_raise_system ();
		error = nih_error_get ();

		TEST_EQ (error->number, ENOENT);
		TEST_EQ_STR (error->message, strerror (ENOENT));
		TEST_ALLOC_PARENT (error->message, error);

		nih_free (error);
	}
	nih_error_pop_context ();
}


static int was_logged;

static int
logger_called (NihLogLevel  priority,
	       const char  *message)
{
	was_logged++;

	return 0;
}

void
test_raise_error (void)
{
	NihError *error1, *error2, *error3;

	TEST_FUNCTION ("nih_error_raise_error");

	/* Check that we can raise an arbitrary error object, and that we
	 * get the exact pointer we raised.
	 */
	TEST_FEATURE ("with no current error");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			error1 = nih_new (NULL, NihError);
			error1->number = ENOENT;
			error1->message = strerror (ENOENT);
		}

		nih_error_raise_error (error1);
		error2 = nih_error_get ();

		TEST_EQ_P (error2, error1);

		nih_free (error1);
	}
	nih_error_pop_context ();


	/* Check that an error raised while there's already an unhandled
	 * error causes an error message to be logged through the usual
	 * mechanism and the unhandled error to be destroyed.  The error
	 * returned should be the new one.
	 */
	TEST_FEATURE ("with unhandled error");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			error1 = nih_new (NULL, NihError);
			error1->number = ENOENT;
			error1->message = strerror (ENOENT);
		}

		TEST_FREE_TAG (error1);
		nih_error_raise_error (error1);

		TEST_ALLOC_SAFE {
			error2 = nih_new (NULL, NihError);
			error2->number = ENODEV;
			error2->message = strerror (ENODEV);
		}

		was_logged = 0;
		nih_log_set_priority (NIH_LOG_MESSAGE);
		nih_log_set_logger (logger_called);

		nih_error_raise_error (error2);
		error3 = nih_error_get ();

		TEST_EQ_P (error3, error2);
		TEST_TRUE (was_logged);
		TEST_FREE (error1);

		nih_free (error2);

		nih_log_set_logger (nih_logger_printf);
	}
	nih_error_pop_context ();
}


static int
call_return_error (int         ret,
		   int         number,
		   const char *message)
{
	nih_return_error (ret, number, message);
	return 254;
}

void
test_return_error (void)
{
	NihError *error;
	int       ret;

	/* Check that the macro to raise an error and return from a
	 * function does just that.
	 */
	TEST_FUNCTION ("nih_return_error");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		ret = call_return_error (-1, 0x20001, "Test error");
		error = nih_error_get ();

		TEST_EQ (ret, -1);
		TEST_EQ (error->number, 0x20001);
		TEST_EQ_STR (error->message, "Test error");

		nih_free (error);
	}
	nih_error_pop_context ();
}


static int
call_return_system_error (int ret)
{
	nih_return_system_error (ret);
}

void
test_return_system_error (void)
{
	NihError *error;
	int       ret;

	/* Check that the macro to raise an error based on the value of
	 * errno and return from a function does just that.
	 */
	TEST_FUNCTION ("nih_return_system_error");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		errno = ENOENT;
		ret = call_return_system_error (-1);
		error = nih_error_get ();

		TEST_EQ (ret, -1);
		TEST_EQ (error->number, ENOENT);
		TEST_EQ_STR (error->message, strerror (ENOENT));

		nih_free (error);
	}
	nih_error_pop_context ();
}

static int
call_return_no_memory_error (int ret)
{
	nih_return_no_memory_error (ret);
}

void
test_return_no_memory_error (void)
{
	NihError *error;
	int       ret;

	/* Check that the macro to raise an ENOMEM error return from a
	 * function does just that without modifying errno.
	 */
	TEST_FUNCTION ("nih_return_no_memory_error");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		errno = ENOENT;
		ret = call_return_no_memory_error (-1);
		error = nih_error_get ();

		TEST_EQ (ret, -1);
		TEST_EQ (error->number, ENOMEM);
		TEST_EQ_STR (error->message, strerror (ENOMEM));

		if (! test_alloc_failed)
			TEST_EQ (errno, ENOENT);

		nih_free (error);
	}
	nih_error_pop_context ();
}


void
test_push_context (void)
{
	NihError *error;

	/* Check that we can push an error context over the top of a
	 * handled error, and that if we try and raise then get an error
	 * afterwards, we get the newer one.
	 */
	TEST_FUNCTION ("nih_error_push_context");
	TEST_ALLOC_FAIL {
		nih_error_raise (0x20003, "Error in default context");
		nih_error_push_context ();
		nih_error_raise (0x20004, "Error in new context");
		error = nih_error_get ();

		TEST_EQ (error->number, 0x20004);

		nih_free (error);

		nih_error_pop_context ();
		nih_free (nih_error_get ());
	}
}

void
test_pop_context (void)
{
	NihError *error;

	TEST_FUNCTION ("nih_error_pop_context");
	nih_error_raise (0x20003, "Error in default context");

	/* Check that we can pop the error context; when doing so, if an
	 * unhandled error exists, an error is logged through the usual
	 * mechanism and the error destroyed.
	 */
	TEST_FEATURE ("with unhandled error in context");
	nih_error_push_context ();

	nih_error_raise (0x20004, "Error in new context");
	error = nih_error_get ();

	TEST_FREE_TAG (error);
	nih_error_raise_error (error);

	was_logged = 0;
	nih_log_set_priority (NIH_LOG_MESSAGE);
	nih_log_set_logger (logger_called);

	nih_error_pop_context ();

	TEST_TRUE (was_logged);
	TEST_FREE (error);

	nih_log_set_logger (nih_logger_printf);


	/* Check that once popped, any unhandled error in lower contexts
	 * is available again.
	 */
	TEST_FEATURE ("with unhandler error beneath context");
	error = nih_error_get ();

	TEST_EQ (error->number, 0x20003);

	nih_free (error);
}


int
main (int   argc,
      char *argv[])
{
	test_raise ();
	test_raise_printf ();
	test_raise_system ();
	test_raise_error ();
	test_return_error ();
	test_return_system_error ();
	test_return_no_memory_error ();
	test_push_context ();
	test_pop_context ();

	return 0;
}
