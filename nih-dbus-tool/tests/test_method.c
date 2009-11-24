/* nih-dbus-tool
 *
 * test_method.c - test suite for nih-dbus-tool/method.c
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

#include <dbus/dbus.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <expat.h>

#include <errno.h>
#include <assert.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/main.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>
#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_object.h>
#include <nih-dbus/dbus_proxy.h>
#include <nih-dbus/errors.h>

#include "type.h"
#include "node.h"
#include "method.h"
#include "argument.h"
#include "parse.h"
#include "errors.h"
#include "tests/method_code.h"


void
test_name_valid (void)
{
	TEST_FUNCTION ("method_name_valid");

	/* Check that a typical method name is valid. */
	TEST_FEATURE ("with typical method name");
	TEST_TRUE (method_name_valid ("Wibble"));


	/* Check that an method name is not valid if it is has an
	 * initial period.
	 */
	TEST_FEATURE ("with initial period");
	TEST_FALSE (method_name_valid (".Wibble"));


	/* Check that an method name is not valid if it ends with a period
	 */
	TEST_FEATURE ("with final period");
	TEST_FALSE (method_name_valid ("Wibble."));


	/* Check that an method name is not valid if it contains a period
	 */
	TEST_FEATURE ("with period");
	TEST_FALSE (method_name_valid ("Wib.ble"));


	/* Check that a method name may contain numbers */
	TEST_FEATURE ("with numbers");
	TEST_TRUE (method_name_valid ("Wib43ble"));


	/* Check that a method name may not begin with numbers */
	TEST_FEATURE ("with leading digits");
	TEST_FALSE (method_name_valid ("43Wibble"));


	/* Check that a method name may end with numbers */
	TEST_FEATURE ("with trailing digits");
	TEST_TRUE (method_name_valid ("Wibble43"));


	/* Check that a method name may contain underscores */
	TEST_FEATURE ("with underscore");
	TEST_TRUE (method_name_valid ("Wib_ble"));


	/* Check that a method name may begin with underscores */
	TEST_FEATURE ("with initial underscore");
	TEST_TRUE (method_name_valid ("_Wibble"));


	/* Check that a method name may end with underscores */
	TEST_FEATURE ("with final underscore");
	TEST_TRUE (method_name_valid ("Wibble_"));


	/* Check that other characters are not permitted */
	TEST_FEATURE ("with non-permitted characters");
	TEST_FALSE (method_name_valid ("Wib-ble"));


	/* Check that an empty method name is invalid */
	TEST_FEATURE ("with empty string");
	TEST_FALSE (method_name_valid (""));


	/* Check that an method name may not exceed 255 characters */
	TEST_FEATURE ("with overly long name");
	TEST_FALSE (method_name_valid ("ReallyLongMethodNameThatNobody"
				       "InTheirRightMindWouldEverUseNo"
				       "tInTheLeastBecauseThenYoudEndU"
				       "pWithAnEvenLongerInterfaceName"
				       "AndThatJustWontWorkWhenCombine"
				       "dButStillWeTestThisShitJustInc"
				       "aseSomeoneTriesItBecauseThatsW"
				       "hatTestDrivenDevelopmentIsAllA"
				       "bout.YayThereNow"));
}


void
test_new (void)
{
	Method *method;

	/* Check that an Method object is allocated with the structure
	 * filled in properly, but not placed in a list.
	 */
	TEST_FUNCTION ("method_new");
	TEST_ALLOC_FAIL {
		method = method_new (NULL, "Wibble");

		if (test_alloc_failed) {
			TEST_EQ_P (method, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_LIST_EMPTY (&method->entry);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_P (method->symbol, NULL);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->async);
		TEST_FALSE (method->no_reply);
		TEST_LIST_EMPTY (&method->arguments);

		nih_free (method);
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
	Interface *  interface = NULL;
	Method *     method;
	char *       attr[5];
	int          ret = 0;
	NihError *   err;
	FILE *       output;

	TEST_FUNCTION ("method_start_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);

	output = tmpfile ();


	/* Check that an method tag for an interface with the usual name
	 * attribute results in an Method member being created and pushed
	 * onto the stack with that attribute filled in correctly.
	 */
	TEST_FEATURE ("with method");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		attr[0] = "name";
		attr[1] = "TestMethod";
		attr[2] = NULL;

		ret = method_start_tag (xmlp, "method", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&interface->methods);

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
		TEST_EQ (entry->type, PARSE_METHOD);

		method = entry->method;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, entry);
		TEST_EQ_STR (method->name, "TestMethod");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_P (method->symbol, NULL);
		TEST_LIST_EMPTY (&method->arguments);

		TEST_LIST_EMPTY (&interface->methods);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a method with a missing name attribute results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with missing name");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			attr[0] = NULL;
		}

		ret = method_start_tag (xmlp, "method", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&interface->methods);

		err = nih_error_get ();
		TEST_EQ (err->number, METHOD_MISSING_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that a method with an invalid name results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid name");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			attr[0] = "name";
			attr[1] = "Test Method";
			attr[2] = NULL;
		}

		ret = method_start_tag (xmlp, "method", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&interface->methods);

		err = nih_error_get ();
		TEST_EQ (err->number, METHOD_INVALID_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that an unknown method attribute results in a warning
	 * being printed to standard error, but is otherwise ignored
	 * and the normal processing finished.
	 */
	TEST_FEATURE ("with unknown attribute");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			attr[0] = "name";
			attr[1] = "TestMethod";
			attr[2] = "frodo";
			attr[3] = "baggins";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = method_start_tag (xmlp, "method", attr);
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
		TEST_EQ (entry->type, PARSE_METHOD);

		method = entry->method;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, entry);
		TEST_EQ_STR (method->name, "TestMethod");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_P (method->symbol, NULL);
		TEST_LIST_EMPTY (&method->arguments);

		TEST_LIST_EMPTY (&interface->methods);

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown <method> attribute: "
				       "frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a method on an empty stack (ie. a top-level
	 * method element) results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with empty stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			attr[0] = "name";
			attr[1] = "TestMethod";
			attr[2] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = method_start_tag (xmlp, "method", attr);
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

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <method> tag\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
	}


	/* Check that a method on top of a stack entry that's not an
	 * interface results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with non-interface on stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);
			nih_discard (node);

			attr[0] = "name";
			attr[1] = "TestMethod";
			attr[2] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = method_start_tag (xmlp, "method", attr);
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

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <method> tag\n");
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
	ParseStack * entry  = NULL;
	XML_Parser   xmlp;
	Interface *  interface = NULL;
	Method *     method = NULL;
	Method *     other = NULL;
	Argument *   argument = NULL;
	int          ret = 0;
	NihError *   err;
	FILE *       output;

	TEST_FUNCTION ("method_end_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);

	output = tmpfile ();


	/* Check that when we parse the end tag for a method, we pop
	 * the Method object off the stack (freeing and removing it)
	 * and append it to the parent interface's methods list, adding a
	 * reference to the interface as well.  A symbol should be generated
	 * for the method by convering its name to C style.
	 */
	TEST_FEATURE ("with no assigned symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			method = method_new (NULL, "TestMethod");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
			nih_discard (method);
		}

		TEST_FREE_TAG (entry);

		ret = method_end_tag (xmlp, "method");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->methods);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (method, interface);

		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_EQ_P (interface->methods.next, &method->entry);

		TEST_EQ_STR (method->symbol, "test_method");
		TEST_ALLOC_PARENT (method->symbol, method);

		nih_free (parent);
	}


	/* Check that when the symbol has been pre-assigned by the data,
	 * it's not overridden and is used even if different.
	 */
	TEST_FEATURE ("with assigned symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			method = method_new (NULL, "TestMethod");
			method->symbol = nih_strdup (method, "foo");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
			nih_discard (method);
		}

		TEST_FREE_TAG (entry);

		ret = method_end_tag (xmlp, "method");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->methods);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (method, interface);

		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_EQ_P (interface->methods.next, &method->entry);

		TEST_EQ_STR (method->symbol, "foo");
		TEST_ALLOC_PARENT (method->symbol, method);

		nih_free (parent);
	}


	/* Check that we don't generate a duplicate symbol, and instead
	 * raise an error and allow the user to deal with it using
	 * the Symbol annotation.  The reason we don't work around this
	 * with a counter or similar is that the function names then
	 * become unpredicatable (introspection data isn't ordered).
	 */
	TEST_FEATURE ("with conflicting symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			other = method_new (interface, "Test");
			other->symbol = nih_strdup (other, "test_method");
			nih_list_add (&interface->methods, &other->entry);

			method = method_new (NULL, "TestMethod");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
			nih_discard (method);
		}

		ret = method_end_tag (xmlp, "method");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		if ((! test_alloc_failed)
		    || (err->number != ENOMEM))
			TEST_EQ (err->number, METHOD_DUPLICATE_SYMBOL);
		nih_free (err);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a method with the NoReply annotation and only
	 * input arguments is accepted.
	 */
	TEST_FEATURE ("with no reply expected");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			method = method_new (NULL, "TestMethod");
			method->no_reply = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
			nih_discard (method);

			argument = argument_new (method, NULL, "i",
						 NIH_DBUS_ARG_IN);
			nih_list_add (&method->arguments, &argument->entry);

			argument = argument_new (method, NULL, "i",
						 NIH_DBUS_ARG_IN);
			nih_list_add (&method->arguments, &argument->entry);
		}

		TEST_FREE_TAG (entry);

		TEST_DIVERT_STDERR (output) {
			ret = method_end_tag (xmlp, "method");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->methods);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (output);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (method, interface);

		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_EQ_P (interface->methods.next, &method->entry);

		TEST_EQ_STR (method->symbol, "test_method");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_TRUE (method->no_reply);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (parent);
	}


	/* Check that a method with the NoReply annotation and output
	 * arguments has the annotation removed and a warning emitted.
	 */
	TEST_FEATURE ("with no reply expected and output arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			method = method_new (NULL, "TestMethod");
			method->no_reply = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
			nih_discard (method);

			argument = argument_new (method, NULL, "i",
						 NIH_DBUS_ARG_OUT);
			nih_list_add (&method->arguments, &argument->entry);

			argument = argument_new (method, NULL, "i",
						 NIH_DBUS_ARG_OUT);
			nih_list_add (&method->arguments, &argument->entry);
		}

		TEST_FREE_TAG (entry);

		TEST_DIVERT_STDERR (output) {
			ret = method_end_tag (xmlp, "method");
		}
		rewind (output);

		if (test_alloc_failed
		    && (ret < 0)) {
			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->methods);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (output);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (method, interface);

		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_EQ_P (interface->methods.next, &method->entry);

		TEST_EQ_STR (method->symbol, "test_method");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->no_reply);

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored NoReply annotation for method with output arguments\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (parent);
	}


	/* Check that a method with the Async annotation is accepted.
	 */
	TEST_FEATURE ("with async implementation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			method = method_new (NULL, "TestMethod");
			method->async = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
			nih_discard (method);
		}

		TEST_FREE_TAG (entry);

		TEST_DIVERT_STDERR (output) {
			ret = method_end_tag (xmlp, "method");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->methods);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (output);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (method, interface);

		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_EQ_P (interface->methods.next, &method->entry);

		TEST_EQ_STR (method->symbol, "test_method");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_TRUE (method->async);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (parent);
	}


	/* Check that a method that is both Async and NoReply has the
	 * async annotation removed and a warning emitted.
	 */
	TEST_FEATURE ("with async but no reply expected");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			method = method_new (NULL, "TestMethod");
			method->async = TRUE;
			method->no_reply = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
			nih_discard (method);
		}

		TEST_FREE_TAG (entry);

		TEST_DIVERT_STDERR (output) {
			ret = method_end_tag (xmlp, "method");
		}
		rewind (output);

		if (test_alloc_failed
		    && (ret < 0)) {
			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->methods);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (output);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (method, interface);

		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_EQ_P (interface->methods.next, &method->entry);

		TEST_EQ_STR (method->symbol, "test_method");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->async);
		TEST_TRUE (method->no_reply);

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored Async annotation for NoReply method\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (parent);
	}


	/* Check that an Async method with the NoReply annotation but
	 * output arguments only has the NoReply annotation removed and
	 * a warning emitted about that, but remains async.
	 */
	TEST_FEATURE ("with async, no reply expected and output arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			method = method_new (NULL, "TestMethod");
			method->async = TRUE;
			method->no_reply = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
			nih_discard (method);

			argument = argument_new (method, NULL, "i",
						 NIH_DBUS_ARG_OUT);
			nih_list_add (&method->arguments, &argument->entry);

			argument = argument_new (method, NULL, "i",
						 NIH_DBUS_ARG_OUT);
			nih_list_add (&method->arguments, &argument->entry);
		}

		TEST_FREE_TAG (entry);

		TEST_DIVERT_STDERR (output) {
			ret = method_end_tag (xmlp, "method");
		}
		rewind (output);

		if (test_alloc_failed
		    && (ret < 0)) {
			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->methods);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (output);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (method, interface);

		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_EQ_P (interface->methods.next, &method->entry);

		TEST_EQ_STR (method->symbol, "test_method");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_TRUE (method->async);
		TEST_FALSE (method->no_reply);

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored NoReply annotation for method with output arguments\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (parent);
	}


	XML_ParserFree (xmlp);
	fclose (output);
}


