/* nih-dbus-tool
 *
 * test_argument.c - test suite for nih-dbus-tool/argument.c
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

#include <expat.h>

#include <errno.h>
#include <assert.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/main.h>
#include <nih/error.h>

#include "node.h"
#include "method.h"
#include "signal.h"
#include "argument.h"
#include "parse.h"
#include "errors.h"


void
test_name_valid (void)
{
	TEST_FUNCTION ("argument_name_valid");

	/* Check that a typical argument name is valid. */
	TEST_FEATURE ("with typical argument name");
	TEST_TRUE (argument_name_valid ("Wibble"));


	/* Check that an argument name is not valid if it is has an
	 * initial period.
	 */
	TEST_FEATURE ("with initial period");
	TEST_FALSE (argument_name_valid (".Wibble"));


	/* Check that an argument name is not valid if it ends with a period
	 */
	TEST_FEATURE ("with final period");
	TEST_FALSE (argument_name_valid ("Wibble."));


	/* Check that an argument name is not valid if it contains a period
	 */
	TEST_FEATURE ("with period");
	TEST_FALSE (argument_name_valid ("Wib.ble"));


	/* Check that a argument name may contain numbers */
	TEST_FEATURE ("with numbers");
	TEST_TRUE (argument_name_valid ("Wib43ble"));


	/* Check that a argument name may not begin with numbers */
	TEST_FEATURE ("with leading digits");
	TEST_FALSE (argument_name_valid ("43Wibble"));


	/* Check that a argument name may end with numbers */
	TEST_FEATURE ("with trailing digits");
	TEST_TRUE (argument_name_valid ("Wibble43"));


	/* Check that a argument name may contain underscores */
	TEST_FEATURE ("with underscore");
	TEST_TRUE (argument_name_valid ("Wib_ble"));


	/* Check that a argument name may begin with underscores */
	TEST_FEATURE ("with initial underscore");
	TEST_TRUE (argument_name_valid ("_Wibble"));


	/* Check that a argument name may end with underscores */
	TEST_FEATURE ("with final underscore");
	TEST_TRUE (argument_name_valid ("Wibble_"));


	/* Check that other characters are not permitted */
	TEST_FEATURE ("with non-permitted characters");
	TEST_FALSE (argument_name_valid ("Wib-ble"));


	/* Check that an empty argument name is invalid */
	TEST_FEATURE ("with empty string");
	TEST_FALSE (argument_name_valid (""));
}


