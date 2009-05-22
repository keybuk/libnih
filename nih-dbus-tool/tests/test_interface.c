/* nih-dbus-tool
 *
 * test_interface.c - test suite for nih-dbus-tool/interface.c
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
#include "interface.h"
#include "method.h"
#include "signal.h"
#include "property.h"
#include "parse.h"
#include "errors.h"


void
test_name_valid (void)
{
	TEST_FUNCTION ("interface_name_valid");

	/* Check that a typical interface name is valid. */
	TEST_FEATURE ("with typical interface name");
	TEST_TRUE (interface_name_valid ("com.netsplit.Nih.Test"));


	/* Check that an interface name is not valid if it is has an
	 * initial period.
	 */
	TEST_FEATURE ("with initial period");
	TEST_FALSE (interface_name_valid (".com.netsplit.Nih.Test"));


	/* Check that an interface name is not valid if it has multiple
	 * consecutive periods.
	 */
	TEST_FEATURE ("with consecutive periods");
	TEST_FALSE (interface_name_valid ("com..netsplit.Nih.Test"));


	/* Check that an interface name is not valid if it ends in a period */
	TEST_FEATURE ("with final period");
	TEST_FALSE (interface_name_valid ("com.netsplit.Nih.Test."));


	/* Check that an interface name is not valid if it only has one
	 * component
	 */
	TEST_FEATURE ("with only one component");
	TEST_FALSE (interface_name_valid ("com"));


	/* Check that an interface name is valid if it has two components.
	 */
	TEST_FEATURE ("with two components");
	TEST_TRUE (interface_name_valid ("com.netsplit"));


	/* Check that a interface name elements may contain numbers */
	TEST_FEATURE ("with numbers in interface name");
	TEST_TRUE (interface_name_valid ("com.netsplit.a43b.Test"));


	/* Check that a interface name elements may not begin with numbers */
	TEST_FEATURE ("with numbers starting interface name element");
	TEST_FALSE (interface_name_valid ("com.netsplit.43b.Test"));


	/* Check that the first interface name element may not begin
	 * with numbers
	 */
	TEST_FEATURE ("with numbers starting first interface name element");
	TEST_FALSE (interface_name_valid ("32com.netsplit.Nih.Test"));


	/* Check that a interface name elements may contain underscores */
	TEST_FEATURE ("with underscore in interface name");
	TEST_TRUE (interface_name_valid ("com.netsplit.Nih_Test"));


	/* Check that a interface name elements may begin with underscores */
	TEST_FEATURE ("with underscore starting interface name element");
	TEST_TRUE (interface_name_valid ("com.netsplit._Nih.Test"));


	/* Check that other characters are not permitted */
	TEST_FEATURE ("with non-permitted characters");
	TEST_FALSE (interface_name_valid ("com.netsplit/Nih.Test-Thing"));


	/* Check that an empty interface name is invalid */
	TEST_FEATURE ("with empty string");
	TEST_FALSE (interface_name_valid (""));


	/* Check that an interface name may not exceed 255 characters */
	TEST_FEATURE ("with overly long name");
	TEST_FALSE (interface_name_valid ("com.netsplit.Nih.ReallyLongInt"
					  "erfaceNameThatNobodyInTheirRig"
					  "htMindWouldEverUseButStillWeNe"
					  "edToTestThisKindOfShitSeriousl"
					  "yLookHowLongThisIs.IMeanYoureJ"
					  "ustNeverGoingToGetAnywhereNear"
					  "ThisLimitInFactIfTheLimitIsThi"
					  "sLongWhyHaveOneAtAllStupidSoft"
					  "ware.YayThereNow"));
}