void
test_annotation (void)
{
	Method *  method = NULL;
	char *    symbol;
	int       ret = 0;
	NihError *err;

	TEST_FUNCTION ("method_annotation");


	/* Check that the annotation to mark a method as deprecated is
	 * handled, and the Method is marked deprecated.
	 */
	TEST_FEATURE ("with deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
		}

		ret = method_annotation (method,
					 "org.freedesktop.DBus.Deprecated",
					 "true");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_FALSE (method->deprecated);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (method);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_TRUE (method->deprecated);

		nih_free (method);
	}


	/* Check that the annotation to mark a method as deprecated can be
	 * given a false value to explicitly mark the Method non-deprecated.
	 */
	TEST_FEATURE ("with explicitly non-deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			method->deprecated = TRUE;
		}

		ret = method_annotation (method,
					 "org.freedesktop.DBus.Deprecated",
					 "false");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_TRUE (method->deprecated);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (method);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FALSE (method->deprecated);

		nih_free (method);
	}


	/* Check that the annotation to mark a method caller to expect no
	 * reply is handled, and the Method is marked.
	 */
	TEST_FEATURE ("with no reply annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
		}

		ret = method_annotation (method,
					 "org.freedesktop.DBus.Method.NoReply",
					 "true");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_FALSE (method->no_reply);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (method);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_TRUE (method->no_reply);

		nih_free (method);
	}


	/* Check that the annotation to mark a method caller to expect
	 * no reply can be given a false value to explicitly mark the
	 * Method caller to expect one.
	 */
	TEST_FEATURE ("with explicitly replies annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			method->no_reply = TRUE;
		}

		ret = method_annotation (method,
					 "org.freedesktop.DBus.Method.NoReply",
					 "false");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_TRUE (method->no_reply);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (method);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FALSE (method->no_reply);

		nih_free (method);
	}


	/* Check that an annotation to add a symbol to the method is
	 * handled, and the new symbol is stored in the method.
	 */
	TEST_FEATURE ("with symbol annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
		}

		ret = method_annotation (method,
					 "com.netsplit.Nih.Symbol",
					 "foo");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (method);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (method->symbol, "foo");
		TEST_ALLOC_PARENT (method->symbol, method);

		nih_free (method);
	}


	/* Check that an annotation to add a symbol to the method
	 * replaces any previous symbol applied (e.g. by a previous
	 * annotation).
	 */
	TEST_FEATURE ("with symbol annotation and existing symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			method->symbol = nih_strdup (method, "test_method");
		}

		symbol = method->symbol;
		TEST_FREE_TAG (symbol);

		ret = method_annotation (method,
					 "com.netsplit.Nih.Symbol",
					 "foo");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (method);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (symbol);

		TEST_EQ_STR (method->symbol, "foo");
		TEST_ALLOC_PARENT (method->symbol, method);

		nih_free (method);
	}


	/* Check that the annotation to mark a method implementation as
	 * asynchronous is handled, and the Method is marked async.
	 */
	TEST_FEATURE ("with async annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
		}

		ret = method_annotation (method,
					 "com.netsplit.Nih.Method.Async",
					 "true");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_FALSE (method->async);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (method);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_TRUE (method->async);

		nih_free (method);
	}


	/* Check that the annotation to mark a method implementation as
	 * asynchronous can be given a false value to explicitly mark the
	 * Method synchronous.
	 */
	TEST_FEATURE ("with explicitly non-async annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			method->async = TRUE;
		}

		ret = method_annotation (method,
					 "com.netsplit.Nih.Method.Async",
					 "false");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_TRUE (method->async);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (method);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FALSE (method->async);

		nih_free (method);
	}


	/* Check that an invalid value for the deprecated annotation results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid value for deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
		}

		ret = method_annotation (method,
					 "org.freedesktop.DBus.Deprecated",
					 "foo");

		TEST_LT (ret, 0);

		TEST_EQ_P (method->symbol, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, METHOD_ILLEGAL_DEPRECATED);
		nih_free (err);

		nih_free (method);
	}


	/* Check that an invalid value for the no reply annotation results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid value for no reply annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");

			ret = method_annotation (method,
						 "org.freedesktop.DBus.Method.NoReply",
						 "foo");
		}

		TEST_LT (ret, 0);

		TEST_EQ_P (method->symbol, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, METHOD_ILLEGAL_NO_REPLY);
		nih_free (err);

		nih_free (method);
	}


	/* Check that an invalid symbol in an annotation results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid symbol in annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
		}

		ret = method_annotation (method,
					 "com.netsplit.Nih.Symbol",
					 "foo bar");

		TEST_LT (ret, 0);

		TEST_EQ_P (method->symbol, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, METHOD_INVALID_SYMBOL);
		nih_free (err);

		nih_free (method);
	}


	/* Check that an invalid value for the async annotation results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid value for async annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
		}

		ret = method_annotation (method,
					 "com.netsplit.Nih.Method.Async",
					 "foo");

		TEST_LT (ret, 0);

		TEST_EQ_P (method->symbol, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, METHOD_ILLEGAL_ASYNC);
		nih_free (err);

		nih_free (method);
	}


	/* Check that an unknown annotation results in an error being
	 * raised.
	 */
	TEST_FEATURE ("with unknown annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
		}

		ret = method_annotation (method,
					 "com.netsplit.Nih.Unknown",
					 "true");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		TEST_EQ (err->number, METHOD_UNKNOWN_ANNOTATION);
		nih_free (err);

		nih_free (method);
	}
}


void
test_lookup (void)
{
	Interface *interface = NULL;
	Method *   method1 = NULL;
	Method *   method2 = NULL;
	Method *   method3 = NULL;
	Method *   ret;

	TEST_FUNCTION ("method_lookup");


	/* Check that the function returns the method if there is one
	 * with the given symbol.
	 */
	TEST_FEATURE ("with matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");

			method1 = method_new (interface, "Test");
			method1->symbol = nih_strdup (method1, "test");
			nih_list_add (&interface->methods, &method1->entry);

			method2 = method_new (interface, "Foo");
			nih_list_add (&interface->methods, &method2->entry);

			method3 = method_new (interface, "Bar");
			method3->symbol = nih_strdup (method3, "bar");
			nih_list_add (&interface->methods, &method3->entry);
		}

		ret = method_lookup (interface, "bar");

		TEST_EQ_P (ret, method3);

		nih_free (interface);
	}


	/* Check that the function returns NULL if there is no method
	 * with the given symbol.
	 */
	TEST_FEATURE ("with non-matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");

			method1 = method_new (interface, "Test");
			method1->symbol = nih_strdup (method1, "test");
			nih_list_add (&interface->methods, &method1->entry);

			method2 = method_new (interface, "Foo");
			nih_list_add (&interface->methods, &method2->entry);

			method3 = method_new (interface, "Bar");
			method3->symbol = nih_strdup (method3, "bar");
			nih_list_add (&interface->methods, &method3->entry);
		}

		ret = method_lookup (interface, "baz");

		TEST_EQ_P (ret, NULL);

		nih_free (interface);
	}
}

void
test_lookup_argument (void)
{
	Method *  method = NULL;
	Argument *argument1 = NULL;
	Argument *argument2 = NULL;
	Argument *argument3 = NULL;
	Argument *ret;

	TEST_FUNCTION ("method_lookup_argument");


	/* Check that the function returns the argument if there is one
	 * with the given symbol.
	 */
	TEST_FEATURE ("with matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "com.netsplit.Nih.Test");

			argument1 = argument_new (method, "Test",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "test");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Foo",
						  "s", NIH_DBUS_ARG_IN);
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Bar",
						  "s", NIH_DBUS_ARG_IN);
			argument3->symbol = nih_strdup (argument3, "bar");
			nih_list_add (&method->arguments, &argument3->entry);
		}

		ret = method_lookup_argument (method, "bar");

		TEST_EQ_P (ret, argument3);

		nih_free (method);
	}


	/* Check that the function returns NULL if there is no argument
	 * with the given symbol.
	 */
	TEST_FEATURE ("with non-matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "com.netsplit.Nih.Test");

			argument1 = argument_new (method, "Test",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "test");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Foo",
						  "s", NIH_DBUS_ARG_IN);
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Bar",
						  "s", NIH_DBUS_ARG_IN);
			argument3->symbol = nih_strdup (argument3, "bar");
			nih_list_add (&method->arguments, &argument3->entry);
		}

		ret = method_lookup_argument (method, "baz");

		TEST_EQ_P (ret, NULL);

		nih_free (method);
	}
}


