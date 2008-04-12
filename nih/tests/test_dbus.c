/* libnih
 *
 * test_dbus.c - test suite for nih/dbus.c
 *
 * Copyright © 2008 Scott James Remnant <scott@netsplit.com>.
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

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/error.h>
#include <nih/errors.h>

#include <nih/dbus.h>


void
test_error_raise (void)
{
	NihError     *error;
	NihDBusError *err;

	/* Make sure that an NIH_DBUS_ERROR is raised with the name and
	 * message we give.
	 */
	TEST_FUNCTION ("nih_dbus_error_raise");
	TEST_ALLOC_SAFE {
		nih_dbus_error_raise ("foo", "bar");
		error = nih_error_get ();

		TEST_ALLOC_PARENT (error, NULL);
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


int
main (int   argc,
      char *argv[])
{
	test_error_raise ();

	return 0;
}
