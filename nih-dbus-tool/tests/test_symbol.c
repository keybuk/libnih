/* nih-dbus-tool
 *
 * test_symbol.c - test suite for nih-dbus-tool/symbol.c
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

#include <nih/macros.h>
#include <nih/alloc.h>

#include "symbol.h"


void
test_valid (void)
{
	TEST_FUNCTION ("symbol_valid");

	/* Check that a typical symbol is valid. */
	TEST_FEATURE ("with typical symbol");
	TEST_TRUE (symbol_valid ("wibble"));


	/* Check that an symbol is not valid if it is has an
	 * initial period.
	 */
	TEST_FEATURE ("with initial period");
	TEST_FALSE (symbol_valid (".wibble"));


	/* Check that an symbol is not valid if it ends with a period
	 */
	TEST_FEATURE ("with final period");
	TEST_FALSE (symbol_valid ("wibble."));


	/* Check that an symbol is not valid if it contains a period
	 */
	TEST_FEATURE ("with period");
	TEST_FALSE (symbol_valid ("wib.ble"));


	/* Check that a symbol may contain numbers */
	TEST_FEATURE ("with numbers");
	TEST_TRUE (symbol_valid ("wib43ble"));


	/* Check that a symbol may not begin with numbers */
	TEST_FEATURE ("with leading digits");
	TEST_FALSE (symbol_valid ("43wibble"));


	/* Check that a symbol may end with numbers */
	TEST_FEATURE ("with trailing digits");
	TEST_TRUE (symbol_valid ("wibble43"));


	/* Check that a symbol may contain underscores */
	TEST_FEATURE ("with underscore");
	TEST_TRUE (symbol_valid ("wib_ble"));


	/* Check that a symbol may begin with underscores */
	TEST_FEATURE ("with initial underscore");
	TEST_TRUE (symbol_valid ("_wibble"));


	/* Check that a symbol may end with underscores */
	TEST_FEATURE ("with final underscore");
	TEST_TRUE (symbol_valid ("wibble_"));


	/* Check that other characters are not permitted */
	TEST_FEATURE ("with non-permitted characters");
	TEST_FALSE (symbol_valid ("wib-ble"));


	/* Check that an empty symbol is invalid */
	TEST_FEATURE ("with empty string");
	TEST_FALSE (symbol_valid (""));
}


void
test_from_name (void)
{
	char *str;

	TEST_FUNCTION ("symbol_from_name");

	/* Check that a CamelCase name is converted to lowercase with an
	 * underscore between the words.
	 */
	TEST_FEATURE ("with basic CamelCase name");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "CamelCase");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "camel_case");

		nih_free (str);
	}


	/* Check that a longer CamelCase name is converted to lowercase with
	 * an underscore between the words.
	 */
	TEST_FEATURE ("with longer CamelCase name");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "CamelCaseName");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "camel_case_name");

		nih_free (str);
	}


	/* Check that a single title-case word is converted to lowercase.
	 */
	TEST_FEATURE ("with Title case word");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "Title");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "title");

		nih_free (str);
	}


	/* Check that name made up up Title case words separated by
	 * underscores is converted to lowercase but additional underscores
	 * are not added.
	 */
	TEST_FEATURE ("with Title case name and undescores");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "Title_Case");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "title_case");

		nih_free (str);
	}


	/* Check that a longer name made up up Title case words separated by
	 * underscores is converted to lowercase but additional underscores
	 * are not added.
	 */
	TEST_FEATURE ("with longer Title case name and undescores");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "Title_Case_Name");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "title_case_name");

		nih_free (str);
	}


	/* Check that an underscore is not inserted between consecutive
	 * capital letters.
	 */
	TEST_FEATURE ("with consecutive capital letters");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "DBusTest");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "dbus_test");

		nih_free (str);
	}


	/* Check that a lowercase word is left alone.
	 */
	TEST_FEATURE ("with lowercase word");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "lower");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "lower");

		nih_free (str);
	}


	/* Check that a lowercase name with words separated by underscores
	 * is left alone.
	 */
	TEST_FEATURE ("with lowercase name and underscores");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "lower_case");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "lower_case");

		nih_free (str);
	}


	/* Check that a longer lowercase name with words separated by
	 * underscores is left alone.
	 */
	TEST_FEATURE ("with longer lowercase name and underscores");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "lower_case_name");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "lower_case_name");

		nih_free (str);
	}


	/* Check that digits after lowercase characters are not separated.
	 */
	TEST_FEATURE ("with digits after lowercase");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "lower69");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "lower69");

		nih_free (str);
	}


	/* Check that digits between lowercase characters are not separated.
	 */
	TEST_FEATURE ("with digits between lowercase");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "lower69th");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "lower69th");

		nih_free (str);
	}


	/* Check that digits after uppercase characters are not separated.
	 */
	TEST_FEATURE ("with digits after uppercase");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "X5");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "x5");

		nih_free (str);
	}


	/* Check that digits before characters are separated.
	 */
	TEST_FEATURE ("with digits before uppercase");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "Platform5B");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "platform5_b");

		nih_free (str);
	}


	/* Check a pathological example.
	 */
	TEST_FEATURE ("with pathological example");
	TEST_ALLOC_FAIL {
		str = symbol_from_name (NULL, "CamelCase_but_2nd_Title_Case_bit");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "camel_case_but_2nd_title_case_bit");

		nih_free (str);
	}
}


int
main (int   argc,
      char *argv[])
{
	test_valid ();
	test_from_name ();

	return 0;
}