static int my_method_called = 0;

int
my_method (void *          data,
	   NihDBusMessage *message,
	   const char *    str,
	   int32_t         flags,
	   char ***        output)
{
	my_method_called++;

	TEST_EQ_P (data, NULL);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_EQ_STR (str, "this is a test");
	TEST_ALLOC_PARENT (str, message);

	TEST_NE_P (output, NULL);

	switch (flags) {
	case 0:
		*output = nih_str_split (message, str, " ", TRUE);
		if (! *output)
			nih_return_no_memory_error (-1);

		break;
	case 1:
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Method.Fail",
				      "Method failed");
		return -1;
	case 2:
		nih_error_raise (EBADF, strerror (EBADF));
		return -1;
	}

	return 0;
}

static int my_async_method_called = 0;

int
my_async_method (void *          data,
		 NihDBusMessage *message,
		 const char *    str,
		 int32_t         flags)
{
	my_async_method_called++;

	TEST_EQ_P (data, NULL);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_EQ_STR (str, "this is a test");
	TEST_ALLOC_PARENT (str, message);

	switch (flags) {
	case 0:
		break;
	case 1:
		nih_dbus_error_raise ("com.netsplit.Nih.Test.AsyncMethod.Fail",
				      "Method failed");
		return -1;
	case 2:
		nih_error_raise (EBADF, strerror (EBADF));
		return -1;
	}

	return 0;
}

void
test_object_function (void)
{
	pid_t             dbus_pid;
	DBusConnection *  server_conn;
	DBusConnection *  client_conn;
	NihList           prototypes;
	NihList           handlers;
	NihList           structs;
	Interface *       interface = NULL;
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
	TypeStruct *      structure;
	TypeVar *         var;
	NihListEntry *    attrib;
	int32_t           flags;
	double            double_arg;
	DBusMessage *     method_call;
	DBusMessage *     next_call;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	DBusMessage *     reply;
	NihDBusMessage *  message = NULL;
	NihDBusObject *   object = NULL;
	dbus_uint32_t     serial;
	dbus_uint32_t     next_serial;
	DBusHandlerResult result;
	DBusError         dbus_error;

	TEST_FUNCTION ("method_object_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a method call that demarshals a
	 * D-Bus message, calls a handler function with the demarshalled
	 * arguments and pointers to output arguments, then marshals those
	 * back into a reply message or an error as appropriate.
	 */
	TEST_FEATURE ("with standard method");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument3->symbol = nih_strdup (argument3, "output");
			nih_list_add (&method->arguments, &argument3->entry);
		}

		str = method_object_function (NULL, "my", interface, method,
					      &prototypes, &handlers,
					      &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_object_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusObject *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "object");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "str");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "flags");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char ***");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with no input arguments still results in
	 * a correctly generated function.
	 */
	TEST_FEATURE ("with no input arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument1->symbol = nih_strdup (argument1, "output");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_object_function (NULL, "my", interface, method,
					      &prototypes, &handlers,
					      &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_object_function_no_input.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusObject *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "object");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char ***");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with no output arguments still results in
	 * a correctly generated function.
	 */
	TEST_FEATURE ("with no output arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);
		}

		str = method_object_function (NULL, "my", interface, method,
					      &prototypes, &handlers,
					      &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_object_function_no_output.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusObject *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "object");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "str");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "flags");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with a structure as an input argument
	 * is correctly generated, with the structure type passed back
	 * in the structs array.
	 */
	TEST_FEATURE ("with structure input argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "structure",
						  "(su)", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "structure");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_object_function (NULL, "my", interface, method,
					      &prototypes, &handlers,
					      &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_object_function_structure_input.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusObject *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "object");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const MyMethodStructure *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "structure");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&handlers);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyMethodStructure");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with a structure as an output argument
	 * is correctly generated, with the structure type passed back
	 * in the structs array.
	 */
	TEST_FEATURE ("with structure output argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "structure",
						  "(su)", NIH_DBUS_ARG_OUT);
			argument1->symbol = nih_strdup (argument1, "structure");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_object_function (NULL, "my", interface, method,
					      &prototypes, &handlers,
					      &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_object_function_structure_output.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusObject *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "object");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyMethodStructure **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "structure");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&handlers);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyMethodStructure");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with no arguments still results in
	 * a correctly generated function.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");
		}

		str = method_object_function (NULL, "my", interface, method,
					      &prototypes, &handlers,
					      &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_object_function_no_args.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusObject *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "object");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that we can use the generated code to convert a message we
	 * send to a function call which on return causes a reply message
	 * to be sent back to us.
	 */
	TEST_FEATURE ("with normal return (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_called = 0;

		result = my_com_netsplit_Nih_Test_Method_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_ARRAY);

		TEST_EQ (dbus_message_iter_get_element_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str);
		TEST_EQ_STR (str, "this");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str);
		TEST_EQ_STR (str, "is");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str);
		TEST_EQ_STR (str, "a");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str);
		TEST_EQ_STR (str, "test");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that when no reply is expected, none is sent but the
	 * function returns success.
	 */
	TEST_FEATURE ("with no reply expected (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_message_set_no_reply (method_call, TRUE);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_called = 0;

		result = my_com_netsplit_Nih_Test_Method_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		next_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"NextMethod");

		dbus_connection_send (server_conn, next_call, &next_serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (next_call);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_CALL);
		TEST_EQ (dbus_message_get_serial (reply), next_serial);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that when the handler function raises a D-Bus error, we
	 * receive the error reply as an equivalent D-Bus error reply.
	 */
	TEST_FEATURE ("with D-Bus error from handler (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 1;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_called = 0;

		result = my_com_netsplit_Nih_Test_Method_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     "com.netsplit.Nih.Test.Method.Fail");

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that when the handler function raises a non-D-Bus error,
	 * we receive an error reply with the generic D-Bus failed error
	 * but the same message.
	 */
	TEST_FEATURE ("with generic error from handler (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 2;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_called = 0;

		result = my_com_netsplit_Nih_Test_Method_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     DBUS_ERROR_FAILED);

		dbus_error_init (&dbus_error);

		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, strerror (EBADF));

		dbus_error_free (&dbus_error);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that an incorrect type in the method we send results in
	 * an error reply being received and the function not being called.
	 */
	TEST_FEATURE ("with incorrect type in method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		dbus_message_iter_init_append (method_call, &iter);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_called = 0;

		result = my_com_netsplit_Nih_Test_Method_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     DBUS_ERROR_INVALID_ARGS);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that a missing argument in the method we send results in
	 * an error reply being received and the function not being called.
	 */
	TEST_FEATURE ("with missing argument in method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_called = 0;

		result = my_com_netsplit_Nih_Test_Method_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     DBUS_ERROR_INVALID_ARGS);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that an extra argument to the method also results in an
	 * error reply being received and the function not being called.
	 */
	TEST_FEATURE ("with too many arguments in method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		double_arg = 3.14;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_arg);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_called = 0;

		result = my_com_netsplit_Nih_Test_Method_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     DBUS_ERROR_INVALID_ARGS);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that we can generate an asynchronous method call that
	 * demarshals a D-Bus message, calls a handler function with the
	 * demarshalled arguments and then returns.  Errors should still
	 * be handled.
	 */
	TEST_FEATURE ("with asynchronous method");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "AsyncMethod");
			method->symbol = nih_strdup (method, "async_method");
			method->async = TRUE;

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument3->symbol = nih_strdup (argument3, "output");
			nih_list_add (&method->arguments, &argument3->entry);
		}

		str = method_object_function (NULL, "my", interface, method,
					      &prototypes, &handlers,
					      &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_object_function_async.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_AsyncMethod_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusObject *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "object");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_async_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "str");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "flags");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that we can use the generated asynchronous method code
	 * to convert a message we send to a function call which returns.
	 */
	TEST_FEATURE ("with asynchronous method return (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_called = 0;

		result = my_com_netsplit_Nih_Test_AsyncMethod_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_async_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (method_call);
	}


	/* Check that when no reply is expected, the asynchronous call
	 * is still left pending since the reply function will ignore it.
	 */
	TEST_FEATURE ("with no reply expected to async (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_message_set_no_reply (method_call, TRUE);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_called = 0;

		result = my_com_netsplit_Nih_Test_AsyncMethod_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_async_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (method_call);
	}


	/* Check that when the handler function raises a D-Bus error, we
	 * receive the error reply as an equivalent D-Bus error reply;
	 * since this constitutes handling, it should return handled.
	 */
	TEST_FEATURE ("with D-Bus error from async handler (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 1;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_called = 0;

		result = my_com_netsplit_Nih_Test_AsyncMethod_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_async_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     "com.netsplit.Nih.Test.AsyncMethod.Fail");

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that when the handler function raises a non-D-Bus error,
	 * we receive an error reply with the generic D-Bus failed error
	 * but the same message, this also constitutes being handled.
	 */
	TEST_FEATURE ("with generic error from async handler (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 2;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_called = 0;

		result = my_com_netsplit_Nih_Test_AsyncMethod_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_async_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     DBUS_ERROR_FAILED);

		dbus_error_init (&dbus_error);

		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, strerror (EBADF));

		dbus_error_free (&dbus_error);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that an incorrect type in the method we send results in
	 * an error reply being received and the function not being called,
	 * this constitutes being handled.
	 */
	TEST_FEATURE ("with incorrect type in async method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_called = 0;

		result = my_com_netsplit_Nih_Test_AsyncMethod_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_async_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     DBUS_ERROR_INVALID_ARGS);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that a missing argument in the method we send results in
	 * an error reply being received and the function not being called,
	 * again this is being handled.
	 */
	TEST_FEATURE ("with missing argument in async method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_called = 0;

		result = my_com_netsplit_Nih_Test_AsyncMethod_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_async_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     DBUS_ERROR_INVALID_ARGS);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that an extra argument to the method also results in an
	 * error reply being received and the function not being called,
	 * again this counts as being handled.
	 */
	TEST_FEATURE ("with too many arguments in async method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		double_arg = 3.14;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_arg);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_called = 0;

		result = my_com_netsplit_Nih_Test_AsyncMethod_method (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_async_method_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     DBUS_ERROR_INVALID_ARGS);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that a method call function for a deprecated method is
	 * identical to the standard one, and does not have the deprecated
	 * attribute since it would always result in a compiler warning/error
	 * and we generally always want to implement it.
	 */
	TEST_FEATURE ("with deprecated method");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");
			method->deprecated = TRUE;

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_object_function (NULL, "my", interface, method,
					      &prototypes, &handlers,
					      &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_object_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusObject *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "object");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "str");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_reply_function (void)
{
	pid_t             dbus_pid;
	DBusConnection *  server_conn;
	DBusConnection *  client_conn;
	NihList           prototypes;
	NihList           structs;
	Interface *       interface = NULL;
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	char *            str;
	int32_t           flags;
	char **           output = NULL;
	TypeFunc *        func;
	TypeVar *         arg;
	TypeStruct *      structure;
	TypeVar *         var;
	NihListEntry *    attrib;
	DBusMessage *     method_call;
	DBusMessage *     next_call;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	DBusMessage *     reply;
	NihDBusMessage *  message = NULL;
	NihDBusObject *   object = NULL;
	dbus_uint32_t     serial;
	dbus_uint32_t     next_serial;
	int               ret;

	TEST_FUNCTION ("method_reply_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that marhals its arguments
	 * into a D-Bus message and sends them as a reply to a previous
	 * D-Bus method call.
	 */
	TEST_FEATURE ("with reply");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "AsyncMethod");
			method->symbol = nih_strdup (method, "async_method");

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument3->symbol = nih_strdup (argument3, "output");
			nih_list_add (&method->arguments, &argument3->entry);
		}

		str = method_reply_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_reply_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_async_method_reply");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char * const *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call without output arguments still has
	 * a reply function generated for it.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "AsyncMethod");
			method->symbol = nih_strdup (method, "async_method");
		}

		str = method_reply_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_reply_function_no_args.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_async_method_reply");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a reply function for a method with a structure as
	 * an output argument is correctly generated, with the structure
	 * type passed back in the structs array.
	 */
	TEST_FEATURE ("with structure argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "AsyncMethod");
			method->symbol = nih_strdup (method, "async_method");

			argument1 = argument_new (method, "structure",
						  "(su)", NIH_DBUS_ARG_OUT);
			argument1->symbol = nih_strdup (argument1, "structure");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_reply_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_reply_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_async_method_reply");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const MyAsyncMethodStructure *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "structure");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyAsyncMethodStructure");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that an array argument may be NULL when the size is non-zero
	 */
	TEST_FEATURE ("with array argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "AsyncMethod");
			method->symbol = nih_strdup (method, "async_method");

			argument1 = argument_new (method, "Output",
						  "ai", NIH_DBUS_ARG_OUT);
			argument1->symbol = nih_strdup (argument1, "output");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_reply_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_reply_function_array.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_async_method_reply");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const int32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "size_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output_len");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that we can use the generated code to reply to a method
	 * call we created, and that we can receive the reply.
	 */
	TEST_FEATURE ("with reply (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;

			output = nih_str_split (NULL, "this is a test",
						" ", TRUE);
		}

		ret = my_async_method_reply (message, output);

		if (test_alloc_failed
		    && (ret < 0)) {
			nih_free (output);
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_ARRAY);

		TEST_EQ (dbus_message_iter_get_element_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str);
		TEST_EQ_STR (str, "this");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str);
		TEST_EQ_STR (str, "is");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str);
		TEST_EQ_STR (str, "a");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str);
		TEST_EQ_STR (str, "test");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		nih_free (output);
		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}

	/* Check that when no reply is expected, none is sent but the
	 * function returns success.
	 */
	TEST_FEATURE ("with no reply expected (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		dbus_message_iter_init_append (method_call, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		flags = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&flags);

		dbus_message_set_no_reply (method_call, TRUE);

		dbus_connection_send (server_conn, method_call, &serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_MESSAGE (client_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		TEST_ALLOC_SAFE {
			message = nih_new (NULL, NihDBusMessage);
			message->connection = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->connection = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;

			output = nih_str_split (NULL, "this is a test",
						" ", TRUE);
		}

		ret = my_async_method_reply (message, output);

		if (test_alloc_failed
		    && (ret < 0)) {
			nih_free (output);
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_EQ (ret, 0);

		next_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"NextMethod");

		dbus_connection_send (server_conn, next_call, &next_serial);
		dbus_connection_flush (server_conn);
		dbus_message_unref (next_call);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_CALL);
		TEST_EQ (dbus_message_get_serial (reply), next_serial);

		nih_free (output);
		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that the code for a deprecated method is the same as
	 * a non-deprecated one, since we don't want to penalise implementing
	 * the object - just using it remotely.
	 */
	TEST_FEATURE ("with deprecated method");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "AsyncMethod");
			method->symbol = nih_strdup (method, "async_method");
			method->deprecated = TRUE;

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument3->symbol = nih_strdup (argument3, "output");
			nih_list_add (&method->arguments, &argument3->entry);
		}

		str = method_reply_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_reply_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_async_method_reply");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char * const *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