void
test_new (void)
{
	Interface *interface;

	/* Check that an Interface object is allocated with the structure
	 * filled in properly, but not placed in a list.
	 */
	TEST_FUNCTION ("interface_new");
	TEST_ALLOC_FAIL {
		interface = interface_new (NULL, "com.netsplit.Nih.Test");

		if (test_alloc_failed) {
			TEST_EQ_P (interface, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_LIST_EMPTY (&interface->entry);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_P (interface->symbol, NULL);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_free (interface);
	}
}


void
test_start_tag (void)
{
	ParseContext context;
	ParseStack * parent = NULL;
	ParseStack * entry = NULL;
	XML_Parser   xmlp;
	Node *       node = NULL;
	Interface *  interface;
	char *       attr[5];
	int          ret;
	NihError *   err;
	FILE *       output;

	TEST_FUNCTION ("interface_start_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);

	output = tmpfile ();


	/* Check that an interface tag for an node with the usual name
	 * attribute results in an Interface member being created and pushed
	 * onto the stack with that attribute filled in correctly.
	 */
	TEST_FEATURE ("with interface");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);
		}

		attr[0] = "name";
		attr[1] = "com.netsplit.Nih.Test";
		attr[2] = NULL;

		ret = interface_start_tag (xmlp, "interface", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&node->interfaces);

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
		TEST_EQ (entry->type, PARSE_INTERFACE);

		interface = entry->interface;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, entry);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_P (interface->symbol, NULL);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a interface with a missing name attribute results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with missing name");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);

			attr[0] = NULL;
		}

		ret = interface_start_tag (xmlp, "interface", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&node->interfaces);

		err = nih_error_get ();
		TEST_EQ (err->number, INTERFACE_MISSING_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that a interface with an invalid name results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid name");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);

			attr[0] = "name";
			attr[1] = "Test Interface";
			attr[2] = NULL;
		}

		ret = interface_start_tag (xmlp, "interface", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&node->interfaces);

		err = nih_error_get ();
		TEST_EQ (err->number, INTERFACE_INVALID_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that an unknown interface attribute results in a warning
	 * being printed to standard error, but is otherwise ignored
	 * and the normal processing finished.
	 */
	TEST_FEATURE ("with unknown attribute");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);

			attr[0] = "name";
			attr[1] = "com.netsplit.Nih.Test";
			attr[2] = "frodo";
			attr[3] = "baggins";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = interface_start_tag (xmlp, "interface", attr);
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
		TEST_EQ (entry->type, PARSE_INTERFACE);

		interface = entry->interface;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, entry);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_P (interface->symbol, NULL);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		TEST_LIST_EMPTY (&node->interfaces);

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown <interface> attribute: "
				       "frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a interface on an empty stack (ie. a top-level
	 * interface element) results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with empty stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			attr[0] = "name";
			attr[1] = "com.netsplit.Nih.Test";
			attr[2] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = interface_start_tag (xmlp, "interface", attr);
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

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <interface> tag\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
	}


	/* Check that a interface on top of a stack entry that's not an
	 * node results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with non-node on stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method_new (NULL, "Test"));

			attr[0] = "name";
			attr[1] = "com.netsplit.Nih.Test";
			attr[2] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = interface_start_tag (xmlp, "interface", attr);
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

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <interface> tag\n");
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
	Node *       node = NULL;
	Interface *  interface = NULL;
	Interface *  other = NULL;
	int          ret;
	NihError *   err;

	TEST_FUNCTION ("interface_end_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);


	/* Check that when we parse the end tag for a interface, we pop
	 * the Interface object off the stack (freeing and removing it)
	 * and append it to the parent node's interfaces list, adding a
	 * reference to the node as well.  A symbol should be generated
	 * for the interface by taking the last part of the Interface
	 * name and convering it to C style.
	 */
	TEST_FEATURE ("with no assigned symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);

			interface = interface_new (NULL, "com.netsplit.Nih.TestInterface");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_INTERFACE, interface);
		}

		TEST_FREE_TAG (entry);

		ret = interface_end_tag (xmlp, "interface");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&node->interfaces);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (interface, node);

		TEST_LIST_NOT_EMPTY (&node->interfaces);
		TEST_EQ_P (node->interfaces.next, &interface->entry);

		TEST_EQ_STR (interface->symbol, "test_interface");
		TEST_ALLOC_PARENT (interface->symbol, interface);

		nih_free (parent);
	}


	/* Check that when the symbol has been pre-assigned by the data,
	 * it's not overriden and is used even if different.
	 */
	TEST_FEATURE ("with assigned symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);

			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = nih_strdup (interface, "foo");

			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_INTERFACE, interface);
		}

		TEST_FREE_TAG (entry);

		ret = interface_end_tag (xmlp, "interface");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&node->interfaces);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (interface, node);

		TEST_LIST_NOT_EMPTY (&node->interfaces);
		TEST_EQ_P (node->interfaces.next, &interface->entry);

		TEST_EQ_STR (interface->symbol, "foo");
		TEST_ALLOC_PARENT (interface->symbol, interface);

		nih_free (parent);
	}


	/* Check that we don't generate a duplicate symbol, and instead
	 * raise an error and allow the user to deal with it using
	 * the Symbol annotation.  The reason we don't work around this
	 * with a counter or similar is that the function names then
	 * become unpredictable (introspection data isn't ordered).
	 */
	TEST_FEATURE ("with conflicting symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);

			other = interface_new (node, "com.netsplit.Foo.Test");
			other->symbol = nih_strdup (other, "test");
			nih_list_add (&node->interfaces, &other->entry);

			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_INTERFACE, interface);
		}

		ret = interface_end_tag (xmlp, "interface");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		if ((! test_alloc_failed)
		    || (err->number != ENOMEM))
			TEST_EQ (err->number, INTERFACE_DUPLICATE_SYMBOL);
		nih_free (err);

		nih_free (entry);
		nih_free (parent);
	}


	XML_ParserFree (xmlp);
}


