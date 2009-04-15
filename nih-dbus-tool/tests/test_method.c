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
#include "argument.h"
#include "parse.h"
#include "errors.h"


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

	return 0;
}