int my_test_method_notify_called = FALSE;
static DBusPendingCall *   last_pending_call = NULL;
static NihDBusPendingData *last_pending_data = NULL;

void
my_com_netsplit_Nih_Test_TestMethod_notify (DBusPendingCall *   pending_call,
					    NihDBusPendingData *pending_data)
{
	my_test_method_notify_called = TRUE;
	last_pending_call = pending_call;
	last_pending_data = pending_data;
}

static void
my_blank_handler (void *          data,
		  NihDBusMessage *message,
		  char * const *  output,
		  int32_t         length)
{
}

static void
my_blank_error_handler (void *          data,
			NihDBusMessage *message)
{
}

void
test_proxy_function (void)
{
	pid_t             dbus_pid;
	DBusConnection *  server_conn;
	DBusConnection *  client_conn;
	NihList           prototypes;
	NihList           structs;
	Interface *       interface = NULL;
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	Argument *        argument4 = NULL;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
	TypeStruct *      structure;
	TypeVar *         var;
	NihListEntry *    attrib;
	DBusConnection *  flakey_conn = NULL;
	NihDBusProxy *    proxy = NULL;
	DBusPendingCall * pending_call;
	DBusMessage *     method_call;
	DBusMessage *     reply;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	char *            str_value;
	int32_t           int32_value;
	NihError *        err;
	NihDBusError *    dbus_err;

	TEST_FUNCTION ("method_proxy_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that marshals its
	 * arguments into a D-Bus message, makes a method call and returns
	 * the pending call structure.
	 */
	TEST_FEATURE ("with method call");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "TestMethod");
			method->symbol = nih_strdup (method, "test_method");

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument3->symbol = nih_strdup (argument3, "output");
			nih_list_add (&method->arguments, &argument3->entry);

			argument4 = argument_new (method, "Length",
						  "i", NIH_DBUS_ARG_OUT);
			argument4->symbol = nih_strdup (argument4, "length");
			nih_list_add (&method->arguments, &argument4->entry);
		}

		str = method_proxy_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "str");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "flags");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestMethodReply");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusErrorHandler");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "error_handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "timeout");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that we can generate a function for a method call with
	 * no arguments, and that it's all still proper.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "TestMethod");
			method->symbol = nih_strdup (method, "test_method");
		}

		str = method_proxy_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_function_no_args.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestMethodReply");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusErrorHandler");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "error_handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "timeout");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with a structure as an input argument
	 * is correctly generated, with the structure type passed back
	 * in the structs array.
	 */
	TEST_FEATURE ("with structure argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "TestMethod");
			method->symbol = nih_strdup (method, "test_method");

			argument1 = argument_new (method, "structure",
						  "(su)", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "structure");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_proxy_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const MyTestMethodStructure *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "structure");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestMethodReply");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusErrorHandler");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "error_handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "timeout");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyTestMethodStructure");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that when the function has an array argument, it does not
	 * assert that the pointer is not NULL unless the length pointer
	 * is non-zero.
	 */
	TEST_FEATURE ("with array argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "TestMethod");
			method->symbol = nih_strdup (method, "test_method");

			argument1 = argument_new (method, "Value",
						  "ai", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "value");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_proxy_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_function_array.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const int32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "size_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value_len");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestMethodReply");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusErrorHandler");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "error_handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "timeout");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that we can use the generated code to make a method call,
	 * it should return a DBusPendingCall object and we should receive
	 * the method call on the other side.  Returning the reply and
	 * blocking the call should result in our notify function being
	 * called with the pending call that was returned and the pending
	 * data with the expected information.
	 */
	TEST_FEATURE ("with method call (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_test_method_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_test_method (proxy, "test string", 42,
					       my_blank_handler,
					       my_blank_error_handler, &proxy, -1);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);
			continue;
		}

		TEST_NE_P (pending_call, NULL);


		TEST_DBUS_MESSAGE (server_conn, method_call);

		/* Check the incoming message */
		TEST_TRUE (dbus_message_is_method_call (method_call,
							"com.netsplit.Nih.Test",
							"TestMethod"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test string");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT32);

		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, 42);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "land";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "make";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "believe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		int32_value = 1234;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);


		/* Block the pending call until we receive the reply */
		dbus_pending_call_block (pending_call);
		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		reply = dbus_pending_call_steal_reply (pending_call);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		dbus_message_unref (reply);

		/* Check the notify function was called with all the right
		 * things.
		 */
		TEST_TRUE (my_test_method_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_handler);
		TEST_EQ_P (last_pending_data->error_handler,
			   my_blank_error_handler);
		TEST_EQ_P (last_pending_data->data, &proxy);

		/* Make sure the pending data is freed along with the
		 * pending call.
		 */
		TEST_FREE_TAG (last_pending_data);

		dbus_pending_call_unref (pending_call);

		TEST_FREE (last_pending_data);
		nih_free (proxy);
	}


	/* Check that the reply handler may be omitted if we only want to
	 * check for errors (assumedly we have no return arguments that
	 * we're interested in).  The function should still return a
	 * DBusPendingCall object and we should still receive the method
	 * call on the other side, and the notify function should still
	 * be called, just with a NULL handler.
	 */
	TEST_FEATURE ("with no handler (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_test_method_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_test_method (proxy, "test string", 42,
					       NULL,
					       my_blank_error_handler, &proxy, -1);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);
			continue;
		}

		TEST_NE_P (pending_call, NULL);


		TEST_DBUS_MESSAGE (server_conn, method_call);

		/* Check the incoming message */
		TEST_TRUE (dbus_message_is_method_call (method_call,
							"com.netsplit.Nih.Test",
							"TestMethod"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test string");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT32);

		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, 42);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "land";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "make";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "believe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		int32_value = 1234;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);


		/* Block the pending call until we receive the reply */
		dbus_pending_call_block (pending_call);
		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		reply = dbus_pending_call_steal_reply (pending_call);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		dbus_message_unref (reply);

		/* Check the notify function was called with all the right
		 * things.
		 */
		TEST_TRUE (my_test_method_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler, NULL);
		TEST_EQ_P (last_pending_data->error_handler,
			   my_blank_error_handler);
		TEST_EQ_P (last_pending_data->data, &proxy);

		/* Make sure the pending data is freed along with the
		 * pending call.
		 */
		TEST_FREE_TAG (last_pending_data);

		dbus_pending_call_unref (pending_call);

		TEST_FREE (last_pending_data);
		nih_free (proxy);
	}


	/* Check that the notify function is still called when the server
	 * returns an error; strictly speaking we're testing D-Bus here,
	 * but let's be complete about the whole thing - besides, it's
	 * good documentation for how things should behave <g>
	 */
	TEST_FEATURE ("with error reply (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_test_method_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_test_method (proxy, "test string", 42,
					       my_blank_handler,
					       my_blank_error_handler, &proxy, -1);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);
			continue;
		}

		TEST_NE_P (pending_call, NULL);


		TEST_DBUS_MESSAGE (server_conn, method_call);

		/* Check the incoming message */
		TEST_TRUE (dbus_message_is_method_call (method_call,
							"com.netsplit.Nih.Test",
							"TestMethod"));

		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test string");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT32);

		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, 42);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_error (method_call,
						"com.netsplit.Nih.Test.Method.Fail",
						"Things didn't work out");
		dbus_message_unref (method_call);

		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);


		/* Block the pending call until we receive the reply */
		dbus_pending_call_block (pending_call);
		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		reply = dbus_pending_call_steal_reply (pending_call);
		TEST_TRUE (dbus_message_is_error (reply,
						  "com.netsplit.Nih.Test.Method.Fail"));
		dbus_message_unref (reply);

		/* Check the notify function was called with all the right
		 * things.
		 */
		TEST_TRUE (my_test_method_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_handler);
		TEST_EQ_P (last_pending_data->error_handler,
			   my_blank_error_handler);
		TEST_EQ_P (last_pending_data->data, &proxy);

		/* Make sure the pending data is freed along with the
		 * pending call.
		 */
		TEST_FREE_TAG (last_pending_data);

		dbus_pending_call_unref (pending_call);

		TEST_FREE (last_pending_data);
		nih_free (proxy);
	}


	/* Check that the pending call will fail if the timeout is reached,
	 * we'll set a really short one for this.  The notify function
	 * should be called with the timeout error.
	 */
	TEST_FEATURE ("with timeout (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_test_method_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_test_method (proxy, "test string", 42,
					       my_blank_handler,
					       my_blank_error_handler, &proxy, 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);
			continue;
		}

		TEST_NE_P (pending_call, NULL);


		TEST_DBUS_MESSAGE (server_conn, method_call);

		/* Check the incoming message */
		TEST_TRUE (dbus_message_is_method_call (method_call,
							"com.netsplit.Nih.Test",
							"TestMethod"));

		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test string");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT32);

		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, 42);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (method_call);


		/* Block the pending call until timeout */
		dbus_pending_call_block (pending_call);
		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		reply = dbus_pending_call_steal_reply (pending_call);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_NO_REPLY));
		dbus_message_unref (reply);

		/* Check the notify function was called with all the right
		 * things.
		 */
		TEST_TRUE (my_test_method_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_handler);
		TEST_EQ_P (last_pending_data->error_handler,
			   my_blank_error_handler);
		TEST_EQ_P (last_pending_data->data, &proxy);

		/* Make sure the pending data is freed along with the
		 * pending call.
		 */
		TEST_FREE_TAG (last_pending_data);

		dbus_pending_call_unref (pending_call);

		TEST_FREE (last_pending_data);
		nih_free (proxy);
	}


	/* Check that the pending call will fail if the remote end
	 * disconnects.  The notify function should be called with the
	 * timeout error.
	 */
	TEST_FEATURE ("with server disconnection (generated code)");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (flakey_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_test_method_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_test_method (proxy, "test string", 42,
					       my_blank_handler,
					       my_blank_error_handler, &proxy, -1);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_NE_P (pending_call, NULL);


		TEST_DBUS_MESSAGE (flakey_conn, method_call);

		/* Check the incoming message */
		TEST_TRUE (dbus_message_is_method_call (method_call,
							"com.netsplit.Nih.Test",
							"TestMethod"));

		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test string");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT32);

		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, 42);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (method_call);


		/* Close the server connection */
		TEST_DBUS_CLOSE (flakey_conn);


		/* Block the pending call until error */
		dbus_pending_call_block (pending_call);
		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		reply = dbus_pending_call_steal_reply (pending_call);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_NO_REPLY));
		dbus_message_unref (reply);

		/* Check the notify function was called with all the right
		 * things.
		 */
		TEST_TRUE (my_test_method_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_handler);
		TEST_EQ_P (last_pending_data->error_handler,
			   my_blank_error_handler);
		TEST_EQ_P (last_pending_data->data, &proxy);

		/* Make sure the pending data is freed along with the
		 * pending call.
		 */
		TEST_FREE_TAG (last_pending_data);

		dbus_pending_call_unref (pending_call);

		TEST_FREE (last_pending_data);
		nih_free (proxy);
	}


	/* Check that the pending call can be cancelled by the user.  The
	 * notify function should not be called, but the data it contains
	 * freed (valgrind will reveal this).
	 */
	TEST_FEATURE ("with cancelled call (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_test_method_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_test_method (proxy, "test string", 42,
					       my_blank_handler,
					       my_blank_error_handler, &proxy, 50);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);
			continue;
		}

		TEST_NE_P (pending_call, NULL);


		TEST_DBUS_MESSAGE (server_conn, method_call);

		/* Check the incoming message */
		TEST_TRUE (dbus_message_is_method_call (method_call,
							"com.netsplit.Nih.Test",
							"TestMethod"));

		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test string");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT32);

		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, 42);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send a reply */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);


		/* Cancel the pending call */
		dbus_pending_call_cancel (pending_call);
		dbus_pending_call_unref (pending_call);

		/* Dispatch until we receive a message */
		TEST_DBUS_DISPATCH (client_conn);

		/* Check the notify function was not called. */
		TEST_FALSE (my_test_method_notify_called);

		nih_free (proxy);
	}


	/* Check that when the remote end is not connected, the function
	 * returns NULL and raises the disconnected D-Bus error.
	 */
	TEST_FEATURE ("with unconnected connection (generated code)");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, flakey_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		TEST_DBUS_CLOSE (flakey_conn);

		my_test_method_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_test_method (proxy, "test string", 42,
					       my_blank_handler,
					       my_blank_error_handler, &proxy, 50);

		TEST_EQ_P (pending_call, NULL);

		err = nih_error_get ();
		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);
			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_DISCONNECTED);

		nih_free (err);

		TEST_FALSE (my_test_method_notify_called);

		nih_free (proxy);
	}


	/* Check that we can pass NULL for both the callback and error
	 * handler arguments, in which case the method call is sent out
	 * with the flag set to expect no reply.  The notify function
	 * should not be called, since we don't care.
	 */
	TEST_FEATURE ("with no reply expected (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_test_method_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_test_method (proxy, "test string", 42,
					       NULL, NULL, NULL, -1);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);
			TEST_DBUS_CLOSE (flakey_conn);
			continue;
		}

		TEST_EQ_P (pending_call, (void *)TRUE);


		TEST_DBUS_MESSAGE (server_conn, method_call);

		/* Check the incoming message */
		TEST_TRUE (dbus_message_is_method_call (method_call,
							"com.netsplit.Nih.Test",
							"TestMethod"));

		TEST_TRUE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test string");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT32);

		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, 42);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send a reply anyway */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);

		/* Dispatch until we receive a message */
		TEST_DBUS_DISPATCH (client_conn);

		/* Check the notify function was not called. */
		TEST_FALSE (my_test_method_notify_called);

		nih_free (proxy);
	}


	/* Check that the function generated for a deprecated method
	 * has the deprecated attribute, since we want a gcc warning
	 * if the client uses it.
	 */
	TEST_FEATURE ("with deprecated method");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "TestMethod");
			method->symbol = nih_strdup (method, "test_method");
			method->deprecated = TRUE;

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument3->symbol = nih_strdup (argument3, "output");
			nih_list_add (&method->arguments, &argument3->entry);

			argument4 = argument_new (method, "Length",
						  "i", NIH_DBUS_ARG_OUT);
			argument4->symbol = nih_strdup (argument4, "length");
			nih_list_add (&method->arguments, &argument4->entry);
		}

		str = method_proxy_function (NULL, "my", interface, method,
					     &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_method");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "str");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "flags");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestMethodReply");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusErrorHandler");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "error_handler");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "timeout");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "deprecated");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);
		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