void
test_annotation (void)
{
	Interface *interface = NULL;
	char *     symbol;
	int        ret;
	NihError * err;

	TEST_FUNCTION ("interface_annotation");


	/* Check that the annotation to mark a interface as deprecated is
	 * handled, and the Interface is marked deprecated.
	 */
	TEST_FEATURE ("with deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
		}

		ret = interface_annotation (interface,
					    "org.freedesktop.DBus.Deprecated",
					    "true");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_FALSE (interface->deprecated);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (interface);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_TRUE (interface->deprecated);

		nih_free (interface);
	}


	/* Check that the annotation to mark a interface as deprecated can be
	 * given a false value to explicitly mark the Interface non-deprecated.
	 */
	TEST_FEATURE ("with explicitly non-deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->deprecated = TRUE;
		}

		ret = interface_annotation (interface,
					    "org.freedesktop.DBus.Deprecated",
					    "false");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_TRUE (interface->deprecated);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (interface);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FALSE (interface->deprecated);

		nih_free (interface);
	}


	/* Check that an annotation to add a symbol to the interface is
	 * handled, and the new symbol is stored in the interface.
	 */
	TEST_FEATURE ("with symbol annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
		}

		ret = interface_annotation (interface,
					    "com.netsplit.Nih.Symbol",
					    "foo");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (interface);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (interface->symbol, "foo");
		TEST_ALLOC_PARENT (interface->symbol, interface);

		nih_free (interface);
	}


	/* Check that an annotation to add a symbol to the interface
	 * replaces any previous symbol applied (e.g. by a previous
	 * annotation).
	 */
	TEST_FEATURE ("with symbol annotation and existing symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = nih_strdup (interface, "test_arg");
		}

		symbol = interface->symbol;
		TEST_FREE_TAG (symbol);

		ret = interface_annotation (interface,
					    "com.netsplit.Nih.Symbol",
					    "foo");


		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (interface);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (symbol);

		TEST_EQ_STR (interface->symbol, "foo");
		TEST_ALLOC_PARENT (interface->symbol, interface);

		nih_free (interface);
	}


	/* Check that an invalid value for the deprecated annotation results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid value for deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
		}

		ret = interface_annotation (interface,
					    "org.freedesktop.DBus.Deprecated",
					    "foo");

		TEST_LT (ret, 0);

		TEST_EQ_P (interface->symbol, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, INTERFACE_ILLEGAL_DEPRECATED);
		nih_free (err);

		nih_free (interface);
	}


	/* Check that an invalid symbol in an annotation results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid symbol in annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
		}

		ret = interface_annotation (interface,
					    "com.netsplit.Nih.Symbol",
					    "foo bar");

		TEST_LT (ret, 0);

		TEST_EQ_P (interface->symbol, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, INTERFACE_INVALID_SYMBOL);
		nih_free (err);

		nih_free (interface);
	}


	/* Check that an unknown annotation results in an error being
	 * raised.
	 */
	TEST_FEATURE ("with unknown annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
		}

		ret = interface_annotation (interface,
					    "com.netsplit.Nih.Unknown",
					    "true");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		TEST_EQ (err->number, INTERFACE_UNKNOWN_ANNOTATION);
		nih_free (err);

		nih_free (interface);
	}
}


