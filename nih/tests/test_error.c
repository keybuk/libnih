/* libnih
 *
 * test_error.c - test suite for nih/error.c
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

#include <nih/test.h>

#include <sys/wait.h>

#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/main.h>
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

void
test_raise_no_memory (void)
{
	NihError *error;

	/* Check that we can raise a no memory error.
	 */
	TEST_FUNCTION ("nih_error_raise_no_memory");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		nih_error_raise_no_memory ();
		error = nih_error_get ();

		TEST_EQ (error->number, ENOMEM);
		TEST_EQ_STR (error->message, strerror (ENOMEM));

		nih_free (error);
	}
	nih_error_pop_context ();
}


void
test_raise_error (void)
{
	NihError *error1 = NULL;
	NihError *error2 = NULL;
	pid_t     pid = 0;
	int       status;
	FILE *    output;
	char      corefile[PATH_MAX + 1];

	TEST_FUNCTION ("nih_error_raise_error");
	output = tmpfile ();


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
	 * error causes an assertion.
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

		TEST_DIVERT_STDERR (output) {
			TEST_CHILD (pid) {
				nih_error_raise_error (error2);
				exit (0);
			}
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFSIGNALED (status));
		TEST_EQ (WTERMSIG (status), SIGABRT);

		rewind (output);

		TEST_FILE_MATCH (output, ("test:*tests/test_error.c:[0-9]*: "
					  "Unhandled error from test_raise_error: "
					  "No such file or directory\n"));
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);

		unlink ("core");

		sprintf (corefile, "core.%d", pid);
		unlink (corefile);

		sprintf (corefile, "vgcore.%d", pid);
		unlink (corefile);

		nih_free (error1);
		nih_free (error2);
	}
	nih_error_pop_context ();


	fclose (output);
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
test_steal (void)
{
	NihError *error1;
	NihError *error2;

	TEST_FUNCTION ("nih_error_steal");


	/* Check that after raising an error, we can steal it, and raise
	 * another error in its place without freeing the original error.
	 */
	TEST_FEATURE ("with same context");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		nih_error_raise (0x20001, "Test error");
		error1 = nih_error_steal ();

		TEST_EQ (error1->number, 0x20001);
		TEST_EQ_STR (error1->message, "Test error");

		TEST_FREE_TAG (error1);

		nih_error_raise (0x20002, "Different error");
		error2 = nih_error_get ();

		TEST_NE_P (error2, error1);
		TEST_NOT_FREE (error1);

		TEST_EQ (error2->number, 0x20002);
		TEST_EQ_STR (error2->message, "Different error");

		nih_free (error2);
		nih_free (error1);
	}
	nih_error_pop_context ();


	/* Check that nih_error_steal() can be used to raise an error from
	 * one context into another.
	 */
	TEST_FEATURE ("with different contexts");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		nih_error_push_context ();
		nih_error_raise (0x20001, "Test error");
		error1 = nih_error_steal ();

		TEST_EQ (error1->number, 0x20001);
		TEST_EQ_STR (error1->message, "Test error");

		TEST_FREE_TAG (error1);

		nih_error_pop_context ();

		TEST_NOT_FREE (error1);

		nih_error_raise_error (error1);

		error2 = nih_error_get ();

		TEST_EQ_P (error2, error1);
		TEST_NOT_FREE (error1);

		TEST_EQ (error2->number, 0x20001);
		TEST_EQ_STR (error2->message, "Test error");

		nih_free (error1);
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
	FILE *    output;
	pid_t     pid = 0;
	int       status;
	char      corefile[PATH_MAX + 1];

	TEST_FUNCTION ("nih_error_pop_context");
	output = tmpfile ();


	/* Check that we can pop the error context; when doing so, if an
	 * unhandled error exists then an assertion is raised.
	 */
	TEST_FEATURE ("with unhandled error in context");
	TEST_ALLOC_FAIL {
		TEST_DIVERT_STDERR (output) {
			TEST_CHILD (pid) {
				nih_error_push_context ();

				nih_error_raise (0x20004, "Error in new context");
				error = nih_error_get ();

				nih_error_pop_context ();
				exit (0);
			}
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFSIGNALED (status));
		TEST_EQ (WTERMSIG (status), SIGABRT);

		rewind (output);

		TEST_FILE_MATCH (output, ("test:*tests/test_error.c:[0-9]*: "
					  "Unhandled error from test_pop_context: "
					  "Error in new context\n"));
		TEST_FILE_END (output);

		TEST_FILE_RESET (output);

		unlink ("core");

		sprintf (corefile, "core.%d", pid);
		unlink (corefile);

		sprintf (corefile, "vgcore.%d", pid);
		unlink (corefile);
	}


	/* Check that once popped, any unhandled error in lower contexts
	 * is available again.
	 */
	TEST_FEATURE ("with unhandled error beneath context");
	TEST_ALLOC_FAIL {
		nih_error_raise (0x20003, "Error in default context");
		nih_error_push_context ();

		nih_error_raise (0x20004, "Error in new context");

		error = nih_error_get ();
		TEST_EQ (error->number, 0x20004);

		nih_free (error);

		nih_error_pop_context ();

		error = nih_error_get ();
		TEST_EQ (error->number, 0x20003);

		nih_free (error);
	}


	fclose (output);
}


int
main (int   argc,
      char *argv[])
{
	program_name = "test";
	nih_error_init ();

	test_raise ();
	test_raise_printf ();
	test_raise_system ();
	test_raise_no_memory ();
	test_raise_error ();
	test_return_error ();
	test_return_system_error ();
	test_return_no_memory_error ();
	test_steal ();
	test_push_context ();
	test_pop_context ();

	return 0;
}