static int my_handler_called = FALSE;
static int my_error_handler_called = FALSE;
static NihDBusMessage *last_message = NULL;
static DBusConnection *last_conn = NULL;
static DBusMessage *last_msg = NULL;
static NihError *last_error = NULL;

static void
my_handler (void *          data,
	    NihDBusMessage *message,
	    char * const *  output,
	    int32_t         length)
{
	my_handler_called++;

	TEST_EQ_P (data, (void *)my_handler);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_message = message;
	TEST_FREE_TAG (last_message);

	last_conn = message->connection;
	dbus_connection_ref (last_conn);

	last_msg = message->message;
	dbus_message_ref (last_msg);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, message);
	TEST_ALLOC_SIZE (output, sizeof (char *) * 5);
	TEST_EQ_STR (output[0], "land");
	TEST_ALLOC_PARENT (output[0], output);
	TEST_EQ_STR (output[1], "of");
	TEST_ALLOC_PARENT (output[1], output);
	TEST_EQ_STR (output[2], "make");
	TEST_ALLOC_PARENT (output[2], output);
	TEST_EQ_STR (output[3], "believe");
	TEST_ALLOC_PARENT (output[3], output);
	TEST_EQ_P (output[4], NULL);

	TEST_EQ (length, 1234);
}

static void
my_error_handler (void *          data,
		  NihDBusMessage *message)
{
	my_error_handler_called++;

	TEST_EQ_P (data, (void *)my_handler);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_message = message;
	TEST_FREE_TAG (last_message);

	last_conn = message->connection;
	dbus_connection_ref (last_conn);

	last_msg = message->message;
	dbus_message_ref (last_msg);

	last_error = nih_error_steal ();
	TEST_NE_P (last_error, NULL);
}