void
test_lookup_method (void)
{
	Interface *interface = NULL;
	Method *   method1 = NULL;
	Method *   method2 = NULL;
	Method *   method3 = NULL;
	Method *   ret;

	TEST_FUNCTION ("interface_lookup_method");


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

		ret = interface_lookup_method (interface, "bar");

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

		ret = interface_lookup_method (interface, "baz");

		TEST_EQ_P (ret, NULL);

		nih_free (interface);
	}
}

void
test_lookup_signal (void)
{
	Interface *interface = NULL;
	Signal *   signal1 = NULL;
	Signal *   signal2 = NULL;
	Signal *   signal3 = NULL;
	Signal *   ret;

	TEST_FUNCTION ("interface_lookup_signal");


	/* Check that the function returns the signal if there is one
	 * with the given symbol.
	 */
	TEST_FEATURE ("with matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");

			signal1 = signal_new (interface, "Test");
			signal1->symbol = nih_strdup (signal1, "test");
			nih_list_add (&interface->signals, &signal1->entry);

			signal2 = signal_new (interface, "Foo");
			nih_list_add (&interface->signals, &signal2->entry);

			signal3 = signal_new (interface, "Bar");
			signal3->symbol = nih_strdup (signal3, "bar");
			nih_list_add (&interface->signals, &signal3->entry);
		}

		ret = interface_lookup_signal (interface, "bar");

		TEST_EQ_P (ret, signal3);

		nih_free (interface);
	}


	/* Check that the function returns NULL if there is no signal
	 * with the given symbol.
	 */
	TEST_FEATURE ("with non-matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");

			signal1 = signal_new (interface, "Test");
			signal1->symbol = nih_strdup (signal1, "test");
			nih_list_add (&interface->signals, &signal1->entry);

			signal2 = signal_new (interface, "Foo");
			nih_list_add (&interface->signals, &signal2->entry);

			signal3 = signal_new (interface, "Bar");
			signal3->symbol = nih_strdup (signal3, "bar");
			nih_list_add (&interface->signals, &signal3->entry);
		}

		ret = interface_lookup_signal (interface, "baz");

		TEST_EQ_P (ret, NULL);

		nih_free (interface);
	}
}

void
test_lookup_property (void)
{
	Interface *interface = NULL;
	Property * property1 = NULL;
	Property * property2 = NULL;
	Property * property3 = NULL;
	Property * ret;

	TEST_FUNCTION ("interface_lookup_property");


	/* Check that the function returns the property if there is one
	 * with the given symbol.
	 */
	TEST_FEATURE ("with matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");

			property1 = property_new (interface, "Test",
						  "s", NIH_DBUS_READ);
			property1->symbol = nih_strdup (property1, "test");
			nih_list_add (&interface->properties, &property1->entry);

			property2 = property_new (interface, "Foo",
						  "s", NIH_DBUS_READ);
			nih_list_add (&interface->properties, &property2->entry);

			property3 = property_new (interface, "Bar",
						  "s", NIH_DBUS_READ);
			property3->symbol = nih_strdup (property3, "bar");
			nih_list_add (&interface->properties, &property3->entry);
		}

		ret = interface_lookup_property (interface, "bar");

		TEST_EQ_P (ret, property3);

		nih_free (interface);
	}


	/* Check that the function returns NULL if there is no property
	 * with the given symbol.
	 */
	TEST_FEATURE ("with non-matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");

			property1 = property_new (interface, "Test",
						  "s", NIH_DBUS_READ);
			property1->symbol = nih_strdup (property1, "test");
			nih_list_add (&interface->properties, &property1->entry);

			property2 = property_new (interface, "Foo",
						  "s", NIH_DBUS_READ);
			nih_list_add (&interface->properties, &property2->entry);

			property3 = property_new (interface, "Bar",
						  "s", NIH_DBUS_READ);
			property3->symbol = nih_strdup (property3, "bar");
			nih_list_add (&interface->properties, &property3->entry);
		}

		ret = interface_lookup_property (interface, "baz");

		TEST_EQ_P (ret, NULL);

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
	test_lookup_method ();
	test_lookup_signal ();
	test_lookup_property ();

	return 0;
}
