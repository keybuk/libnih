/* libnih
 *
 * test_dbus_util.c - test suite for nih-dbus/dbus_util.c
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

#include <nih-dbus/dbus_util.h>


void
test_path (void)
{
	char *path;

	TEST_FUNCTION ("nih_dbus_path");

	/* Check that a root path with no additional elements is simply
	 * returned duplicated, the root should not be escaped.
	 */
	TEST_FEATURE ("with root only");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, "/com/netsplit/Nih");

		nih_free (path);
	}


	/* Check that a root path with a single additional element has that
	 * appended separated by a slash.
	 */
	TEST_FEATURE ("with single additional element");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih", "test", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, "/com/netsplit/Nih/test");

		nih_free (path);
	}


	/* Check that a root path with multiple additional elements have them
	 * appended separated by slashes.
	 */
	TEST_FEATURE ("with multiple additional elements");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih",
				      "test", "frodo", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, "/com/netsplit/Nih/test/frodo");

		nih_free (path);
	}


	/* Check that if one of the additional elements requires escaping,
	 * it is appended in the escaped form.
	 */
	TEST_FEATURE ("with element requiring escaping");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih",
				      "test", "foo/bar.baz", "frodo", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, ("/com/netsplit/Nih"
				    "/test/foo_2fbar_2ebaz/frodo"));

		nih_free (path);
	}


	/* Check that when multiple elements require escaping, they are
	 * all escaped; also check that an underscore requires escaping to
	 * ensure path uniqueness.
	 */
	TEST_FEATURE ("with multiple elements requiring escaping");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih",
				      "test_thing", "foo/bar.baz",
				      "frodo", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, ("/com/netsplit/Nih"
				    "/test_5fthing/foo_2fbar_2ebaz/frodo"));

		nih_free (path);
	}


	/* Check that if one of the additional elements is empty, it
	 * is replaced with an underscore.
	 */
	TEST_FEATURE ("with empty element");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih", "", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, "/com/netsplit/Nih/_");

		nih_free (path);
	}
}


int
main (int   argc,
      char *argv[])
{
	test_path ();

	return 0;
}
