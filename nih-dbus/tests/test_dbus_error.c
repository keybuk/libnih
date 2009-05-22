/* libnih
 *
 * test_dbus_error.c - test suite for nih-dbus/dbus_error.c
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

#include <nih/test.h>
#include <nih-dbus/test_dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>
#include <nih-dbus/errors.h>


void
test_error_raise (void)
{
	NihError *    error;
	NihDBusError *err;

	/* Make sure that an NIH_DBUS_ERROR is raised with the name and
	 * message we give.
	 */
	TEST_FUNCTION ("nih_dbus_error_raise");
	TEST_ALLOC_FAIL {
		nih_dbus_error_raise ("foo", "bar");
		error = nih_error_get ();

		TEST_ALLOC_SIZE (error, sizeof (NihDBusError));
		TEST_EQ (error->number, NIH_DBUS_ERROR);

		err = (NihDBusError *)error;
		TEST_EQ_STR (err->name, "foo");
		TEST_ALLOC_PARENT (err->name, err);
		TEST_EQ_STR (err->error.message, "bar");
		TEST_ALLOC_PARENT (err->error.message, err);

		nih_free (error);
	}
}

void
test_error_raise_printf (void)
{
	NihError *    error;
	NihDBusError *err;

	/* Make sure that an NIH_DBUS_ERROR is raised with the name and
	 * formatted message we give.
	 */
	TEST_FUNCTION ("nih_dbus_error_raise_printf");
	TEST_ALLOC_FAIL {
		nih_dbus_error_raise_printf ("foo", "hello %d this is a %s",
					     123, "test");
		error = nih_error_get ();

		TEST_ALLOC_SIZE (error, sizeof (NihDBusError));
		TEST_EQ (error->number, NIH_DBUS_ERROR);

		err = (NihDBusError *)error;
		TEST_EQ_STR (err->name, "foo");
		TEST_ALLOC_PARENT (err->name, err);
		TEST_EQ_STR (err->error.message, "hello 123 this is a test");
		TEST_ALLOC_PARENT (err->error.message, err);

		nih_free (error);
	}
}


int
main (int   argc,
      char *argv[])
{
	nih_error_init ();

	test_error_raise ();
	test_error_raise_printf ();

	return 0;
}
