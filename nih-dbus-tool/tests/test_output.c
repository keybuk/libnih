/* nih-dbus-tool
 *
 * test_output.c - test suite for nih-dbus-tool/output.c
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

#include <stdio.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/main.h>
#include <nih/error.h>

#include "method.h"
#include "signal.h"
#include "property.h"
#include "node.h"
#include "output.h"


void
test_output (void)
{
	FILE *        source;
	FILE *        header;
	Node *        node = NULL;
	Interface *   interface = NULL;
	Method *      method = NULL;
	Signal *      signal = NULL;
	Argument *    argument = NULL;
	Property *    property = NULL;
	int           ret;
	NihError *    err;

	TEST_FUNCTION ("output");
	source = tmpfile ();
	header = tmpfile ();


	/* Check that we can generate a valid source file and accompanying
	 * header file for a node in proxy mode.
	 */
	TEST_FEATURE ("with proxy");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

			method = method_new (interface, "Poke");
			method->symbol = "poke";
			nih_list_add (&interface->methods, &method->entry);

			argument = argument_new (method, "address",
						 "u", NIH_DBUS_ARG_IN);
			argument->symbol = "address";
			nih_list_add (&method->arguments, &argument->entry);

			argument = argument_new (method, "value",
						 "s", NIH_DBUS_ARG_IN);
			argument->symbol = "value";
			nih_list_add (&method->arguments, &argument->entry);

			method = method_new (interface, "Peek");
			method->symbol = "peek";
			nih_list_add (&interface->methods, &method->entry);

			argument = argument_new (method, "address",
						 "u", NIH_DBUS_ARG_IN);
			argument->symbol = "address";
			nih_list_add (&method->arguments, &argument->entry);

			argument = argument_new (method, "value",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = "value";
			nih_list_add (&method->arguments, &argument->entry);

			method = method_new (interface, "IsValidAddress");
			method->symbol = "is_valid_address";
			nih_list_add (&interface->methods, &method->entry);

			argument = argument_new (method, "address",
						 "u", NIH_DBUS_ARG_IN);
			argument->symbol = "address";
			nih_list_add (&method->arguments, &argument->entry);


			signal = signal_new (interface, "Bounce");
			signal->symbol = "bounce";
			nih_list_add (&interface->signals, &signal->entry);

			argument = argument_new (signal, "height",
						 "u", NIH_DBUS_ARG_OUT);
			argument->symbol = "height";
			nih_list_add (&signal->arguments, &argument->entry);

			argument = argument_new (signal, "velocity",
						 "i", NIH_DBUS_ARG_OUT);
			argument->symbol = "velocity";
			nih_list_add (&signal->arguments, &argument->entry);

			signal = signal_new (interface, "Exploded");
			signal->symbol = "exploded";
			nih_list_add (&interface->signals, &signal->entry);


			property = property_new (interface, "colour",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = "colour";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "size",
						 "u", NIH_DBUS_READ);
			property->symbol = "size";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "touch",
						 "b", NIH_DBUS_WRITE);
			property->symbol = "touch";
			nih_list_add (&interface->properties, &property->entry);


			interface = interface_new (node, "com.netsplit.Nih.Foo");
			interface->symbol = "foo";
			nih_list_add (&node->interfaces, &interface->entry);

			method = method_new (interface, "Bing");
			method->symbol = "bing";
			nih_list_add (&interface->methods, &method->entry);

			signal = signal_new (interface, "NewResult");
			signal->symbol = "new_result";
			nih_list_add (&interface->signals, &signal->entry);

			property = property_new (interface, "preferences",
						 "(us)", NIH_DBUS_READWRITE);
			property->symbol = "preferences";
			nih_list_add (&interface->properties, &property->entry);

		}

		ret = output ("test.c", fileno (source),
			      "test.h", fileno (header),
			      "my", node, FALSE);

		rewind (source);
		rewind (header);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (source);
			TEST_FILE_RESET (header);

			nih_free (node);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EXPECTED_FILE (source, "test_output_proxy_standard.c");
		TEST_EXPECTED_FILE (header, "test_output_proxy_standard.h");

		nih_free (node);
	}


	/* Check that when there are no interfaces, a valid empty source
	 * and header file are generated.
	 */
	TEST_FEATURE ("with proxy but no interfaces");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);
		}

		ret = output ("test.c", fileno (source),
			      "test.h", fileno (header),
			      "my", node, FALSE);

		rewind (source);
		rewind (header);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (source);
			TEST_FILE_RESET (header);

			nih_free (node);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EXPECTED_FILE (source, "test_output_proxy_no_interfaces.c");
		TEST_EXPECTED_FILE (header, "test_output_proxy_no_interfaces.h");

		nih_free (node);
	}


	/* Check that we can generate a valid source file and accompanying
	 * header file for a node in object mode.
	 */
	TEST_FEATURE ("with object");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

			method = method_new (interface, "Poke");
			method->symbol = "poke";
			nih_list_add (&interface->methods, &method->entry);

			argument = argument_new (method, "address",
						 "u", NIH_DBUS_ARG_IN);
			argument->symbol = "address";
			nih_list_add (&method->arguments, &argument->entry);

			argument = argument_new (method, "value",
						 "s", NIH_DBUS_ARG_IN);
			argument->symbol = "value";
			nih_list_add (&method->arguments, &argument->entry);

			method = method_new (interface, "Peek");
			method->symbol = "peek";
			method->async = TRUE;
			nih_list_add (&interface->methods, &method->entry);

			argument = argument_new (method, "address",
						 "u", NIH_DBUS_ARG_IN);
			argument->symbol = "address";
			nih_list_add (&method->arguments, &argument->entry);

			argument = argument_new (method, "value",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = "value";
			nih_list_add (&method->arguments, &argument->entry);

			method = method_new (interface, "IsValidAddress");
			method->symbol = "is_valid_address";
			nih_list_add (&interface->methods, &method->entry);

			argument = argument_new (method, "address",
						 "u", NIH_DBUS_ARG_IN);
			argument->symbol = "address";
			nih_list_add (&method->arguments, &argument->entry);

			argument = argument_new (method, "is_valid",
						 "b", NIH_DBUS_ARG_OUT);
			argument->symbol = "is_valid";
			nih_list_add (&method->arguments, &argument->entry);


			signal = signal_new (interface, "Bounce");
			signal->symbol = "bounce";
			nih_list_add (&interface->signals, &signal->entry);

			argument = argument_new (signal, "height",
						 "u", NIH_DBUS_ARG_OUT);
			argument->symbol = "height";
			nih_list_add (&signal->arguments, &argument->entry);

			argument = argument_new (signal, "velocity",
						 "i", NIH_DBUS_ARG_OUT);
			argument->symbol = "velocity";
			nih_list_add (&signal->arguments, &argument->entry);

			signal = signal_new (interface, "Exploded");
			signal->symbol = "exploded";
			nih_list_add (&interface->signals, &signal->entry);


			property = property_new (interface, "colour",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = "colour";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "size",
						 "u", NIH_DBUS_READ);
			property->symbol = "size";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "touch",
						 "b", NIH_DBUS_WRITE);
			property->symbol = "touch";
			nih_list_add (&interface->properties, &property->entry);


			interface = interface_new (node, "com.netsplit.Nih.Foo");
			interface->symbol = "foo";
			nih_list_add (&node->interfaces, &interface->entry);

			method = method_new (interface, "Bing");
			method->symbol = "bing";
			nih_list_add (&interface->methods, &method->entry);

			signal = signal_new (interface, "NewResult");
			signal->symbol = "new_result";
			nih_list_add (&interface->signals, &signal->entry);

			property = property_new (interface, "preferences",
						 "(us)", NIH_DBUS_READWRITE);
			property->symbol = "preferences";
			nih_list_add (&interface->properties, &property->entry);
		}

		ret = output ("test.c", fileno (source),
			      "test.h", fileno (header),
			      "my", node, TRUE);

		rewind (source);
		rewind (header);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (source);
			TEST_FILE_RESET (header);

			nih_free (node);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EXPECTED_FILE (source, "test_output_object_standard.c");
		TEST_EXPECTED_FILE (header, "test_output_object_standard.h");

		nih_free (node);
	}


	/* Check that when there are no interfaces, a valid empty source
	 * and header file are generated.
	 */
	TEST_FEATURE ("with object but no interfaces");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);
		}

		ret = output ("test.c", fileno (source),
			      "test.h", fileno (header),
			      "my", node, TRUE);

		rewind (source);
		rewind (header);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (source);
			TEST_FILE_RESET (header);

			nih_free (node);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EXPECTED_FILE (source, "test_output_object_no_interfaces.c");
		TEST_EXPECTED_FILE (header, "test_output_object_no_interfaces.h");

		nih_free (node);
	}


	fclose (source);
	fclose (header);
}