void
test_new (void)
{
	Argument *argument;

	TEST_FUNCTION ("argument_new");

	/* Check that an Argument object is allocated with the structure
	 * filled in properly, but not placed in a list.
	 */
	TEST_FEATURE ("with name");
	TEST_ALLOC_FAIL {
		argument = argument_new (NULL, "name", "s", NIH_DBUS_ARG_IN);

		if (test_alloc_failed) {
			TEST_EQ_P (argument, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_LIST_EMPTY (&argument->entry);
		TEST_EQ_STR (argument->name, "name");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ_P (argument->symbol, NULL);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_free (argument);
	}


	/* Check that the name of an Argument can be left as NULL and that
	 * is stored in the structure.
	 */
	TEST_FEATURE ("without name");
	TEST_ALLOC_FAIL {
		argument = argument_new (NULL, NULL, "s", NIH_DBUS_ARG_OUT);

		if (test_alloc_failed) {
			TEST_EQ_P (argument, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_LIST_EMPTY (&argument->entry);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ_P (argument->symbol, NULL);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_free (argument);
	}
}


void
test_start_tag (void)
{
	ParseContext context;
	ParseStack * parent = NULL;
	ParseStack * entry;
	XML_Parser   xmlp;
	Node *       node = NULL;
	Method *     method = NULL;
	Signal *     signal = NULL;
	Argument *   argument;
	char *       attr[7];
	int          ret = 0;
	NihError *   err;
	FILE *       output;

	TEST_FUNCTION ("argument_start_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);

	output = tmpfile ();


	/* Check that an argument tag for a method with the usual name and
	 * type attributes results in an Argument member being created and
	 * pushed onto the stack with those attributes filled in correctly.
	 * Method argument direction should default to "in".
	 */
	TEST_FEATURE ("with method argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);
		}

		attr[0] = "name";
		attr[1] = "test_arg";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = NULL;

		ret = argument_start_tag (xmlp, "arg", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ARGUMENT);

		argument = entry->argument;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, entry);
		TEST_EQ_STR (argument->name, "test_arg");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ_P (argument->symbol, NULL);

		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		TEST_LIST_EMPTY (&method->arguments);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an argument tag for a signal with the usual name and
	 * type attributes results in an Argument member being created and
	 * pushed onto the stack with those attributes filled in correctly.
	 * Signal argument direction should default to "in".
	 */
	TEST_FEATURE ("with signal argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);
		}

		attr[0] = "name";
		attr[1] = "test_arg";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = NULL;

		ret = argument_start_tag (xmlp, "arg", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&signal->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ARGUMENT);

		argument = entry->argument;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, entry);
		TEST_EQ_STR (argument->name, "test_arg");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ_P (argument->symbol, NULL);

		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		TEST_LIST_EMPTY (&signal->arguments);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that direction of a method argument can be specified in
	 * an attribute as "in" (the default).
	 */
	TEST_FEATURE ("with method input argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);
		}

		attr[0] = "name";
		attr[1] = "test_arg";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = "direction";
		attr[5] = "in";
		attr[6] = NULL;

		ret = argument_start_tag (xmlp, "arg", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ARGUMENT);

		argument = entry->argument;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, entry);
		TEST_EQ_STR (argument->name, "test_arg");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ_P (argument->symbol, NULL);

		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		TEST_LIST_EMPTY (&method->arguments);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that direction of a method argument can be specified in
	 * an attribute as "out".
	 */
	TEST_FEATURE ("with method output argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);
		}

		attr[0] = "name";
		attr[1] = "test_arg";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = "direction";
		attr[5] = "out";
		attr[6] = NULL;

		ret = argument_start_tag (xmlp, "arg", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ARGUMENT);

		argument = entry->argument;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, entry);
		TEST_EQ_STR (argument->name, "test_arg");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ_P (argument->symbol, NULL);

		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		TEST_LIST_EMPTY (&method->arguments);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that direction of a signal argument can be specified in
	 * an attribute as "out".
	 */
	TEST_FEATURE ("with signal output argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);
		}

		attr[0] = "name";
		attr[1] = "test_arg";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = "direction";
		attr[5] = "out";
		attr[6] = NULL;

		ret = argument_start_tag (xmlp, "arg", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&signal->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ARGUMENT);

		argument = entry->argument;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, entry);
		TEST_EQ_STR (argument->name, "test_arg");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ_P (argument->symbol, NULL);

		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		TEST_LIST_EMPTY (&signal->arguments);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that the name attribute to an argument is optional, and when
	 * omitted NULL is stored in the structure.
	 */
	TEST_FEATURE ("with unnamed argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);
		}

		attr[0] = "type";
		attr[1] = "s";
		attr[2] = NULL;

		ret = argument_start_tag (xmlp, "arg", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ARGUMENT);

		argument = entry->argument;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, entry);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ_P (argument->symbol, NULL);

		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		TEST_LIST_EMPTY (&method->arguments);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an argument with an invalid name results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid argument name");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			attr[0] = "name";
			attr[1] = "test arg";
			attr[2] = "type";
			attr[3] = "s";
			attr[4] = NULL;
		}

		ret = argument_start_tag (xmlp, "arg", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&method->arguments);

		err = nih_error_get ();
		TEST_EQ (err->number, ARGUMENT_INVALID_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that an argument with a missing type attribute results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with missing argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			attr[0] = "name";
			attr[1] = "test_arg";
			attr[2] = NULL;
		}

		ret = argument_start_tag (xmlp, "arg", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&method->arguments);

		err = nih_error_get ();
		TEST_EQ (err->number, ARGUMENT_MISSING_TYPE);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that an argument with an invalid type results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			attr[0] = "name";
			attr[1] = "test_arg";
			attr[2] = "type";
			attr[3] = "!";
			attr[4] = NULL;
		}

		ret = argument_start_tag (xmlp, "arg", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&method->arguments);

		err = nih_error_get ();
		TEST_EQ (err->number, ARGUMENT_INVALID_TYPE);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that a method argument with an invalid direction results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid method argument direction");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			attr[0] = "name";
			attr[1] = "test_arg";
			attr[2] = "type";
			attr[3] = "s";
			attr[4] = "direction";
			attr[5] = "widdershins";
			attr[6] = NULL;
		}

		ret = argument_start_tag (xmlp, "arg", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&method->arguments);

		err = nih_error_get ();
		TEST_EQ (err->number, ARGUMENT_ILLEGAL_METHOD_DIRECTION);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that a signal argument with an invalid direction results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid signal argument direction");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);

			attr[0] = "name";
			attr[1] = "test_arg";
			attr[2] = "type";
			attr[3] = "s";
			attr[4] = "direction";
			attr[5] = "widdershins";
			attr[6] = NULL;
		}

		ret = argument_start_tag (xmlp, "arg", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&signal->arguments);

		err = nih_error_get ();
		TEST_EQ (err->number, ARGUMENT_ILLEGAL_SIGNAL_DIRECTION);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that "in" is an invalid direction for a signal argument
	 * and results in an error being raised.
	 */
	TEST_FEATURE ("with input signal argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);

			attr[0] = "name";
			attr[1] = "test_arg";
			attr[2] = "type";
			attr[3] = "s";
			attr[4] = "direction";
			attr[5] = "in";
			attr[6] = NULL;
		}

		ret = argument_start_tag (xmlp, "arg", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&signal->arguments);

		err = nih_error_get ();
		TEST_EQ (err->number, ARGUMENT_ILLEGAL_SIGNAL_DIRECTION);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that an unknown argument attribute results in a warning
	 * being printed to standard error, but is otherwise ignored
	 * and the normal processing finished.
	 */
	TEST_FEATURE ("with unknown attribute");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			attr[0] = "name";
			attr[1] = "test_arg";
			attr[2] = "type";
			attr[3] = "s";
			attr[4] = "frodo";
			attr[5] = "baggins";
			attr[6] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = argument_start_tag (xmlp, "arg", attr);
		}
		rewind (output);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			TEST_FILE_RESET (output);

			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ARGUMENT);

		argument = entry->argument;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, entry);
		TEST_EQ_STR (argument->name, "test_arg");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ_P (argument->symbol, NULL);

		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		TEST_LIST_EMPTY (&method->arguments);

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown <arg> attribute: "
				       "frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an argument on an empty stack (ie. a top-level
	 * argument element) results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with empty stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			attr[0] = "name";
			attr[1] = "test_arg";
			attr[2] = "type";
			attr[3] = "s";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = argument_start_tag (xmlp, "arg", attr);
		}
		rewind (output);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_EQ_P (parse_stack_top (&context.stack), NULL);

			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_IGNORED);
		TEST_EQ_P (entry->data, NULL);

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <arg> tag\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
	}


	/* Check that an argument on top of a stack entry that's not a
	 * method or signal results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with non-method/signal on stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);
			nih_discard (node);

			attr[0] = "name";
			attr[1] = "test_arg";
			attr[2] = "type";
			attr[3] = "s";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = argument_start_tag (xmlp, "arg", attr);
		}
		rewind (output);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			TEST_FILE_RESET (output);

			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_IGNORED);
		TEST_EQ_P (entry->data, NULL);

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <arg> tag\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	XML_ParserFree (xmlp);
	fclose (output);
}

void
test_end_tag (void)
{
	ParseContext context;
	ParseStack * parent = NULL;
	ParseStack * entry = NULL;
	XML_Parser   xmlp;
	Method *     method = NULL;
	Signal *     signal = NULL;
	Argument *   argument = NULL;
	Argument *   other1 = NULL;
	Argument *   other2 = NULL;
	int          ret;
	NihError *   err;

	TEST_FUNCTION ("argument_end_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);


	/* Check that when we parse the end tag for a named method argument,
	 * we pop the Argument object off the stack (freeing and removing
	 * it) and append it to the parent method's arguments list, adding
	 * a reference to the method as well.  A symbol should be generated
	 * for the argument by converting its name to C style.
	 */
	TEST_FEATURE ("with named method argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			argument = argument_new (NULL, "test_arg", "s",
						 NIH_DBUS_ARG_IN);
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		TEST_FREE_TAG (entry);

		ret = argument_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (argument, method);

		TEST_LIST_NOT_EMPTY (&method->arguments);
		TEST_EQ_P (method->arguments.next, &argument->entry);

		TEST_EQ_STR (argument->symbol, "test_arg");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (parent);
	}


	/* Check that when the symbol for a named method argument has been
	 * pre-assigned by the data, it's not overriden and is used even
	 * if different.
	 */
	TEST_FEATURE ("with symbol for named method argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			argument = argument_new (NULL, "test_arg", "s",
						 NIH_DBUS_ARG_IN);
			argument->symbol = nih_strdup (argument, "test");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		TEST_FREE_TAG (entry);

		ret = argument_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (argument, method);

		TEST_LIST_NOT_EMPTY (&method->arguments);
		TEST_EQ_P (method->arguments.next, &argument->entry);

		TEST_EQ_STR (argument->symbol, "test");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (parent);
	}


	/* Check that an unnamed argument without a symbol has the name
	 * argNN assigned, where NN is its position in the list.
	 */
	TEST_FEATURE ("with unnamed method argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			other1 = argument_new (method, NULL,
					       "s", NIH_DBUS_ARG_IN);
			nih_list_add (&method->arguments, &other1->entry);

			other2 = argument_new (method, NULL,
					       "i", NIH_DBUS_ARG_IN);
			nih_list_add (&method->arguments, &other2->entry);

			argument = argument_new (NULL, NULL, "s",
						 NIH_DBUS_ARG_IN);
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		TEST_FREE_TAG (entry);

		ret = argument_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (argument, method);

		TEST_LIST_NOT_EMPTY (&method->arguments);
		TEST_EQ_P (method->arguments.prev, &argument->entry);

		TEST_EQ_STR (argument->symbol, "arg3");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (parent);
	}


	/* Check that a symbol can be supplied even when a name isn't,
	 * and that symbol is used in preference to generating a name.
	 */
	TEST_FEATURE ("with symbol for unnamed method argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			other1 = argument_new (method, NULL,
					       "s", NIH_DBUS_ARG_IN);
			nih_list_add (&method->arguments, &other1->entry);

			other2 = argument_new (method, NULL,
					       "i", NIH_DBUS_ARG_IN);
			nih_list_add (&method->arguments, &other2->entry);

			argument = argument_new (NULL, NULL, "s",
						 NIH_DBUS_ARG_IN);
			argument->symbol = nih_strdup (argument, "test");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		TEST_FREE_TAG (entry);

		ret = argument_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (argument, method);

		TEST_LIST_NOT_EMPTY (&method->arguments);
		TEST_EQ_P (method->arguments.prev, &argument->entry);

		TEST_EQ_STR (argument->symbol, "test");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (parent);
	}


	/* Check that when we parse the end tag for a named signal argument,
	 * we pop the Argument object off the stack (freeing and removing
	 * it) and append it to the parent signal's arguments list, adding
	 * a reference to the signal as well.  A symbol should be generated
	 * for the argument by converting its name to C style.
	 */
	TEST_FEATURE ("with named signal argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);

			argument = argument_new (NULL, "test_arg", "s",
						 NIH_DBUS_ARG_IN);
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		TEST_FREE_TAG (entry);

		ret = argument_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&signal->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (argument, signal);

		TEST_LIST_NOT_EMPTY (&signal->arguments);
		TEST_EQ_P (signal->arguments.next, &argument->entry);

		TEST_EQ_STR (argument->symbol, "test_arg");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (parent);
	}


	/* Check that when the symbol for a named signal argument has been
	 * pre-assigned by the data, it's not overriden and is used even
	 * if different.
	 */
	TEST_FEATURE ("with symbol for named signal argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);

			argument = argument_new (NULL, "test_arg", "s",
						 NIH_DBUS_ARG_IN);
			argument->symbol = nih_strdup (argument, "test");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		TEST_FREE_TAG (entry);

		ret = argument_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&signal->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (argument, signal);

		TEST_LIST_NOT_EMPTY (&signal->arguments);
		TEST_EQ_P (signal->arguments.next, &argument->entry);

		TEST_EQ_STR (argument->symbol, "test");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (parent);
	}


	/* Check that an unnamed argument without a symbol has the name
	 * argNN assigned, where NN is its position in the list.
	 */
	TEST_FEATURE ("with unnamed signal argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);

			other1 = argument_new (signal, NULL,
					       "s", NIH_DBUS_ARG_IN);
			nih_list_add (&signal->arguments, &other1->entry);

			other2 = argument_new (signal, NULL,
					       "i", NIH_DBUS_ARG_IN);
			nih_list_add (&signal->arguments, &other2->entry);

			argument = argument_new (NULL, NULL, "s",
						 NIH_DBUS_ARG_IN);
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		TEST_FREE_TAG (entry);

		ret = argument_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (argument, signal);

		TEST_LIST_NOT_EMPTY (&signal->arguments);
		TEST_EQ_P (signal->arguments.prev, &argument->entry);

		TEST_EQ_STR (argument->symbol, "arg3");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (parent);
	}


	/* Check that a symbol can be supplied even when a name isn't,
	 * and that symbol is used in preference to generating a name.
	 */
	TEST_FEATURE ("with symbol for unnamed signal argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);

			other1 = argument_new (signal, NULL,
					       "s", NIH_DBUS_ARG_IN);
			nih_list_add (&signal->arguments, &other1->entry);

			other2 = argument_new (signal, NULL,
					       "i", NIH_DBUS_ARG_IN);
			nih_list_add (&signal->arguments, &other2->entry);

			argument = argument_new (NULL, NULL, "s",
						 NIH_DBUS_ARG_IN);
			argument->symbol = nih_strdup (argument, "test");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		TEST_FREE_TAG (entry);

		ret = argument_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&signal->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (argument, signal);

		TEST_LIST_NOT_EMPTY (&signal->arguments);
		TEST_EQ_P (signal->arguments.prev, &argument->entry);

		TEST_EQ_STR (argument->symbol, "test");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (parent);
	}


	/* Check that we won't allow a duplicate symbol for a method
	 * argument, and instead raise an error and allow the user to
	 * deal with it using the Symbol annotation.  We could work
	 * around this, but there's no point since argument names are
	 * only for bindings anyway so they should never clash!
	 */
	TEST_FEATURE ("with conflicting symbol for method argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			other1 = argument_new (method, "TestArg",
					       "i", NIH_DBUS_ARG_IN);
			other1->symbol = nih_strdup (other1, "test_arg");
			nih_list_add (&method->arguments, &other1->entry);

			argument = argument_new (NULL, "test_arg", "s",
						 NIH_DBUS_ARG_IN);
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		ret = argument_end_tag (xmlp, "arg");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		if ((! test_alloc_failed)
		    || (err->number != ENOMEM))
			TEST_EQ (err->number, ARGUMENT_DUPLICATE_SYMBOL);
		nih_free (err);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that we won't allow a duplicate symbol for a signal
	 * argument, and instead raise an error and allow the user to
	 * deal with it using the Symbol annotation.  We could work
	 * around this, but there's no point since argument names are
	 * only for bindings anyway so they should never clash!
	 */
	TEST_FEATURE ("with conflicting symbol for signal argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);

			other1 = argument_new (signal, "TestArg", "i", NIH_DBUS_ARG_IN);
			other1->symbol = nih_strdup (other1, "test_arg");
			nih_list_add (&signal->arguments, &other1->entry);

			argument = argument_new (NULL, "test_arg", "s",
						 NIH_DBUS_ARG_IN);
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		ret = argument_end_tag (xmlp, "arg");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		if ((! test_alloc_failed)
		    || (err->number != ENOMEM))
			TEST_EQ (err->number, ARGUMENT_DUPLICATE_SYMBOL);
		nih_free (err);

		nih_free (entry);
		nih_free (parent);
	}


	XML_ParserFree (xmlp);
}


