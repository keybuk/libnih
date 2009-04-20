/* nih-dbus-tool
 *
 * test_method.c - test suite for nih-dbus-tool/method.c
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
#include <nih-dbus/test_dbus.h>

#include <dbus/dbus.h>

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
	Interface *  interface = NULL;
	Method *     method;
	char *       attr[5];
	int          ret;
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
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

	attr[0] = NULL;

	ret = method_start_tag (xmlp, "method", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->methods);

	err = nih_error_get ();
	TEST_EQ (err->number, METHOD_MISSING_NAME);
	nih_free (err);

	nih_free (parent);


	/* Check that a method with an invalid name results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid name");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

	attr[0] = "name";
	attr[1] = "Test Method";
	attr[2] = NULL;

	ret = method_start_tag (xmlp, "method", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->methods);

	err = nih_error_get ();
	TEST_EQ (err->number, METHOD_INVALID_NAME);
	nih_free (err);

	nih_free (parent);


	/* Check that an unknown method attribute results in a warning
	 * being printed to standard error, but is otherwise ignored
	 * and the normal processing finished.
	 */
	TEST_FEATURE ("with unknown attribute");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

	attr[0] = "name";
	attr[1] = "TestMethod";
	attr[2] = "frodo";
	attr[3] = "baggins";
	attr[4] = NULL;

	TEST_DIVERT_STDERR (output) {
		ret = method_start_tag (xmlp, "method", attr);
	}
	rewind (output);

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


	/* Check that a method on an empty stack (ie. a top-level
	 * method element) results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with empty stack");
	attr[0] = "name";
	attr[1] = "TestMethod";
	attr[2] = NULL;

	TEST_DIVERT_STDERR (output) {
		ret = method_start_tag (xmlp, "method", attr);
	}
	rewind (output);

	TEST_EQ (ret, 0);

	entry = parse_stack_top (&context.stack);
	TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
	TEST_EQ (entry->type, PARSE_IGNORED);
	TEST_EQ_P (entry->data, NULL);

	TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <method> tag\n");
	TEST_FILE_END (output);
	TEST_FILE_RESET (output);

	nih_free (entry);


	/* Check that a method on top of a stack entry that's not an
	 * interface results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with non-interface on stack");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_NODE, node_new (NULL, NULL));

	attr[0] = "name";
	attr[1] = "TestMethod";
	attr[2] = NULL;

	TEST_DIVERT_STDERR (output) {
		ret = method_start_tag (xmlp, "method", attr);
	}
	rewind (output);

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
	int          ret;
	NihError *   err;

	TEST_FUNCTION ("method_end_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);


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

			method = method_new (NULL, "TestMethod");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
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

			method = method_new (NULL, "TestMethod");
			method->symbol = nih_strdup (method, "foo");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
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
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

	other = method_new (interface, "Test");
	other->symbol = nih_strdup (other, "test_method");
	nih_list_add (&interface->methods, &other->entry);

	method = method_new (NULL, "TestMethod");
	entry = parse_stack_push (NULL, &context.stack,
				  PARSE_METHOD, method);

	ret = method_end_tag (xmlp, "method");

	TEST_LT (ret, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, METHOD_DUPLICATE_SYMBOL);
	nih_free (err);

	nih_free (entry);
	nih_free (parent);


	XML_ParserFree (xmlp);
}


void
test_annotation (void)
{
	Method *  method = NULL;
	char *    symbol;
	int       ret;
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
	method = method_new (NULL, "TestMethod");

	ret = method_annotation (method,
				 "org.freedesktop.DBus.Deprecated",
				 "foo");

	TEST_LT (ret, 0);

	TEST_EQ_P (method->symbol, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, METHOD_ILLEGAL_DEPRECATED);
	nih_free (err);

	nih_free (method);


	/* Check that an invalid value for the no reply annotation results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid value for no reply annotation");
	method = method_new (NULL, "TestMethod");

	ret = method_annotation (method,
				 "org.freedesktop.DBus.Method.NoReply",
				 "foo");

	TEST_LT (ret, 0);

	TEST_EQ_P (method->symbol, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, METHOD_ILLEGAL_NO_REPLY);
	nih_free (err);

	nih_free (method);


	/* Check that an invalid symbol in an annotation results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid symbol in annotation");
	method = method_new (NULL, "TestMethod");

	ret = method_annotation (method,
				 "com.netsplit.Nih.Symbol",
				 "foo bar");

	TEST_LT (ret, 0);

	TEST_EQ_P (method->symbol, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, METHOD_INVALID_SYMBOL);
	nih_free (err);

	nih_free (method);


	/* Check that an invalid value for the async annotation results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid value for async annotation");
	method = method_new (NULL, "TestMethod");

	ret = method_annotation (method,
				 "com.netsplit.Nih.Method.Async",
				 "foo");

	TEST_LT (ret, 0);

	TEST_EQ_P (method->symbol, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, METHOD_ILLEGAL_ASYNC);
	nih_free (err);

	nih_free (method);


	/* Check that an unknown annotation results in an error being
	 * raised.
	 */
	TEST_FEATURE ("with unknown annotation");
	method = method_new (NULL, "TestMethod");

	ret = method_annotation (method,
				 "com.netsplit.Nih.Unknown",
				 "true");

	TEST_LT (ret, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, METHOD_UNKNOWN_ANNOTATION);
	nih_free (err);

	nih_free (method);
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


