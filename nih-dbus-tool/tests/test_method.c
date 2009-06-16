/* nih-dbus-tool
 *
 * test_method.c - test suite for nih-dbus-tool/method.c
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
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);

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
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node_new (NULL, NULL));

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
	int          ret;
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
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);

			other = method_new (interface, "Test");
			other->symbol = nih_strdup (other, "test_method");
			nih_list_add (&interface->methods, &other->entry);

			method = method_new (NULL, "TestMethod");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
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

			method = method_new (NULL, "TestMethod");
			method->no_reply = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);

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

			method = method_new (NULL, "TestMethod");
			method->no_reply = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);

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

			method = method_new (NULL, "TestMethod");
			method->async = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
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

			method = method_new (NULL, "TestMethod");
			method->async = TRUE;
			method->no_reply = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);
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

			method = method_new (NULL, "TestMethod");
			method->async = TRUE;
			method->no_reply = TRUE;
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_METHOD, method);

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
	Interface *       interface = NULL;
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
	NihListEntry *    attrib;
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
		nih_list_init (&prototypes);
		nih_list_init (&handlers);

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
					      &prototypes, &handlers);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Method_method (NihDBusObject * object,\n"
				   "                                        NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar *          str;\n"
				   "\tconst char *    str_dbus;\n"
				   "\tint32_t         flags;\n"
				   "\tchar **         output;\n"
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
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
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
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
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
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\tif (my_method (object->data, message, str, flags, &output) < 0) {\n"
				   "\t\tNihError *err;\n"
				   "\n"
				   "\t\terr = nih_error_get ();\n"
				   "\t\tif (err->number == ENOMEM) {\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t} else if (err->number == NIH_DBUS_ERROR) {\n"
				   "\t\t\tNihDBusError *dbus_err = (NihDBusError *)err;\n"
				   "\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t} else {\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\t/* If the sender doesn't care about a reply, don't bother wasting\n"
				   "\t * effort constructing and sending one.\n"
				   "\t */\n"
				   "\tif (dbus_message_get_no_reply (message->message))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Construct the reply message. */\n"
				   "\t\treply = dbus_message_new_method_return (message->message);\n"
				   "\t\tif (! reply)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init_append (reply, &iter);\n"
				   "\n"
				   "\t\t/* Marshal an array onto the message */\n"
				   "\t\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"s\", &output_iter)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treply = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tfor (size_t output_i = 0; output[output_i]; output_i++) {\n"
				   "\t\t\tconst char *output_element;\n"
				   "\n"
				   "\t\t\toutput_element = output[output_i];\n"
				   "\n"
				   "\t\t\t/* Marshal a char * onto the message */\n"
				   "\t\t\tif (! dbus_message_iter_append_basic (&output_iter, DBUS_TYPE_STRING, &output_element)) {\n"
				   "\t\t\t\tdbus_message_iter_abandon_container (&iter, &output_iter);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\treply = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tif (! dbus_message_iter_close_container (&iter, &output_iter)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treply = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! reply);\n"
				   "\n"
				   "\t/* Send the reply, appending it to the outgoing queue. */\n"
				   "\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "}\n"));

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
					      &prototypes, &handlers);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Method_method (NihDBusObject * object,\n"
				   "                                        NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar **         output;\n"
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
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\tif (my_method (object->data, message, &output) < 0) {\n"
				   "\t\tNihError *err;\n"
				   "\n"
				   "\t\terr = nih_error_get ();\n"
				   "\t\tif (err->number == ENOMEM) {\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t} else if (err->number == NIH_DBUS_ERROR) {\n"
				   "\t\t\tNihDBusError *dbus_err = (NihDBusError *)err;\n"
				   "\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t} else {\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\t/* If the sender doesn't care about a reply, don't bother wasting\n"
				   "\t * effort constructing and sending one.\n"
				   "\t */\n"
				   "\tif (dbus_message_get_no_reply (message->message))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Construct the reply message. */\n"
				   "\t\treply = dbus_message_new_method_return (message->message);\n"
				   "\t\tif (! reply)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init_append (reply, &iter);\n"
				   "\n"
				   "\t\t/* Marshal an array onto the message */\n"
				   "\t\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"s\", &output_iter)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treply = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tfor (size_t output_i = 0; output[output_i]; output_i++) {\n"
				   "\t\t\tconst char *output_element;\n"
				   "\n"
				   "\t\t\toutput_element = output[output_i];\n"
				   "\n"
				   "\t\t\t/* Marshal a char * onto the message */\n"
				   "\t\t\tif (! dbus_message_iter_append_basic (&output_iter, DBUS_TYPE_STRING, &output_element)) {\n"
				   "\t\t\t\tdbus_message_iter_abandon_container (&iter, &output_iter);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\treply = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tif (! dbus_message_iter_close_container (&iter, &output_iter)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treply = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! reply);\n"
				   "\n"
				   "\t/* Send the reply, appending it to the outgoing queue. */\n"
				   "\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "}\n"));

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
					      &prototypes, &handlers);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Method_method (NihDBusObject * object,\n"
				   "                                        NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar *          str;\n"
				   "\tconst char *    str_dbus;\n"
				   "\tint32_t         flags;\n"
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
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
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
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
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
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\tif (my_method (object->data, message, str, flags) < 0) {\n"
				   "\t\tNihError *err;\n"
				   "\n"
				   "\t\terr = nih_error_get ();\n"
				   "\t\tif (err->number == ENOMEM) {\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t} else if (err->number == NIH_DBUS_ERROR) {\n"
				   "\t\t\tNihDBusError *dbus_err = (NihDBusError *)err;\n"
				   "\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t} else {\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\t/* If the sender doesn't care about a reply, don't bother wasting\n"
				   "\t * effort constructing and sending one.\n"
				   "\t */\n"
				   "\tif (dbus_message_get_no_reply (message->message))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Construct the reply message. */\n"
				   "\t\treply = dbus_message_new_method_return (message->message);\n"
				   "\t\tif (! reply)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init_append (reply, &iter);\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! reply);\n"
				   "\n"
				   "\t/* Send the reply, appending it to the outgoing queue. */\n"
				   "\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "}\n"));

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

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");
		}

		str = method_object_function (NULL, "my", interface, method,
					      &prototypes, &handlers);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Method_method (NihDBusObject * object,\n"
				   "                                        NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\tif (my_method (object->data, message) < 0) {\n"
				   "\t\tNihError *err;\n"
				   "\n"
				   "\t\terr = nih_error_get ();\n"
				   "\t\tif (err->number == ENOMEM) {\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t} else if (err->number == NIH_DBUS_ERROR) {\n"
				   "\t\t\tNihDBusError *dbus_err = (NihDBusError *)err;\n"
				   "\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t} else {\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\t/* If the sender doesn't care about a reply, don't bother wasting\n"
				   "\t * effort constructing and sending one.\n"
				   "\t */\n"
				   "\tif (dbus_message_get_no_reply (message->message))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Construct the reply message. */\n"
				   "\t\treply = dbus_message_new_method_return (message->message);\n"
				   "\t\tif (! reply)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init_append (reply, &iter);\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! reply);\n"
				   "\n"
				   "\t/* Send the reply, appending it to the outgoing queue. */\n"
				   "\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "}\n"));

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
					      &prototypes, &handlers);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_AsyncMethod_method (NihDBusObject * object,\n"
				   "                                             NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar *          str;\n"
				   "\tconst char *    str_dbus;\n"
				   "\tint32_t         flags;\n"
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
				   "\t\t                                _(\"Invalid arguments to AsyncMethod method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
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
				   "\t\t                                _(\"Invalid arguments to AsyncMethod method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
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
				   "\t\t                                _(\"Invalid arguments to AsyncMethod method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\tif (my_async_method (object->data, message, str, flags) < 0) {\n"
				   "\t\tNihError *err;\n"
				   "\n"
				   "\t\terr = nih_error_get ();\n"
				   "\t\tif (err->number == ENOMEM) {\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t} else if (err->number == NIH_DBUS_ERROR) {\n"
				   "\t\t\tNihDBusError *dbus_err = (NihDBusError *)err;\n"
				   "\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t} else {\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "}\n"));

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
					      &prototypes, &handlers);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Method_method (NihDBusObject * object,\n"
				   "                                        NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar *          str;\n"
				   "\tconst char *    str_dbus;\n"
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
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
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
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Method method\"));\n"
				   "\t\tif (! reply)\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\tif (my_method (object->data, message, str) < 0) {\n"
				   "\t\tNihError *err;\n"
				   "\n"
				   "\t\terr = nih_error_get ();\n"
				   "\t\tif (err->number == ENOMEM) {\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t\t} else if (err->number == NIH_DBUS_ERROR) {\n"
				   "\t\t\tNihDBusError *dbus_err = (NihDBusError *)err;\n"
				   "\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t} else {\n"
				   "\t\t\treply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));\n"
				   "\t\t\tnih_free (err);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\t/* If the sender doesn't care about a reply, don't bother wasting\n"
				   "\t * effort constructing and sending one.\n"
				   "\t */\n"
				   "\tif (dbus_message_get_no_reply (message->message))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Construct the reply message. */\n"
				   "\t\treply = dbus_message_new_method_return (message->message);\n"
				   "\t\tif (! reply)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init_append (reply, &iter);\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! reply);\n"
				   "\n"
				   "\t/* Send the reply, appending it to the outgoing queue. */\n"
				   "\tNIH_MUST (dbus_connection_send (message->connection, reply, NULL));\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				   "}\n"));

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
	Interface *       interface = NULL;
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	char *            str;
	int32_t           flags;
	char **           output;
	TypeFunc *        func;
	TypeVar *         arg;
	NihListEntry *    attrib;
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
		nih_list_init (&prototypes);

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
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_async_method_reply (NihDBusMessage *message,\n"
				   "                       char * const *  output)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
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
				   "\t\tconst char *output_element;\n"
				   "\n"
				   "\t\toutput_element = output[output_i];\n"
				   "\n"
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&output_iter, DBUS_TYPE_STRING, &output_element)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &output_iter);\n"
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
				   "\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "AsyncMethod");
			method->symbol = nih_strdup (method, "async_method");
		}

		str = method_reply_function (NULL, "my", interface, method,
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_async_method_reply (NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (message != NULL);\n"
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
				   "\t/* Send the reply, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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

		nih_free (str);
		nih_free (method);
		nih_free (interface);
	}


	/* Check that an array argument may be NULL when the size is non-zero
	 */
	TEST_FEATURE ("with array argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

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
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_async_method_reply (NihDBusMessage *message,\n"
				   "                       const int32_t * output,\n"
				   "                       size_t          output_len)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter output_iter;\n"
				   "\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert ((output_len == 0) || (output != NULL));\n"
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
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"i\", &output_iter)) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tfor (size_t output_i = 0; output_i < output_len; output_i++) {\n"
				   "\t\tint32_t output_element;\n"
				   "\n"
				   "\t\toutput_element = output[output_i];\n"
				   "\n"
				   "\t\t/* Marshal a int32_t onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&output_iter, DBUS_TYPE_INT32, &output_element)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &output_iter);\n"
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
				   "\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_async_method_reply (NihDBusMessage *message,\n"
				   "                       char * const *  output)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
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
				   "\t\tconst char *output_element;\n"
				   "\n"
				   "\t\toutput_element = output[output_i];\n"
				   "\n"
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&output_iter, DBUS_TYPE_STRING, &output_element)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &output_iter);\n"
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
				   "\tif (! dbus_connection_send (message->connection, reply, NULL)) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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
	Interface *       interface = NULL;
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	Argument *        argument4 = NULL;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
	NihListEntry *    attrib;
	DBusConnection *  flakey_conn;
	NihDBusProxy *    proxy = NULL;
	DBusPendingCall * pending_call;
	DBusMessage *     method_call;
	DBusMessage *     reply;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	char *            str_value;
	int32_t           int32_value;
	NihError *        err;

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
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusPendingCall *\n"
				   "my_test_method (NihDBusProxy *      proxy,\n"
				   "                const char *        str,\n"
				   "                int32_t             flags,\n"
				   "                MyTestMethodReply   handler,\n"
				   "                NihDBusErrorHandler error_handler,\n"
				   "                void *              data,\n"
				   "                int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (str != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"TestMethod\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &str)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &flags)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Handle a fire-and-forget message */\n"
				   "\tif (! error_handler) {\n"
				   "\t\tdbus_message_set_no_reply (method_call, TRUE);\n"
				   "\t\tif (! dbus_connection_send (proxy->connection, method_call, NULL)) {\n"
				   "\t\t\tdbus_message_unref (method_call);\n"
				   "\t\t\tnih_return_no_memory_error (NULL);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\treturn (DBusPendingCall *)TRUE;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the message and set up the reply notification. */\n"
				   "\tpending_data = nih_dbus_pending_data_new (NULL, proxy->connection,\n"
				   "\t                                          (NihDBusReplyHandler)handler,\n"
				   "\t                                          error_handler, data);\n"
				   "\tif (! pending_data) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tpending_call = NULL;\n"
				   "\tif (! dbus_connection_send_with_reply (proxy->connection, method_call,\n"
				   "\t                                       &pending_call, timeout)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_free (pending_data);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_TestMethod_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"));

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

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "TestMethod");
			method->symbol = nih_strdup (method, "test_method");
		}

		str = method_proxy_function (NULL, "my", interface, method,
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusPendingCall *\n"
				   "my_test_method (NihDBusProxy *      proxy,\n"
				   "                MyTestMethodReply   handler,\n"
				   "                NihDBusErrorHandler error_handler,\n"
				   "                void *              data,\n"
				   "                int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"TestMethod\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Handle a fire-and-forget message */\n"
				   "\tif (! error_handler) {\n"
				   "\t\tdbus_message_set_no_reply (method_call, TRUE);\n"
				   "\t\tif (! dbus_connection_send (proxy->connection, method_call, NULL)) {\n"
				   "\t\t\tdbus_message_unref (method_call);\n"
				   "\t\t\tnih_return_no_memory_error (NULL);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\treturn (DBusPendingCall *)TRUE;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the message and set up the reply notification. */\n"
				   "\tpending_data = nih_dbus_pending_data_new (NULL, proxy->connection,\n"
				   "\t                                          (NihDBusReplyHandler)handler,\n"
				   "\t                                          error_handler, data);\n"
				   "\tif (! pending_data) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tpending_call = NULL;\n"
				   "\tif (! dbus_connection_send_with_reply (proxy->connection, method_call,\n"
				   "\t                                       &pending_call, timeout)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_free (pending_data);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_TestMethod_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"));

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
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusPendingCall *\n"
				   "my_test_method (NihDBusProxy *      proxy,\n"
				   "                const int32_t *     value,\n"
				   "                size_t              value_len,\n"
				   "                MyTestMethodReply   handler,\n"
				   "                NihDBusErrorHandler error_handler,\n"
				   "                void *              data,\n"
				   "                int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tDBusMessageIter     value_iter;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((value_len == 0) || (value != NULL));\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"TestMethod\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal an array onto the message */\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"i\", &value_iter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tfor (size_t value_i = 0; value_i < value_len; value_i++) {\n"
				   "\t\tint32_t value_element;\n"
				   "\n"
				   "\t\tvalue_element = value[value_i];\n"
				   "\n"
				   "\t\t/* Marshal a int32_t onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_INT32, &value_element)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\tdbus_message_unref (method_call);\n"
				   "\t\t\tnih_return_no_memory_error (NULL);\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &value_iter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Handle a fire-and-forget message */\n"
				   "\tif (! error_handler) {\n"
				   "\t\tdbus_message_set_no_reply (method_call, TRUE);\n"
				   "\t\tif (! dbus_connection_send (proxy->connection, method_call, NULL)) {\n"
				   "\t\t\tdbus_message_unref (method_call);\n"
				   "\t\t\tnih_return_no_memory_error (NULL);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\treturn (DBusPendingCall *)TRUE;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the message and set up the reply notification. */\n"
				   "\tpending_data = nih_dbus_pending_data_new (NULL, proxy->connection,\n"
				   "\t                                          (NihDBusReplyHandler)handler,\n"
				   "\t                                          error_handler, data);\n"
				   "\tif (! pending_data) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tpending_call = NULL;\n"
				   "\tif (! dbus_connection_send_with_reply (proxy->connection, method_call,\n"
				   "\t                                       &pending_call, timeout)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_free (pending_data);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_TestMethod_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"));

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
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("DBusPendingCall *\n"
				   "my_test_method (NihDBusProxy *      proxy,\n"
				   "                const char *        str,\n"
				   "                int32_t             flags,\n"
				   "                MyTestMethodReply   handler,\n"
				   "                NihDBusErrorHandler error_handler,\n"
				   "                void *              data,\n"
				   "                int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (str != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"TestMethod\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &str)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &flags)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Handle a fire-and-forget message */\n"
				   "\tif (! error_handler) {\n"
				   "\t\tdbus_message_set_no_reply (method_call, TRUE);\n"
				   "\t\tif (! dbus_connection_send (proxy->connection, method_call, NULL)) {\n"
				   "\t\t\tdbus_message_unref (method_call);\n"
				   "\t\t\tnih_return_no_memory_error (NULL);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\treturn (DBusPendingCall *)TRUE;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the message and set up the reply notification. */\n"
				   "\tpending_data = nih_dbus_pending_data_new (NULL, proxy->connection,\n"
				   "\t                                          (NihDBusReplyHandler)handler,\n"
				   "\t                                          error_handler, data);\n"
				   "\tif (! pending_data) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tpending_call = NULL;\n"
				   "\tif (! dbus_connection_send_with_reply (proxy->connection, method_call,\n"
				   "\t                                       &pending_call, timeout)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_free (pending_data);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_TestMethod_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"));

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
	Interface *         interface = NULL;
	Method *            method = NULL;
	Argument *          argument1 = NULL;
	Argument *          argument2 = NULL;
	Argument *          argument3 = NULL;
	Argument *          argument4 = NULL;
	char *              str;
	TypeFunc *          func;
	TypeVar *           arg;
	dbus_uint32_t       serial;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data;
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
						    &prototypes, &typedefs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("void\n"
				   "my_com_netsplit_Nih_Test_Method_notify (DBusPendingCall *   pending_call,\n"
				   "                                        NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tchar **         output;\n"
				   "\tDBusMessageIter output_iter;\n"
				   "\tsize_t          output_size;\n"
				   "\tint32_t         length;\n"
				   "\n"
				   "\tnih_assert (pending_call != NULL);\n"
				   "\tnih_assert (pending_data != NULL);\n"
				   "\n"
				   "\tnih_assert (dbus_pending_call_get_completed (pending_call));\n"
				   "\n"
				   "\t/* Steal the reply from the pending call. */\n"
				   "\treply = dbus_pending_call_steal_reply (pending_call);\n"
				   "\tnih_assert (reply != NULL);\n"
				   "\n"
				   "\t/* Handle error replies */\n"
				   "\tif (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {\n"
				   "\t\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\n"
				   "\t\tdbus_error_init (&error);\n"
				   "\t\tdbus_set_error_from_message (&error, message->message);\n"
				   "\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_dbus_error_raise (error.name, error.message);\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tdbus_error_free (&error);\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\tnih_assert (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_METHOD_RETURN);\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Create a message context for the reply, and iterate\n"
				   "\t\t * over its arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\t/* Demarshal an array from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\t\t\tnih_error_push_context ();\n"
				   "\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_recurse (&iter, &output_iter);\n"
				   "\n"
				   "\t\toutput_size = 0;\n"
				   "\n"
				   "\t\toutput = nih_alloc (message, sizeof (char *));\n"
				   "\t\tif (! output) {\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tmessage = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\toutput[output_size] = NULL;\n"
				   "\n"
				   "\t\twhile (dbus_message_iter_get_arg_type (&output_iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\t\tconst char *output_element_dbus;\n"
				   "\t\t\tchar **     output_tmp;\n"
				   "\t\t\tchar *      output_element;\n"
				   "\n"
				   "\t\t\t/* Demarshal a char * from the message */\n"
				   "\t\t\tif (dbus_message_iter_get_arg_type (&output_iter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\t\tif (output)\n"
				   "\t\t\t\t\tnih_free (output);\n"
				   "\t\t\t\tnih_error_push_context ();\n"
				   "\t\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\t\tnih_free (message);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\treturn;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\tdbus_message_iter_get_basic (&output_iter, &output_element_dbus);\n"
				   "\n"
				   "\t\t\toutput_element = nih_strdup (output, output_element_dbus);\n"
				   "\t\t\tif (! output_element) {\n"
				   "\t\t\t\tif (output)\n"
				   "\t\t\t\t\tnih_free (output);\n"
				   "\t\t\t\tnih_free (message);\n"
				   "\t\t\t\tmessage = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\tdbus_message_iter_next (&output_iter);\n"
				   "\n"
				   "\t\t\tif (output_size + 2 > SIZE_MAX / sizeof (char *)) {\n"
				   "\t\t\t\tif (output)\n"
				   "\t\t\t\t\tnih_free (output);\n"
				   "\t\t\t\tnih_error_push_context ();\n"
				   "\t\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\t\tnih_free (message);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\treturn;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\toutput_tmp = nih_realloc (output, message, sizeof (char *) * (output_size + 2));\n"
				   "\t\t\tif (! output_tmp) {\n"
				   "\t\t\t\tif (output)\n"
				   "\t\t\t\t\tnih_free (output);\n"
				   "\t\t\t\tnih_free (message);\n"
				   "\t\t\t\tmessage = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\toutput = output_tmp;\n"
				   "\t\t\toutput[output_size] = output_element;\n"
				   "\t\t\toutput[output_size + 1] = NULL;\n"
				   "\n"
				   "\t\t\toutput_size++;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\t/* Demarshal a int32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\t\t\tnih_error_push_context ();\n"
				   "\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&iter, &length);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\t\tnih_error_push_context ();\n"
				   "\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn;\n"
				   "\t\t}\n"
				   "\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! message);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyMethodReply)pending_data->handler) (pending_data->data, message, output, length);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"));

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

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");
		}

		str = method_proxy_notify_function (NULL, "my", interface,
						    method,
						    &prototypes, &typedefs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("void\n"
				   "my_com_netsplit_Nih_Test_Method_notify (DBusPendingCall *   pending_call,\n"
				   "                                        NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\n"
				   "\tnih_assert (pending_call != NULL);\n"
				   "\tnih_assert (pending_data != NULL);\n"
				   "\n"
				   "\tnih_assert (dbus_pending_call_get_completed (pending_call));\n"
				   "\n"
				   "\t/* Steal the reply from the pending call. */\n"
				   "\treply = dbus_pending_call_steal_reply (pending_call);\n"
				   "\tnih_assert (reply != NULL);\n"
				   "\n"
				   "\t/* Handle error replies */\n"
				   "\tif (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {\n"
				   "\t\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\n"
				   "\t\tdbus_error_init (&error);\n"
				   "\t\tdbus_set_error_from_message (&error, message->message);\n"
				   "\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_dbus_error_raise (error.name, error.message);\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tdbus_error_free (&error);\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\tnih_assert (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_METHOD_RETURN);\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Create a message context for the reply, and iterate\n"
				   "\t\t * over its arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\t\tnih_error_push_context ();\n"
				   "\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn;\n"
				   "\t\t}\n"
				   "\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! message);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyMethodReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"));

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
						    &prototypes, &typedefs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("void\n"
				   "my_com_netsplit_Nih_Test_Method_notify (DBusPendingCall *   pending_call,\n"
				   "                                        NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tchar **         output;\n"
				   "\tDBusMessageIter output_iter;\n"
				   "\tsize_t          output_size;\n"
				   "\tint32_t         length;\n"
				   "\n"
				   "\tnih_assert (pending_call != NULL);\n"
				   "\tnih_assert (pending_data != NULL);\n"
				   "\n"
				   "\tnih_assert (dbus_pending_call_get_completed (pending_call));\n"
				   "\n"
				   "\t/* Steal the reply from the pending call. */\n"
				   "\treply = dbus_pending_call_steal_reply (pending_call);\n"
				   "\tnih_assert (reply != NULL);\n"
				   "\n"
				   "\t/* Handle error replies */\n"
				   "\tif (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {\n"
				   "\t\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\n"
				   "\t\tdbus_error_init (&error);\n"
				   "\t\tdbus_set_error_from_message (&error, message->message);\n"
				   "\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_dbus_error_raise (error.name, error.message);\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tdbus_error_free (&error);\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\tnih_assert (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_METHOD_RETURN);\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Create a message context for the reply, and iterate\n"
				   "\t\t * over its arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\t/* Demarshal an array from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\t\t\tnih_error_push_context ();\n"
				   "\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_recurse (&iter, &output_iter);\n"
				   "\n"
				   "\t\toutput_size = 0;\n"
				   "\n"
				   "\t\toutput = nih_alloc (message, sizeof (char *));\n"
				   "\t\tif (! output) {\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tmessage = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\toutput[output_size] = NULL;\n"
				   "\n"
				   "\t\twhile (dbus_message_iter_get_arg_type (&output_iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\t\tconst char *output_element_dbus;\n"
				   "\t\t\tchar **     output_tmp;\n"
				   "\t\t\tchar *      output_element;\n"
				   "\n"
				   "\t\t\t/* Demarshal a char * from the message */\n"
				   "\t\t\tif (dbus_message_iter_get_arg_type (&output_iter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\t\tif (output)\n"
				   "\t\t\t\t\tnih_free (output);\n"
				   "\t\t\t\tnih_error_push_context ();\n"
				   "\t\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\t\tnih_free (message);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\treturn;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\tdbus_message_iter_get_basic (&output_iter, &output_element_dbus);\n"
				   "\n"
				   "\t\t\toutput_element = nih_strdup (output, output_element_dbus);\n"
				   "\t\t\tif (! output_element) {\n"
				   "\t\t\t\tif (output)\n"
				   "\t\t\t\t\tnih_free (output);\n"
				   "\t\t\t\tnih_free (message);\n"
				   "\t\t\t\tmessage = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\tdbus_message_iter_next (&output_iter);\n"
				   "\n"
				   "\t\t\tif (output_size + 2 > SIZE_MAX / sizeof (char *)) {\n"
				   "\t\t\t\tif (output)\n"
				   "\t\t\t\t\tnih_free (output);\n"
				   "\t\t\t\tnih_error_push_context ();\n"
				   "\t\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\t\tnih_free (message);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\treturn;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\toutput_tmp = nih_realloc (output, message, sizeof (char *) * (output_size + 2));\n"
				   "\t\t\tif (! output_tmp) {\n"
				   "\t\t\t\tif (output)\n"
				   "\t\t\t\t\tnih_free (output);\n"
				   "\t\t\t\tnih_free (message);\n"
				   "\t\t\t\tmessage = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\toutput = output_tmp;\n"
				   "\t\t\toutput[output_size] = output_element;\n"
				   "\t\t\toutput[output_size + 1] = NULL;\n"
				   "\n"
				   "\t\t\toutput_size++;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\t/* Demarshal a int32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\t\t\tnih_error_push_context ();\n"
				   "\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&iter, &length);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\t\tnih_error_push_context ();\n"
				   "\t\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\treturn;\n"
				   "\t\t}\n"
				   "\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! message);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyMethodReply)pending_data->handler) (pending_data->data, message, output, length);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"));

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
	NihList           prototypes;
	Interface *       interface = NULL;
	Method *          method = NULL;
	Argument *        argument1 = NULL;
	Argument *        argument2 = NULL;
	Argument *        argument3 = NULL;
	Argument *        argument4 = NULL;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
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
						  &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_method_sync (const void *  parent,\n"
				   "                NihDBusProxy *proxy,\n"
				   "                const char *  str,\n"
				   "                int32_t       flags,\n"
				   "                char ***      output,\n"
				   "                int32_t *     length)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar **         output_local;\n"
				   "\tDBusMessageIter output_local_iter;\n"
				   "\tsize_t          output_local_size;\n"
				   "\tint32_t         length_local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (str != NULL);\n"
				   "\tnih_assert (output != NULL);\n"
				   "\tnih_assert (length != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Method\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &str)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &flags)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the message, and wait for the reply. */\n"
				   "\tdbus_error_init (&error);\n"
				   "\n"
				   "\treply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call, -1, &error);\n"
				   "\tif (! reply) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t\tif (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) {\n"
				   "\t\t\tnih_error_raise_no_memory ();\n"
				   "\t\t} else {\n"
				   "\t\t\tnih_dbus_error_raise (error.name, error.message);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_error_free (&error);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t/* Iterate the arguments of the reply */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal an array from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_recurse (&iter, &output_local_iter);\n"
				   "\n"
				   "\t\toutput_local_size = 0;\n"
				   "\n"
				   "\t\toutput_local = nih_alloc (parent, sizeof (char *));\n"
				   "\t\tif (! output_local) {\n"
				   "\t\t\t*output = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\toutput_local[output_local_size] = NULL;\n"
				   "\n"
				   "\t\twhile (dbus_message_iter_get_arg_type (&output_local_iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\t\tconst char *output_local_element_dbus;\n"
				   "\t\t\tchar **     output_local_tmp;\n"
				   "\t\t\tchar *      output_local_element;\n"
				   "\n"
				   "\t\t\t/* Demarshal a char * from the message */\n"
				   "\t\t\tif (dbus_message_iter_get_arg_type (&output_local_iter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\t\tif (output_local)\n"
				   "\t\t\t\t\tnih_free (output_local);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\tdbus_message_iter_get_basic (&output_local_iter, &output_local_element_dbus);\n"
				   "\n"
				   "\t\t\toutput_local_element = nih_strdup (output_local, output_local_element_dbus);\n"
				   "\t\t\tif (! output_local_element) {\n"
				   "\t\t\t\tif (output_local)\n"
				   "\t\t\t\t\tnih_free (output_local);\n"
				   "\t\t\t\t*output = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\tdbus_message_iter_next (&output_local_iter);\n"
				   "\n"
				   "\t\t\tif (output_local_size + 2 > SIZE_MAX / sizeof (char *)) {\n"
				   "\t\t\t\tif (output_local)\n"
				   "\t\t\t\t\tnih_free (output_local);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\toutput_local_tmp = nih_realloc (output_local, parent, sizeof (char *) * (output_local_size + 2));\n"
				   "\t\t\tif (! output_local_tmp) {\n"
				   "\t\t\t\tif (output_local)\n"
				   "\t\t\t\t\tnih_free (output_local);\n"
				   "\t\t\t\t*output = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\toutput_local = output_local_tmp;\n"
				   "\t\t\toutput_local[output_local_size] = output_local_element;\n"
				   "\t\t\toutput_local[output_local_size + 1] = NULL;\n"
				   "\n"
				   "\t\t\toutput_local_size++;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\t*output = output_local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *output);\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a int32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\t\t\tnih_free (output_local);\n"
				   "\t\t\t*output = NULL;\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&iter, &length_local);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\t*length = length_local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *length);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (output_local);\n"
				   "\t\t*output = NULL;\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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
						  &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_method_sync (const void *  parent,\n"
				   "                NihDBusProxy *proxy,\n"
				   "                char ***      output,\n"
				   "                int32_t *     length)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar **         output_local;\n"
				   "\tDBusMessageIter output_local_iter;\n"
				   "\tsize_t          output_local_size;\n"
				   "\tint32_t         length_local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (output != NULL);\n"
				   "\tnih_assert (length != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Method\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Send the message, and wait for the reply. */\n"
				   "\tdbus_error_init (&error);\n"
				   "\n"
				   "\treply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call, -1, &error);\n"
				   "\tif (! reply) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t\tif (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) {\n"
				   "\t\t\tnih_error_raise_no_memory ();\n"
				   "\t\t} else {\n"
				   "\t\t\tnih_dbus_error_raise (error.name, error.message);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_error_free (&error);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t/* Iterate the arguments of the reply */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal an array from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_recurse (&iter, &output_local_iter);\n"
				   "\n"
				   "\t\toutput_local_size = 0;\n"
				   "\n"
				   "\t\toutput_local = nih_alloc (parent, sizeof (char *));\n"
				   "\t\tif (! output_local) {\n"
				   "\t\t\t*output = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\toutput_local[output_local_size] = NULL;\n"
				   "\n"
				   "\t\twhile (dbus_message_iter_get_arg_type (&output_local_iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\t\tconst char *output_local_element_dbus;\n"
				   "\t\t\tchar **     output_local_tmp;\n"
				   "\t\t\tchar *      output_local_element;\n"
				   "\n"
				   "\t\t\t/* Demarshal a char * from the message */\n"
				   "\t\t\tif (dbus_message_iter_get_arg_type (&output_local_iter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\t\tif (output_local)\n"
				   "\t\t\t\t\tnih_free (output_local);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\tdbus_message_iter_get_basic (&output_local_iter, &output_local_element_dbus);\n"
				   "\n"
				   "\t\t\toutput_local_element = nih_strdup (output_local, output_local_element_dbus);\n"
				   "\t\t\tif (! output_local_element) {\n"
				   "\t\t\t\tif (output_local)\n"
				   "\t\t\t\t\tnih_free (output_local);\n"
				   "\t\t\t\t*output = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\tdbus_message_iter_next (&output_local_iter);\n"
				   "\n"
				   "\t\t\tif (output_local_size + 2 > SIZE_MAX / sizeof (char *)) {\n"
				   "\t\t\t\tif (output_local)\n"
				   "\t\t\t\t\tnih_free (output_local);\n"
				   "\t\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\toutput_local_tmp = nih_realloc (output_local, parent, sizeof (char *) * (output_local_size + 2));\n"
				   "\t\t\tif (! output_local_tmp) {\n"
				   "\t\t\t\tif (output_local)\n"
				   "\t\t\t\t\tnih_free (output_local);\n"
				   "\t\t\t\t*output = NULL;\n"
				   "\t\t\t\tgoto enomem;\n"
				   "\t\t\t}\n"
				   "\n"
				   "\t\t\toutput_local = output_local_tmp;\n"
				   "\t\t\toutput_local[output_local_size] = output_local_element;\n"
				   "\t\t\toutput_local[output_local_size + 1] = NULL;\n"
				   "\n"
				   "\t\t\toutput_local_size++;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\t*output = output_local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *output);\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a int32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\t\t\tnih_free (output_local);\n"
				   "\t\t\t*output = NULL;\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&iter, &length_local);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\t*length = length_local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *length);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (output_local);\n"
				   "\t\t*output = NULL;\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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
						  &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_method_sync (const void *  parent,\n"
				   "                NihDBusProxy *proxy,\n"
				   "                const char *  str,\n"
				   "                int32_t       flags)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (str != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Method\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &str)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &flags)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the message, and wait for the reply. */\n"
				   "\tdbus_error_init (&error);\n"
				   "\n"
				   "\treply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call, -1, &error);\n"
				   "\tif (! reply) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t\tif (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) {\n"
				   "\t\t\tnih_error_raise_no_memory ();\n"
				   "\t\t} else {\n"
				   "\t\t\tnih_dbus_error_raise (error.name, error.message);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_error_free (&error);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t/* Iterate the arguments of the reply */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			method = method_new (NULL, "Method");
			method->symbol = nih_strdup (method, "method");
		}

		str = method_proxy_sync_function (NULL, "my", interface,
						  method,
						  &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_method_sync (const void *  parent,\n"
				   "                NihDBusProxy *proxy)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Method\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Send the message, and wait for the reply. */\n"
				   "\tdbus_error_init (&error);\n"
				   "\n"
				   "\treply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call, -1, &error);\n"
				   "\tif (! reply) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t\tif (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) {\n"
				   "\t\t\tnih_error_raise_no_memory ();\n"
				   "\t\t} else {\n"
				   "\t\t\tnih_dbus_error_raise (error.name, error.message);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_error_free (&error);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t/* Iterate the arguments of the reply */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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
						  &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_method_sync (const void *   parent,\n"
				   "                NihDBusProxy * proxy,\n"
				   "                const int32_t *value,\n"
				   "                size_t         value_len)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter value_iter;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((value_len == 0) || (value != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Method\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal an array onto the message */\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"i\", &value_iter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tfor (size_t value_i = 0; value_i < value_len; value_i++) {\n"
				   "\t\tint32_t value_element;\n"
				   "\n"
				   "\t\tvalue_element = value[value_i];\n"
				   "\n"
				   "\t\t/* Marshal a int32_t onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_INT32, &value_element)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\tdbus_message_unref (method_call);\n"
				   "\t\t\tnih_return_no_memory_error (-1);\n"
				   "\t\t}\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &value_iter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the message, and wait for the reply. */\n"
				   "\tdbus_error_init (&error);\n"
				   "\n"
				   "\treply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call, -1, &error);\n"
				   "\tif (! reply) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t\tif (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) {\n"
				   "\t\t\tnih_error_raise_no_memory ();\n"
				   "\t\t} else {\n"
				   "\t\t\tnih_dbus_error_raise (error.name, error.message);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_error_free (&error);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t/* Iterate the arguments of the reply */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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
						  &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (method);
			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_method_sync (const void *  parent,\n"
				   "                NihDBusProxy *proxy,\n"
				   "                int32_t       flags)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Method\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a int32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &flags)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the message, and wait for the reply. */\n"
				   "\tdbus_error_init (&error);\n"
				   "\n"
				   "\treply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call, -1, &error);\n"
				   "\tif (! reply) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t\tif (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) {\n"
				   "\t\t\tnih_error_raise_no_memory ();\n"
				   "\t\t} else {\n"
				   "\t\t\tnih_dbus_error_raise (error.name, error.message);\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_error_free (&error);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (method_call);\n"
				   "\n"
				   "\t/* Iterate the arguments of the reply */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

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
