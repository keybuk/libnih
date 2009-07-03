/* libnih
 *
 * test_dbus_error.c - test suite for nih-dbus/dbus_error.c
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
#include <nih-dbus/test_dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>
#include <nih-dbus/errors.h>


void
test_error_raise (void)
{
	NihError *    err;
	NihDBusError *dbus_err;

	/* Make sure that an NIH_DBUS_ERROR is raised with the name and
	 * message we give.
	 */
	TEST_FUNCTION ("nih_dbus_error_raise");
	TEST_ALLOC_FAIL {
		nih_dbus_error_raise ("foo", "bar");
		err = nih_error_get ();

		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		TEST_EQ (err->number, NIH_DBUS_ERROR);

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "foo");
		TEST_ALLOC_PARENT (dbus_err->name, dbus_err);
		TEST_EQ_STR (dbus_err->message, "bar");
		TEST_ALLOC_PARENT (dbus_err->message, dbus_err);

		nih_free (dbus_err);
	}
}

void
test_error_raise_printf (void)
{
	NihError *    err;
	NihDBusError *dbus_err;

	/* Make sure that an NIH_DBUS_ERROR is raised with the name and
	 * formatted message we give.
	 */
	TEST_FUNCTION ("nih_dbus_error_raise_printf");
	TEST_ALLOC_FAIL {
		nih_dbus_error_raise_printf ("foo", "hello %d this is a %s",
					     123, "test");
		err = nih_error_get ();

		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		TEST_EQ (err->number, NIH_DBUS_ERROR);

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "foo");
		TEST_ALLOC_PARENT (dbus_err->name, dbus_err);
		TEST_EQ_STR (dbus_err->message, "hello 123 this is a test");
		TEST_ALLOC_PARENT (dbus_err->message, dbus_err);

		nih_free (dbus_err);
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