void
test_preamble (void)
{
	char *str;

	TEST_FUNCTION ("output_preamble");

	/* Check that a preamble for a source file is correctly generated,
	 * with the package name, source file path and copyright all
	 * present.
	 */
	TEST_FEATURE ("with path");
	TEST_ALLOC_FAIL {
		str = output_preamble (NULL, "path.c");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, ("/* test\n"
				   " *\n"
				   " * path.c - auto-generated D-Bus bindings\n"
				   " *\n"
				   " * Copyright (C) 2009 Joe Bloggs.\n"
				   " *\n"
				   " * This file was automatically generated; see the source for copying\n"
				   " * conditions.\n"
				   " */\n"
				   "\n"));

		nih_free (str);
	}


	/* Check that a preamble for a header file (NULL path) is correctly
	 * generated with the package name and copyright present.
	 */
	TEST_FEATURE ("with no path");
	TEST_ALLOC_FAIL {
		str = output_preamble (NULL, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, ("/* test\n"
				   " *\n"
				   " * Copyright (C) 2009 Joe Bloggs.\n"
				   " *\n"
				   " * This file was automatically generated; see the source for copying\n"
				   " * conditions.\n"
				   " */\n"
				   "\n"));

		nih_free (str);
	}
}


void
test_sentinel (void)
{
	char *str;

	TEST_FUNCTION ("output_sentinel");

	/* Check that a header file sentinel is correctly generated for a
	 * local path.
	 */
	TEST_FEATURE ("with local path");
	TEST_ALLOC_FAIL {
		str = output_sentinel (NULL, "foo.h");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "TEST_FOO_H");

		nih_free (str);
	}


	/* Check that a header file sentinel is correctly generated for a
	 * sub-directory path.
	 */
	TEST_FEATURE ("with sub-directory path");
	TEST_ALLOC_FAIL {
		str = output_sentinel (NULL, "path/to/foo.h");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "TEST_PATH_TO_FOO_H");

		nih_free (str);
	}


	/* Check that a header file sentinel is generated for an absolute
	 * path; we might want to change the format of this later, but it's
	 * ok for now.
	 */
	TEST_FEATURE ("with absolute path");
	TEST_ALLOC_FAIL {
		str = output_sentinel (NULL, "/path/to/foo.h");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "TEST__PATH_TO_FOO_H");

		nih_free (str);
	}
}


int
main (int   argc,
      char *argv[])
{
	package_name = "test";
	package_copyright = "Copyright (C) 2009 Joe Bloggs.";
	program_name = "test";
	nih_error_init ();

	test_output ();
	test_preamble ();
	test_sentinel ();

	return 0;
}