void
test_proxy_notify_function (void)
{
	pid_t               dbus_pid;
	DBusConnection *    server_conn;
	DBusConnection *    client_conn;
	DBusConnection *    flakey_conn;
	NihList             prototypes;
	NihList             typedefs;
	NihList             structs;
	Interface *         interface = NULL;
	Method *            method = NULL;
	Argument *          argument1 = NULL;
	Argument *          argument2 = NULL;
	Argument *          argument3 = NULL;
	Argument *          argument4 = NULL;
	char *              str;
	TypeFunc *          func;
	TypeVar *           arg;
	TypeStruct *        structure;
	TypeVar *           var;
	dbus_uint32_t       serial;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data = NULL;
	DBusMessage *       method_call;
	DBusMessage *       reply;
	DBusMessageIter     iter;
	DBusMessageIter     subiter;
	char *              str_value;
	int32_t             int32_value;
	double              double_value;
	NihDBusError *      dbus_err;

	TEST_FUNCTION ("method_proxy_notify_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that takes a pending call
	 * and pending data structure, stealing the D-Bus message and
	 * demarshalling the arguments before making a call to either the
	 * handler for a valid reply or error handler for an invalid
	 * reply.  The typedef for the handler function is returned in
	 * addition to the prototype.
	 */
	TEST_FEATURE ("with reply");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument3->symbol = nih_strdup (argument3, "output");
			nih_list_add (&method->arguments, &argument3->entry);

			argument4 = argument_new (method, "Length",
						  "i", NIH_DBUS_ARG_OUT);
			argument4->symbol = nih_strdup (argument4, "length");
			nih_list_add (&method->arguments, &argument4->entry);
		}

		str = method_proxy_notify_function (NULL, "my", interface,
						    method,
						    &prototypes, &typedefs,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_notify_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_notify");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "pending_call");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusPendingData *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "pending_data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyMethodReply)");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char * const *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "length");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&typedefs);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that we can generate a function for a method with no
	 * arguments.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");
		}

		str = method_proxy_notify_function (NULL, "my", interface,
						    method,
						    &prototypes, &typedefs,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_notify_function_no_args.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_notify");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "pending_call");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusPendingData *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "pending_data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyMethodReply)");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&typedefs);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with a structure as an output argument
	 * is correctly generated, with the structure type passed back
	 * in the structs array.
	 */
	TEST_FEATURE ("with structure argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "structure",
						  "(su)", NIH_DBUS_ARG_OUT);
			argument1->symbol = nih_strdup (argument1, "structure");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_proxy_notify_function (NULL, "my", interface,
						    method,
						    &prototypes, &typedefs,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_notify_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_notify");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "pending_call");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusPendingData *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "pending_data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyMethodReply)");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const MyMethodStructure *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "structure");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&typedefs);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyMethodStructure");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that we can use the generated code to handle a completed
	 * pending call, demarshalling the arguments from the reply and
	 * passing them to our handler.
	 */
	TEST_FEATURE ("with reply (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, -1);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (server_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		/* Reply to it */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "land";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "make";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "believe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		int32_value = 1234;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		/* Send the reply */
		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);

		/* Now we should have the reply */
		dbus_pending_call_block (pending_call);
		assert (dbus_pending_call_get_completed (pending_call));


		TEST_ALLOC_SAFE {
			pending_data = nih_dbus_pending_data_new (
				NULL, client_conn,
				(NihDBusReplyHandler)my_handler,
				my_error_handler, (void *)my_handler);
		}

		my_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_Method_notify (pending_call, pending_data);

		TEST_TRUE (my_handler_called);
		TEST_FALSE (my_error_handler_called);

		TEST_NE_P (last_message, NULL);
		TEST_FREE (last_message);

		TEST_EQ_P (last_conn, client_conn);
		dbus_connection_unref (last_conn);

		TEST_NE_P (last_msg, NULL);
		TEST_EQ (dbus_message_get_reply_serial (last_msg),
			 serial);
		dbus_message_unref (last_msg);

		nih_free (pending_data);
		dbus_pending_call_unref (pending_call);
	}


	/* Check that the caller can omit the reply handler when it has
	 * no useful information it wants to obtain from the reply (thus
	 * only requiring the error handler), in which case the handler
	 * function should not be called.
	 */
	TEST_FEATURE ("with no handler (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, -1);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (server_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		/* Reply to it */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "land";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "make";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "believe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		int32_value = 1234;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		/* Send the reply */
		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);

		/* Now we should have the reply */
		dbus_pending_call_block (pending_call);
		assert (dbus_pending_call_get_completed (pending_call));


		TEST_ALLOC_SAFE {
			pending_data = nih_dbus_pending_data_new (
				NULL, client_conn,
				NULL,
				my_error_handler, (void *)my_handler);
		}

		my_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_Method_notify (pending_call, pending_data);

		TEST_FALSE (my_handler_called);
		TEST_FALSE (my_error_handler_called);

		nih_free (pending_data);
		dbus_pending_call_unref (pending_call);
	}


	/* Check that we can use the generated code to handle an error
	 * reply to a pending call, passing them instead to the error
	 * handler as a raised error.
	 */
	TEST_FEATURE ("with error reply (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, -1);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (server_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		/* Reply to it */
		reply = dbus_message_new_error (method_call,
						"com.netsplit.Nih.Test.Method.Fail",
						"Things didn't work out");
		dbus_message_unref (method_call);

		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);

		/* Now we should have the reply */
		dbus_pending_call_block (pending_call);
		assert (dbus_pending_call_get_completed (pending_call));


		TEST_ALLOC_SAFE {
			pending_data = nih_dbus_pending_data_new (
				NULL, client_conn,
				(NihDBusReplyHandler)my_handler,
				my_error_handler, (void *)my_handler);
		}

		my_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;
		last_error = NULL;

		my_com_netsplit_Nih_Test_Method_notify (pending_call, pending_data);

		TEST_FALSE (my_handler_called);
		TEST_TRUE (my_error_handler_called);

		TEST_NE_P (last_message, NULL);
		TEST_FREE (last_message);

		TEST_EQ_P (last_conn, client_conn);
		dbus_connection_unref (last_conn);

		TEST_NE_P (last_msg, NULL);
		TEST_EQ (dbus_message_get_reply_serial (last_msg),
			 serial);
		dbus_message_unref (last_msg);

		TEST_NE_P (last_error, NULL);
		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Method.Fail");
		TEST_EQ_STR (last_error->message, "Things didn't work out");
		nih_free (last_error);

		nih_free (pending_data);
		dbus_pending_call_unref (pending_call);
	}


	/* Check that the generated code catches a timeout of the pending
	 * call and runs the error handler with the D-Bus timeout error
	 * raised.
	 */
	TEST_FEATURE ("with timeout (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (server_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);
		dbus_message_unref (method_call);

		/* Wait for timeout */
		dbus_pending_call_block (pending_call);
		assert (dbus_pending_call_get_completed (pending_call));


		TEST_ALLOC_SAFE {
			pending_data = nih_dbus_pending_data_new (
				NULL, client_conn,
				(NihDBusReplyHandler)my_handler,
				my_error_handler, (void *)my_handler);
		}

		my_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;
		last_error = NULL;

		my_com_netsplit_Nih_Test_Method_notify (pending_call, pending_data);

		TEST_FALSE (my_handler_called);
		TEST_TRUE (my_error_handler_called);

		TEST_NE_P (last_message, NULL);
		TEST_FREE (last_message);

		TEST_EQ_P (last_conn, client_conn);
		dbus_connection_unref (last_conn);

		TEST_NE_P (last_msg, NULL);
		TEST_EQ (dbus_message_get_reply_serial (last_msg),
			 serial);
		dbus_message_unref (last_msg);

		TEST_NE_P (last_error, NULL);
		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);
		nih_free (last_error);

		nih_free (pending_data);
		dbus_pending_call_unref (pending_call);
	}


	/* Check that the generated code catches disconnection of the
	 * remote end during a pending call call and runs the error handler
	 * with the D-Bus timeout error raised.
	 */
	TEST_FEATURE ("with disconnection (generated code)");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (flakey_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (flakey_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);
		dbus_message_unref (method_call);

		TEST_DBUS_CLOSE (flakey_conn);

		/* Wait for error */
		dbus_pending_call_block (pending_call);
		assert (dbus_pending_call_get_completed (pending_call));


		TEST_ALLOC_SAFE {
			pending_data = nih_dbus_pending_data_new (
				NULL, client_conn,
				(NihDBusReplyHandler)my_handler,
				my_error_handler, (void *)my_handler);
		}

		my_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;
		last_error = NULL;

		my_com_netsplit_Nih_Test_Method_notify (pending_call, pending_data);

		TEST_FALSE (my_handler_called);
		TEST_TRUE (my_error_handler_called);

		TEST_NE_P (last_message, NULL);
		TEST_FREE (last_message);

		TEST_EQ_P (last_conn, client_conn);
		dbus_connection_unref (last_conn);

		TEST_NE_P (last_msg, NULL);
		TEST_EQ (dbus_message_get_reply_serial (last_msg),
			 serial);
		dbus_message_unref (last_msg);

		TEST_NE_P (last_error, NULL);
		TEST_EQ (last_error->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (last_error, sizeof (NihDBusError));

		dbus_err = (NihDBusError *)last_error;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_NO_REPLY);
		nih_free (last_error);

		nih_free (pending_data);
		dbus_pending_call_unref (pending_call);
	}


	/* Check that the generated code catches an invalid argument type in
	 * the reply and calls the error handler with the invalid arguments
	 * error raised.
	 */
	TEST_FEATURE ("with incorrect argument type (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, -1);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (server_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		/* Reply to it */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "land";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "make";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "believe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		double_value = 1.618;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_value);

		/* Send the reply */
		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);

		/* Now we should have the reply */
		dbus_pending_call_block (pending_call);
		assert (dbus_pending_call_get_completed (pending_call));


		TEST_ALLOC_SAFE {
			pending_data = nih_dbus_pending_data_new (
				NULL, client_conn,
				(NihDBusReplyHandler)my_handler,
				my_error_handler, (void *)my_handler);
		}

		my_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;
		last_error = NULL;

		my_com_netsplit_Nih_Test_Method_notify (pending_call, pending_data);

		TEST_FALSE (my_handler_called);
		TEST_TRUE (my_error_handler_called);

		TEST_NE_P (last_message, NULL);
		TEST_FREE (last_message);

		TEST_EQ_P (last_conn, client_conn);
		dbus_connection_unref (last_conn);

		TEST_NE_P (last_msg, NULL);
		TEST_EQ (dbus_message_get_reply_serial (last_msg),
			 serial);
		dbus_message_unref (last_msg);

		TEST_NE_P (last_error, NULL);
		TEST_EQ (last_error->number, NIH_DBUS_INVALID_ARGS);
		nih_free (last_error);

		nih_free (pending_data);
		dbus_pending_call_unref (pending_call);
	}


	/* Check that the generated code catches an invalid argument type in
	 * the reply even when there's no handler for it and still calls the
	 * error handler with the invalid arguments error raised.
	 */
	TEST_FEATURE ("with incorrect argument type and no handler (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, -1);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (server_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		/* Reply to it */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "land";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "make";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "believe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		double_value = 1.618;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_value);

		/* Send the reply */
		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);

		/* Now we should have the reply */
		dbus_pending_call_block (pending_call);
		assert (dbus_pending_call_get_completed (pending_call));


		TEST_ALLOC_SAFE {
			pending_data = nih_dbus_pending_data_new (
				NULL, client_conn,
				NULL,
				my_error_handler, (void *)my_handler);
		}

		my_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;
		last_error = NULL;

		my_com_netsplit_Nih_Test_Method_notify (pending_call, pending_data);

		TEST_FALSE (my_handler_called);
		TEST_TRUE (my_error_handler_called);

		TEST_NE_P (last_message, NULL);
		TEST_FREE (last_message);

		TEST_EQ_P (last_conn, client_conn);
		dbus_connection_unref (last_conn);

		TEST_NE_P (last_msg, NULL);
		TEST_EQ (dbus_message_get_reply_serial (last_msg),
			 serial);
		dbus_message_unref (last_msg);

		TEST_NE_P (last_error, NULL);
		TEST_EQ (last_error->number, NIH_DBUS_INVALID_ARGS);
		nih_free (last_error);

		nih_free (pending_data);
		dbus_pending_call_unref (pending_call);
	}


	/* Check that the generated code catches insufficient arguments in
	 * the reply and calls the error handler with the invalid arguments
	 * error raised.
	 */
	TEST_FEATURE ("with missing argument (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, -1);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (server_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		/* Reply to it */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "land";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "make";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "believe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		/* Send the reply */
		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);

		/* Now we should have the reply */
		dbus_pending_call_block (pending_call);
		assert (dbus_pending_call_get_completed (pending_call));


		TEST_ALLOC_SAFE {
			pending_data = nih_dbus_pending_data_new (
				NULL, client_conn,
				(NihDBusReplyHandler)my_handler,
				my_error_handler, (void *)my_handler);
		}

		my_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;
		last_error = NULL;

		my_com_netsplit_Nih_Test_Method_notify (pending_call, pending_data);

		TEST_FALSE (my_handler_called);
		TEST_TRUE (my_error_handler_called);

		TEST_NE_P (last_message, NULL);
		TEST_FREE (last_message);

		TEST_EQ_P (last_conn, client_conn);
		dbus_connection_unref (last_conn);

		TEST_NE_P (last_msg, NULL);
		TEST_EQ (dbus_message_get_reply_serial (last_msg),
			 serial);
		dbus_message_unref (last_msg);

		TEST_NE_P (last_error, NULL);
		TEST_EQ (last_error->number, NIH_DBUS_INVALID_ARGS);
		nih_free (last_error);

		nih_free (pending_data);
		dbus_pending_call_unref (pending_call);
	}


	/* Check that the generated code catches too many arguments in
	 * the reply and calls the error handler with the invalid arguments
	 * error raised.
	 */
	TEST_FEATURE ("with too many arguments (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih.Test",
			"Method");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, -1);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (server_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		/* Reply to it */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "land";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "make";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "believe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		int32_value = 1234;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		double_value = 1.618;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_value);

		/* Send the reply */
		dbus_connection_send (server_conn, reply, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (reply);

		/* Now we should have the reply */
		dbus_pending_call_block (pending_call);
		assert (dbus_pending_call_get_completed (pending_call));


		TEST_ALLOC_SAFE {
			pending_data = nih_dbus_pending_data_new (
				NULL, client_conn,
				(NihDBusReplyHandler)my_handler,
				my_error_handler, (void *)my_handler);
		}

		my_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;
		last_error = NULL;

		my_com_netsplit_Nih_Test_Method_notify (pending_call, pending_data);

		TEST_FALSE (my_handler_called);
		TEST_TRUE (my_error_handler_called);

		TEST_NE_P (last_message, NULL);
		TEST_FREE (last_message);

		TEST_EQ_P (last_conn, client_conn);
		dbus_connection_unref (last_conn);

		TEST_NE_P (last_msg, NULL);
		TEST_EQ (dbus_message_get_reply_serial (last_msg),
			 serial);
		dbus_message_unref (last_msg);

		TEST_NE_P (last_error, NULL);
		TEST_EQ (last_error->number, NIH_DBUS_INVALID_ARGS);
		nih_free (last_error);

		nih_free (pending_data);
		dbus_pending_call_unref (pending_call);
	}


	/* Check that the generated function for a deprecated method is
	 * not marked deprecated, since it's implementation.
	 */
	TEST_FEATURE ("with deprecated method");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");
			method->deprecated = TRUE;

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument3->symbol = nih_strdup (argument3, "output");
			nih_list_add (&method->arguments, &argument3->entry);

			argument4 = argument_new (method, "Length",
						  "i", NIH_DBUS_ARG_OUT);
			argument4->symbol = nih_strdup (argument4, "length");
			nih_list_add (&method->arguments, &argument4->entry);
		}

		str = method_proxy_notify_function (NULL, "my", interface,
						    method,
						    &prototypes, &typedefs,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_notify_function_array.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Method_notify");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "pending_call");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusPendingData *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "pending_data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyMethodReply)");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "data");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char * const *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "length");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&typedefs);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_proxy_sync_function (void)
{
	pid_t             dbus_pid;
	DBusConnection *  server_conn;
	DBusConnection *  client_conn;
	DBusConnection *  flakey_conn;
	NihList           prototypes;
	NihList           structs;
	Interface *       interface = NULL;
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	Argument *        argument4 = NULL;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
	TypeStruct *      structure;
	TypeVar *         var;
	NihListEntry *    attrib;
	pid_t             pid;
	int               status;
	NihDBusProxy *    proxy = NULL;
	void *            parent = NULL;
	DBusMessage *     method_call;
	DBusMessage *     reply;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	char *            str_value;
	int32_t           int32_value;
	char **           output;
	int               ret;
	NihError *        err;
	NihDBusError *    dbus_err;

	TEST_FUNCTION ("method_proxy_sync_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that marshals its
	 * arguments into a D-Bus message, makes a method call, waits for
	 * the reply, demarshals the reply message into its output
	 * arguments and returns the message context for the reply.
	 */
	TEST_FEATURE ("with method call");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);

			argument3 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument3->symbol = nih_strdup (argument3, "output");
			nih_list_add (&method->arguments, &argument3->entry);

			argument4 = argument_new (method, "Length",
						  "i", NIH_DBUS_ARG_OUT);
			argument4->symbol = nih_strdup (argument4, "length");
			nih_list_add (&method->arguments, &argument4->entry);
		}

		str = method_proxy_sync_function (NULL, "my", interface,
						  method,
						  &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_sync_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method_sync");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "parent");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "str");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "flags");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char ***");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "length");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a function with no input arguments still results in
	 * correctly generated code.
	 */
	TEST_FEATURE ("with no input arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "Output",
						  "as", NIH_DBUS_ARG_OUT);
			argument1->symbol = nih_strdup (argument1, "output");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Length",
						  "i", NIH_DBUS_ARG_OUT);
			argument2->symbol = nih_strdup (argument2, "length");
			nih_list_add (&method->arguments, &argument2->entry);
		}

		str = method_proxy_sync_function (NULL, "my", interface,
						  method,
						  &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_sync_function_no_input.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method_sync");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "parent");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char ***");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "output");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "length");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with no output arguments still results
	 * in correctly generated code.
	 */
	TEST_FEATURE ("with no output arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "Str",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "str");
			nih_list_add (&method->arguments, &argument1->entry);

			argument2 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument2->symbol = nih_strdup (argument2, "flags");
			nih_list_add (&method->arguments, &argument2->entry);
		}

		str = method_proxy_sync_function (NULL, "my", interface,
						  method,
						  &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_sync_function_no_output.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method_sync");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "parent");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "str");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "flags");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with no arguments at all still results
	 * in correctly generated code.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");
		}

		str = method_proxy_sync_function (NULL, "my", interface,
						  method,
						  &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_sync_function_no_args.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method_sync");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "parent");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with a structure as an input argument
	 * is correctly generated, with the structure type passed back
	 * in the structs array.
	 */
	TEST_FEATURE ("with structure input argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "structure",
						  "(su)", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "structure");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_proxy_sync_function (NULL, "my", interface,
						  method,
						  &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_sync_function_structure_input.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method_sync");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "parent");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const MyMethodStructure *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "structure");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyMethodStructure");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that a method call with a structure as an output argument
	 * is correctly generated, with the structure type passed back
	 * in the structs array.
	 */
	TEST_FEATURE ("with structure output argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "structure",
						  "(su)", NIH_DBUS_ARG_OUT);
			argument1->symbol = nih_strdup (argument1, "structure");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_proxy_sync_function (NULL, "my", interface,
						  method,
						  &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_sync_function_structure_output.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method_sync");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "parent");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyMethodStructure **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "structure");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyMethodStructure");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that an array input argument may be NULL if the length
	 * is zero.
	 */
	TEST_FEATURE ("with array input argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");

			argument1 = argument_new (method, "Value",
						  "ai", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "value");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_proxy_sync_function (NULL, "my", interface,
						  method,
						  &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_sync_function_array_input.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method_sync");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "parent");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const int32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "size_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value_len");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that we can use the generated code to make a method call,
	 * and that it returns an NihDBusMessage * which is the parent of
	 * the output arguments placed in the pointer we provided.
	 */
	TEST_FEATURE ("with method call (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								"com.netsplit.Nih.Test",
								"Method"));

			TEST_FALSE (dbus_message_get_no_reply (method_call));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "test string");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INT32);

			dbus_message_iter_get_basic (&iter, &int32_value);
			TEST_EQ (int32_value, 42);

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter);

			str_value = "land";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "of";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "make";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "believe";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&iter, &subiter);

			int32_value = 1234;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
							&int32_value);

			dbus_connection_send (server_conn, reply, NULL);
			dbus_connection_flush (server_conn);
			dbus_message_unref (reply);

			TEST_DBUS_CLOSE (client_conn);
			TEST_DBUS_CLOSE (server_conn);

			dbus_shutdown ();
			exit (0);
		}

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
			parent = nih_alloc (proxy, 0);
		}

		output = NULL;
		int32_value = 0;

		ret = my_method_sync (parent, proxy, "test string", 42,
				      &output, &int32_value);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			/* If we failed with ENOMEM, the server must not
			 * have processed the reply
			 */
			kill (pid, SIGTERM);

			waitpid (pid, &status, 0);
			TEST_TRUE (WIFSIGNALED (status));
			TEST_EQ (WTERMSIG (status), SIGTERM);

			TEST_EQ_P (output, NULL);
			TEST_EQ (int32_value, 0);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (ret, 0);

		TEST_NE_P (output, NULL);
		TEST_ALLOC_PARENT (output, parent);
		TEST_ALLOC_SIZE (output, sizeof (char *) * 5);
		TEST_EQ_STR (output[0], "land");
		TEST_ALLOC_PARENT (output[0], output);
		TEST_EQ_STR (output[1], "of");
		TEST_ALLOC_PARENT (output[1], output);
		TEST_EQ_STR (output[2], "make");
		TEST_ALLOC_PARENT (output[2], output);
		TEST_EQ_STR (output[3], "believe");
		TEST_ALLOC_PARENT (output[3], output);
		TEST_EQ_P (output[4], NULL);

		TEST_EQ (int32_value, 1234);

		nih_free (proxy);
	}


	/* Check that the generated code handles an out-of-memory error
	 * from the remote end, and returns it as if there was an
	 * out-of-memory error on the local end so it can be repeated in
	 * the same manner.
	 */
	TEST_FEATURE ("with no memory error (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								"com.netsplit.Nih.Test",
								"Method"));

			TEST_FALSE (dbus_message_get_no_reply (method_call));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "test string");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INT32);

			dbus_message_iter_get_basic (&iter, &int32_value);
			TEST_EQ (int32_value, 42);

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_error (method_call,
							DBUS_ERROR_NO_MEMORY,
							"Out of hunk!");
			dbus_message_unref (method_call);

			dbus_connection_send (server_conn, reply, NULL);
			dbus_connection_flush (server_conn);
			dbus_message_unref (reply);

			TEST_DBUS_CLOSE (client_conn);
			TEST_DBUS_CLOSE (server_conn);

			dbus_shutdown ();
			exit (0);
		}

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
			parent = nih_alloc (proxy, 0);
		}

		output = NULL;
		int32_value = 0;

		ret = my_method_sync (parent, proxy, "test string", 42,
				      &output, &int32_value);

		kill (pid, SIGTERM);
		waitpid (pid, &status, 0);
		if (! WIFSIGNALED (status)) {
			TEST_TRUE (WIFEXITED (status));
			TEST_EQ (WEXITSTATUS (status), 0);
		} else {
			TEST_EQ (WTERMSIG (status), SIGTERM);
		}

		TEST_LT (ret, 0);

		err = nih_error_get ();
		TEST_EQ (err->number, ENOMEM);
		nih_free (err);

		nih_free (proxy);
	}


	/* Check that the generated code handles an error returned
	 * from the remote end, and returns it as a raised error on the
	 * local end.
	 */
	TEST_FEATURE ("with error return (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								"com.netsplit.Nih.Test",
								"Method"));

			TEST_FALSE (dbus_message_get_no_reply (method_call));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "test string");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INT32);

			dbus_message_iter_get_basic (&iter, &int32_value);
			TEST_EQ (int32_value, 42);

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_error (method_call,
							"com.netsplit.Nih.Test.Method.Failed",
							"Didn't work out, sorry");
			dbus_message_unref (method_call);

			dbus_connection_send (server_conn, reply, NULL);
			dbus_connection_flush (server_conn);
			dbus_message_unref (reply);

			TEST_DBUS_CLOSE (client_conn);
			TEST_DBUS_CLOSE (server_conn);

			dbus_shutdown ();
			exit (0);
		}

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
			parent = nih_alloc (proxy, 0);
		}

		output = NULL;
		int32_value = 0;

		ret = my_method_sync (parent, proxy, "test string", 42,
				      &output, &int32_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			/* If we failed with ENOMEM, the server must not
			 * have processed the reply
			 */
			kill (pid, SIGTERM);

			waitpid (pid, &status, 0);
			TEST_TRUE (WIFSIGNALED (status));
			TEST_EQ (WTERMSIG (status), SIGTERM);

			TEST_EQ_P (output, NULL);
			TEST_EQ (int32_value, 0);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;

		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Test.Method.Failed");
		TEST_EQ_STR (err->message, "Didn't work out, sorry");

		nih_free (err);

		nih_free (proxy);
	}


	/* Check that the generated code returns a raised disconnected
	 * error when called on a disconnected connection.
	 */
	TEST_FEATURE ("with disconnected connection (generated code)");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (flakey_conn);

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, flakey_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
			parent = nih_alloc (proxy, 0);
		}

		TEST_DBUS_CLOSE (flakey_conn);

		output = NULL;
		int32_value = 0;

		ret = my_method_sync (parent, proxy, "test string", 42,
				      &output, &int32_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			TEST_EQ_P (output, NULL);
			TEST_EQ (int32_value, 0);

			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_DISCONNECTED);

		nih_free (err);

		TEST_EQ_P (output, NULL);
		TEST_EQ (int32_value, 0);

		nih_free (proxy);
	}


	/* Check that if the remote method returns a wrong type in the
	 * reply, an error is returned by the proxied call to indicate
	 * that it refused to accept the reply.
	 */
	TEST_FEATURE ("with incorrect type (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								"com.netsplit.Nih.Test",
								"Method"));

			TEST_FALSE (dbus_message_get_no_reply (method_call));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "test string");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INT32);

			dbus_message_iter_get_basic (&iter, &int32_value);
			TEST_EQ (int32_value, 42);

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter);

			str_value = "land";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "of";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "make";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "believe";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&iter, &subiter);

			str_value = "wibble";
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&str_value);

			dbus_connection_send (server_conn, reply, NULL);
			dbus_connection_flush (server_conn);
			dbus_message_unref (reply);

			TEST_DBUS_CLOSE (client_conn);
			TEST_DBUS_CLOSE (server_conn);

			dbus_shutdown ();
			exit (0);
		}

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
			parent = nih_alloc (proxy, 0);
		}

		output = NULL;
		int32_value = 0;

		ret = my_method_sync (parent, proxy, "test string", 42,
				      &output, &int32_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			/* If we failed with ENOMEM, the server must not
			 * have processed the reply
			 */
			kill (pid, SIGTERM);

			waitpid (pid, &status, 0);
			TEST_TRUE (WIFSIGNALED (status));
			TEST_EQ (WTERMSIG (status), SIGTERM);

			TEST_EQ_P (output, NULL);
			TEST_EQ (int32_value, 0);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		nih_free (proxy);
	}


	/* Check that if the remote method returns with a missing output
	 * argument, an error is returned by the proxied call to indicate
	 * that it refused to accept the reply.
	 */
	TEST_FEATURE ("with missing argument (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								"com.netsplit.Nih.Test",
								"Method"));

			TEST_FALSE (dbus_message_get_no_reply (method_call));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "test string");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INT32);

			dbus_message_iter_get_basic (&iter, &int32_value);
			TEST_EQ (int32_value, 42);

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter);

			str_value = "land";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "of";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "make";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "believe";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&iter, &subiter);

			dbus_connection_send (server_conn, reply, NULL);
			dbus_connection_flush (server_conn);
			dbus_message_unref (reply);

			TEST_DBUS_CLOSE (client_conn);
			TEST_DBUS_CLOSE (server_conn);

			dbus_shutdown ();
			exit (0);
		}

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
			parent = nih_alloc (proxy, 0);
		}

		output = NULL;
		int32_value = 0;

		ret = my_method_sync (parent, proxy, "test string", 42,
				      &output, &int32_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			/* If we failed with ENOMEM, the server must not
			 * have processed the reply
			 */
			kill (pid, SIGTERM);

			waitpid (pid, &status, 0);
			TEST_TRUE (WIFSIGNALED (status));
			TEST_EQ (WTERMSIG (status), SIGTERM);

			TEST_EQ_P (output, NULL);
			TEST_EQ (int32_value, 0);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		nih_free (proxy);
	}


	/* Check that if the remote method returns with too many arguments,
	 * an error is returned by the proxied call to indicate that it
	 * refused to accept the reply.
	 */
	TEST_FEATURE ("with too many arguments (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								"com.netsplit.Nih.Test",
								"Method"));

			TEST_FALSE (dbus_message_get_no_reply (method_call));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "test string");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INT32);

			dbus_message_iter_get_basic (&iter, &int32_value);
			TEST_EQ (int32_value, 42);

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter);

			str_value = "land";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "of";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "make";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "believe";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&iter, &subiter);

			int32_value = 1234;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
							&int32_value);

			str_value = "wibble";
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&str_value);

			dbus_connection_send (server_conn, reply, NULL);
			dbus_connection_flush (server_conn);
			dbus_message_unref (reply);

			TEST_DBUS_CLOSE (client_conn);
			TEST_DBUS_CLOSE (server_conn);

			dbus_shutdown ();
			exit (0);
		}

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
			parent = nih_alloc (proxy, 0);
		}

		output = NULL;
		int32_value = 0;

		ret = my_method_sync (parent, proxy, "test string", 42,
				      &output, &int32_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			/* If we failed with ENOMEM, the server must not
			 * have processed the reply
			 */
			kill (pid, SIGTERM);

			waitpid (pid, &status, 0);
			TEST_TRUE (WIFSIGNALED (status));
			TEST_EQ (WTERMSIG (status), SIGTERM);

			TEST_EQ_P (output, NULL);
			TEST_EQ (int32_value, 0);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		nih_free (proxy);
	}


	/* Check that a deprecated method call has the deprecated attribute
	 * added to its prototype, so using it results in a compiler
	 * warning.
	 */
	TEST_FEATURE ("with deprecated method");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");
			method->deprecated = TRUE;

			argument1 = argument_new (method, "Flags",
						  "i", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "flags");
			nih_list_add (&method->arguments, &argument1->entry);
		}

		str = method_proxy_sync_function (NULL, "my", interface,
						  method,
						  &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_method_proxy_sync_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_method_sync");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const void *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "parent");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "flags");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "warn_unused_result");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "deprecated");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&prototypes);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_args_array (void)
{
	NihList    prototypes;
	Interface *interface = NULL;
	Method *   method = NULL;
	Argument * arg1 = NULL;
	Argument * arg2 = NULL;
	Argument * arg3 = NULL;
	char *     str;
	TypeVar *  var;


	TEST_FUNCTION ("method_args_array");


	/* Check that we can generate an array of argument definitions for
	 * a method, with each name and type lined up with each other and
	 * the final part lined up too.  Arguments without names should have
	 * NULL in place of the name.
	 */
	TEST_FEATURE ("with arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			method = method_new (interface, "Method");
			method->symbol = "method";
			nih_list_add (&interface->methods, &method->entry);

			arg1 = argument_new (method, "foo",
					     "as", NIH_DBUS_ARG_IN);
			arg1->symbol = "foo";
			nih_list_add (&method->arguments, &arg1->entry);

			arg2 = argument_new (method, "wibble",
					     "i", NIH_DBUS_ARG_OUT);
			arg2->symbol = "wibble";
			nih_list_add (&method->arguments, &arg2->entry);

			arg3 = argument_new (method, NULL,
					     "a(iii)", NIH_DBUS_ARG_IN);
			arg3->symbol = "arg3";
			nih_list_add (&method->arguments, &arg3->entry);
		}

		str = method_args_array (NULL, "my", interface, method,
					 &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusArg my_com_netsplit_Nih_Test_Method_method_args[] = {\n"
			     "\t{ \"foo\",    \"as\",     NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"wibble\", \"i\",      NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL,     \"a(iii)\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusArg");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_Method_method_args");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that a method with no arguments has an empty array
	 * returned.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			method = method_new (interface, "Method");
			method->symbol = "method";
			nih_list_add (&interface->methods, &method->entry);
		}

		str = method_args_array (NULL, "my", interface, method,
					 &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusArg my_com_netsplit_Nih_Test_Method_method_args[] = {\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusArg");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_Method_method_args");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
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
	test_lookup ();
	test_lookup_argument ();

	test_object_function ();
	test_reply_function ();
	test_proxy_function ();
	test_proxy_notify_function ();
	test_proxy_sync_function ();

	test_args_array ();

	return 0;
}