static int my_method_handler_called = 0;

int
my_method_handler (void *          data,
		   NihDBusMessage *message,
		   const char *    str,
		   int32_t         flags,
		   char ***        output)
{
	my_method_handler_called++;

	TEST_EQ_P (data, NULL);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->conn, NULL);
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
		nih_dbus_error_raise ("com.netsplit.Nih.MyMethod.Fail",
				      "MyMethod failed");
		return -1;
	case 2:
		nih_error_raise (EBADF, strerror (EBADF));
		return -1;
	}

	return 0;
}

static int my_async_method_handler_called = 0;

int
my_async_method_handler (void *          data,
			 NihDBusMessage *message,
			 const char *    str,
			 int32_t         flags)
{
	my_async_method_handler_called++;

	TEST_EQ_P (data, NULL);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->conn, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_EQ_STR (str, "this is a test");
	TEST_ALLOC_PARENT (str, message);

	switch (flags) {
	case 0:
		break;
	case 1:
		nih_dbus_error_raise ("com.netsplit.Nih.MyAsyncMethod.Fail",
				      "MyMethod failed");
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
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	char *            str;
	int32_t           flags;
	double            double_arg;
	DBusMessage *     method_call;
	DBusMessage *     next_call;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	DBusMessage *     reply;
	NihDBusMessage *  message;
	NihDBusObject *   object;
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
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "MyMethod");
			method->symbol = nih_strdup (method, "my_method");

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

		str = method_object_function (NULL, method,
					      "MyMethod_handle",
					      "my_method_handler");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (method);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "MyMethod_handle (NihDBusObject * object, NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage * reply;\n"
				   "\tchar * str;\n"
				   "\tconst char * str_dbus;\n"
				   "\tint32_t flags;\n"
				   "\tchar ** output;\n"
				   "\tDBusMessageIter output_iter;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to MyMethod method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &str_dbus);\n"
				   "\n"
				   "\tstr = nih_strdup (message, str_dbus);\n"
				   "\tif (! str) {\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t/* Demarshal a int32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to MyMethod method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &flags);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to MyMethod method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_method_handler (object->data, message, str, flags, &output) < 0) {\n"
				   "\t\tNihError *err;\n"
				   "\n"
				   "\t\terr = nih_error_get ();\n"
				   "\t\tif (err->number == ENOMEM) {\n"
				   "\t\t\tnih_free (err);\n"
				   "\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t} else if (err->number == NIH_DBUS_ERROR) {\n"
				   "\t\t\tNihDBusError *dbus_err = (NihDBusError *)err;\n"
				   "\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->conn, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t} else {\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->conn, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\n"
				   "\t/* If the sender doesn't care about a reply, don't bother wasting\n"
				   "\t * effort constructing and sending one.\n"
				   "\t */\n"
				   "\tif (dbus_message_get_no_reply (message->message))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t/* Construct the reply message. */\n"
				   "\t\treply = dbus_message_new_method_return (message->message);\n"
				   "\t\tif (! reply)\n"
				   "\t\t\tcontinue;\n"
				   "\n"
				   "\t\tdbus_message_iter_init_append (reply, &iter);\n"
				   "\n"
				   "\t\t/* Marshal an array onto the message */\n"
				   "\t\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"s\", &output_iter)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treply = NULL;\n"
				   "\t\t\tcontinue;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tfor (size_t output_i = 0; output[output_i]; output_i++) {\n"
				   "\t\t\tconst char * output_element;\n"
				   "\n"
				   "\t\t\toutput_element = output[output_i];\n"
				   "\n"
				   "\t\t\t/* Marshal a char * onto the message */\n"
				   "\t\t\tif (! dbus_message_iter_append_basic (&output_iter, DBUS_TYPE_STRING, &output_element)) {\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\treply = NULL;\n"
				   "\t\t\t\tcontinue;\n"
				   "\t\t\t}\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tif (! dbus_message_iter_close_container (&iter, &output_iter)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treply = NULL;\n"
				   "\t\t\tcontinue;\n"
				   "\t\t}\n"
				   "\t} while (! reply);\n"
				   "\n"
				   "\t/* Send the reply, appending it to the outgoing queue. */\n"
				   "\tNIH_MUST (dbus_connection_send (message->conn, reply, NULL));\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "}\n"));

		nih_free (str);
		nih_free (method);
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
			"com.netsplit.Nih",
			"MyMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_handler_called = 0;

		result = MyMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_method_handler_called);
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
			"com.netsplit.Nih",
			"MyMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_handler_called = 0;

		result = MyMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_method_handler_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		next_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih",
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
			"com.netsplit.Nih",
			"MyMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_handler_called = 0;

		result = MyMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_method_handler_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     "com.netsplit.Nih.MyMethod.Fail");

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
			"com.netsplit.Nih",
			"MyMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_handler_called = 0;

		result = MyMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_method_handler_called);
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
			"com.netsplit.Nih",
			"MyMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_handler_called = 0;

		result = MyMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_method_handler_called);
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
			"com.netsplit.Nih",
			"MyMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_handler_called = 0;

		result = MyMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_method_handler_called);
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
			"com.netsplit.Nih",
			"MyMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_method_handler_called = 0;

		result = MyMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_method_handler_called);
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
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "MyAsyncMethod");
			method->symbol = nih_strdup (method, "my_async_method");
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

		str = method_object_function (NULL, method,
					      "MyAsyncMethod_handle",
					      "my_async_method_handler");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (method);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "MyAsyncMethod_handle (NihDBusObject * object, NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage * reply;\n"
				   "\tchar * str;\n"
				   "\tconst char * str_dbus;\n"
				   "\tint32_t flags;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to MyAsyncMethod method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &str_dbus);\n"
				   "\n"
				   "\tstr = nih_strdup (message, str_dbus);\n"
				   "\tif (! str) {\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t/* Demarshal a int32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to MyAsyncMethod method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &flags);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to MyAsyncMethod method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_async_method_handler (object->data, message, str, flags) < 0) {\n"
				   "\t\tNihError *err;\n"
				   "\n"
				   "\t\terr = nih_error_get ();\n"
				   "\t\tif (err->number == ENOMEM) {\n"
				   "\t\t\tnih_free (err);\n"
				   "\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t} else if (err->number == NIH_DBUS_ERROR) {\n"
				   "\t\t\tNihDBusError *dbus_err = (NihDBusError *)err;\n"
				   "\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->conn, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t} else {\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->conn, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "}\n"));

		nih_free (str);
		nih_free (method);
	}


	/* Check that we can use the generated asynchronous method code
	 * to convert a message we send to a function call which returns.
	 */
	TEST_FEATURE ("with asynchronous method return (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih",
			"MyAsyncMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_handler_called = 0;

		result = MyAsyncMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_async_method_handler_called);
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
			"com.netsplit.Nih",
			"MyAsyncMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_handler_called = 0;

		result = MyAsyncMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_async_method_handler_called);
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
			"com.netsplit.Nih",
			"MyAsyncMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_handler_called = 0;

		result = MyAsyncMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_async_method_handler_called);
		TEST_EQ (result, DBUS_HANDLER_RESULT_HANDLED);

		TEST_DBUS_MESSAGE (server_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     "com.netsplit.Nih.MyAsyncMethod.Fail");

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
			"com.netsplit.Nih",
			"MyAsyncMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_handler_called = 0;

		result = MyAsyncMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_async_method_handler_called);
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
			"com.netsplit.Nih",
			"MyAsyncMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_handler_called = 0;

		result = MyAsyncMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_async_method_handler_called);
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
			"com.netsplit.Nih",
			"MyAsyncMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_handler_called = 0;

		result = MyAsyncMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_async_method_handler_called);
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
			"com.netsplit.Nih",
			"MyAsyncMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
			object->data = NULL;
			object->interfaces = NULL;
			object->registered = TRUE;
		}

		my_async_method_handler_called = 0;

		result = MyAsyncMethod_handle (object, message);

		if (test_alloc_failed
		    && (result == DBUS_HANDLER_RESULT_NEED_MEMORY)) {
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_async_method_handler_called);
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
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	char *            str;
	int32_t           flags;
	char **           output;
	DBusMessage *     method_call;
	DBusMessage *     next_call;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	DBusMessage *     reply;
	NihDBusMessage *  message;
	NihDBusObject *   object;
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
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "MyAsyncMethod");
			method->symbol = nih_strdup (method, "my_async_method");

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

		str = method_reply_function (NULL, method,
					     "my_async_method_reply");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (method);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_async_method_reply (NihDBusMessage *message, char * const * output)\n"
				   "{\n"
				   "\tDBusMessage * reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter output_iter;\n"
				   "\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (output != NULL);\n"
				   "\n"
				   "\t/* If the sender doesn't care about a reply, don't bother wasting\n"
				   "\t * effort constructing and sending one.\n"
				   "\t */\n"
				   "\tif (dbus_message_get_no_reply (message->message))\n"
				   "\t\treturn 0;\n"
				   "\n"
				   "\t/* Construct the reply message. */\n"
				   "\treply = dbus_message_new_method_return (message->message);\n"
				   "\tif (! reply)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (reply, &iter);\n"
				   "\n"
				   "\t/* Marshal an array onto the message */\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"s\", &output_iter)) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tfor (size_t output_i = 0; output[output_i]; output_i++) {\n"
				   "\t\tconst char * output_element;\n"
				   "\n"
				   "\t\toutput_element = output[output_i];\n"
				   "\n"
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&output_iter, DBUS_TYPE_STRING, &output_element)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &output_iter)) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the reply, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

		nih_free (str);
		nih_free (method);
	}


	/* Check that we can use the generated code to reply to a method
	 * call we created, and that we can receive the reply.
	 */
	TEST_FEATURE ("with reply (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"com.netsplit.Nih",
			"MyMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
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
			"com.netsplit.Nih",
			"MyMethod");

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
			message->conn = client_conn;
			message->message = method_call;

			object = nih_new (NULL, NihDBusObject);
			object->path = "/com/netsplit/Nih";
			object->conn = client_conn;
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
			"com.netsplit.Nih",
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


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
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
	test_lookup_argument ();

	test_object_function ();
	test_reply_function ();

	return 0;
}