void
test_annotation (void)
{
	Argument *argument = NULL;
	char *    symbol;
	int       ret;
	NihError *err;

	TEST_FUNCTION ("argument_annotation");


	/* Check that an annotation to add a symbol to the argument is
	 * handled, and the new symbol is stored in the argument.
	 */
	TEST_FEATURE ("with symbol annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			argument = argument_new (NULL, "TestArg", "s",
						 NIH_DBUS_ARG_IN);
		}

		ret = argument_annotation (argument,
					   "com.netsplit.Nih.Symbol",
					   "foo");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (argument);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (argument->symbol, "foo");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (argument);
	}


	/* Check that an annotation to add a symbol to the argument
	 * replaces any previous symbol applied (e.g. by a previous
	 * annotation).
	 */
	TEST_FEATURE ("with symbol annotation and existing symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			argument = argument_new (NULL, "TestArg", "s",
						 NIH_DBUS_ARG_IN);
			argument->symbol = nih_strdup (argument, "test_arg");
		}

		symbol = argument->symbol;
		TEST_FREE_TAG (symbol);

		ret = argument_annotation (argument,
					   "com.netsplit.Nih.Symbol",
					   "foo");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (argument);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (symbol);

		TEST_EQ_STR (argument->symbol, "foo");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (argument);
	}


	/* Check that an invalid symbol in an annotation results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid symbol in annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			argument = argument_new (NULL, "TestArg",
						 "s", NIH_DBUS_ARG_IN);
		}

		ret = argument_annotation (argument,
					   "com.netsplit.Nih.Symbol",
					   "foo bar");

		TEST_LT (ret, 0);

		TEST_EQ_P (argument->symbol, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, ARGUMENT_INVALID_SYMBOL);
		nih_free (err);

		nih_free (argument);
	}


	/* Check that an unknown annotation results in an error being
	 * raised.
	 */
	TEST_FEATURE ("with unknown annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			argument = argument_new (NULL, "TestArg",
						 "s", NIH_DBUS_ARG_IN);
		}

		ret = argument_annotation (argument,
					   "com.netsplit.Nih.Unknown",
					   "true");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		TEST_EQ (err->number, ARGUMENT_UNKNOWN_ANNOTATION);
		nih_free (err);

		nih_free (argument);
	}


	/* Check that the deprecated annotation is unknown for an
	 * argument.
	 */
	TEST_FEATURE ("with deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			argument = argument_new (NULL, "TestArg",
						 "s", NIH_DBUS_ARG_IN);
		}

		ret = argument_annotation (argument,
					   "org.freedesktop.DBus.Deprecated",
					   "true");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		TEST_EQ (err->number, ARGUMENT_UNKNOWN_ANNOTATION);
		nih_free (err);

		nih_free (argument);
	}
}


int
main (int   argc,
      char *argv[])
{
	program_name = "test";
	nih_error_init ();

	test_name_valid ();
	test_new ();
	test_start_tag ();
	test_end_tag ();
	test_annotation ();

	return 0;
}
