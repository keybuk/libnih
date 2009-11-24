/* nih-dbus-tool
 *
 * test_interface.c - test suite for nih-dbus-tool/interface.c
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

#include <expat.h>

#include <errno.h>
#include <assert.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/main.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>
#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_pending_data.h>
#include <nih-dbus/errors.h>

#include "type.h"
#include "node.h"
#include "interface.h"
#include "method.h"
#include "signal.h"
#include "property.h"
#include "parse.h"
#include "errors.h"

#include "tests/interface_code.h"


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
	Method *     method = NULL;
	char *       attr[5];
	int          ret = 0;
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
			nih_discard (node);
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
			nih_discard (node);

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
			nih_discard (node);

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
			nih_discard (node);

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
			method = method_new (NULL, "Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

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
			nih_discard (node);

			interface = interface_new (NULL, "com.netsplit.Nih.TestInterface");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_INTERFACE, interface);
			nih_discard (interface);
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
			nih_discard (node);

			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = nih_strdup (interface, "foo");

			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_INTERFACE, interface);
			nih_discard (interface);
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
			nih_discard (node);

			other = interface_new (node, "com.netsplit.Foo.Test");
			other->symbol = nih_strdup (other, "test");
			nih_list_add (&node->interfaces, &other->entry);

			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_INTERFACE, interface);
			nih_discard (interface);
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
test_methods_array (void)
{
	NihList    prototypes;
	Interface *interface = NULL;
	Method *   method = NULL;
	Argument * arg = NULL;
	char *     str;
	TypeVar *  var;

	TEST_FUNCTION ("interface_methods_array");


	/* Check that we can generate an array of interface methods with
	 * their handler functions.  The C code returned should be lined up
	 * nicely and include the argument variable definitions as well.
	 */
	TEST_FEATURE ("with handlers");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			method = method_new (interface, "Poke");
			method->symbol = "poke";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_IN);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "Peek");
			method->symbol = "peek";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_OUT);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "IsValidAddress");
			method->symbol = "is_valid_address";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);
		}

		str = interface_methods_array (NULL, "my", interface, TRUE,
					       &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Poke_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Peek_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_IsValidAddress_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {\n"
			     "\t{ \"Poke\",           my_com_netsplit_Nih_Test_Poke_method_args,           my_com_netsplit_Nih_Test_Poke_method           },\n"
			     "\t{ \"Peek\",           my_com_netsplit_Nih_Test_Peek_method_args,           my_com_netsplit_Nih_Test_Peek_method           },\n"
			     "\t{ \"IsValidAddress\", my_com_netsplit_Nih_Test_IsValidAddress_method_args, my_com_netsplit_Nih_Test_IsValidAddress_method },\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusMethod");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_methods");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that we can generate an array of interface methods without
	 * their handler functions.  The C code returned should be lined up
	 * nicely, but should include NULL in the place of the handler
	 * function.
	 */
	TEST_FEATURE ("without handlers");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			method = method_new (interface, "Poke");
			method->symbol = "poke";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_IN);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "Peek");
			method->symbol = "peek";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_OUT);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "IsValidAddress");
			method->symbol = "is_valid_address";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);
		}

		str = interface_methods_array (NULL, "my", interface, FALSE,
					       &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Poke_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Peek_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_IsValidAddress_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {\n"
			     "\t{ \"Poke\",           my_com_netsplit_Nih_Test_Poke_method_args,           NULL },\n"
			     "\t{ \"Peek\",           my_com_netsplit_Nih_Test_Peek_method_args,           NULL },\n"
			     "\t{ \"IsValidAddress\", my_com_netsplit_Nih_Test_IsValidAddress_method_args, NULL },\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusMethod");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_methods");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that the array is returned empty if the interface has
	 * no methods.
	 */
	TEST_FEATURE ("with no methods");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";
		}

		str = interface_methods_array (NULL, "my", interface, FALSE,
					       &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusMethod");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_methods");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}
}

void
test_signals_array (void)
{
	NihList    prototypes;
	Interface *interface = NULL;
	Signal *   signal = NULL;
	Argument * arg = NULL;
	char *     str;
	TypeVar *  var;

	TEST_FUNCTION ("interface_signals_array");


	/* Check that we can generate an array of interface signals with
	 * their filter functions.  The C code returned should be lined up
	 * nicely and include the argument variable definitions as well.
	 */
	TEST_FEATURE ("with filters");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			signal = signal_new (interface, "Bounce");
			signal->symbol = "bounce";
			nih_list_add (&interface->signals, &signal->entry);

			arg = argument_new (signal, "height",
					    "u", NIH_DBUS_ARG_OUT);
			arg->symbol = "height";
			nih_list_add (&signal->arguments, &arg->entry);

			arg = argument_new (signal, "velocity",
					    "i", NIH_DBUS_ARG_OUT);
			arg->symbol = "velocity";
			nih_list_add (&signal->arguments, &arg->entry);

			signal = signal_new (interface, "Exploded");
			signal->symbol = "exploded";
			nih_list_add (&interface->signals, &signal->entry);
		}

		str = interface_signals_array (NULL, "my", interface, TRUE,
					       &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Bounce_signal_args[] = {\n"
			     "\t{ \"height\",   \"u\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ \"velocity\", \"i\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Exploded_signal_args[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {\n"
			     "\t{ \"Bounce\",   my_com_netsplit_Nih_Test_Bounce_signal_args,   my_com_netsplit_Nih_Test_Bounce_signal   },\n"
			     "\t{ \"Exploded\", my_com_netsplit_Nih_Test_Exploded_signal_args, my_com_netsplit_Nih_Test_Exploded_signal },\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusSignal");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_signals");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that we can generate an array of interface signals without
	 * their filter functions.  The C code returned should be lined up
	 * nicely, but should include NULL in the place of the filter
	 * function.
	 */
	TEST_FEATURE ("without filters");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			signal = signal_new (interface, "Bounce");
			signal->symbol = "bounce";
			nih_list_add (&interface->signals, &signal->entry);

			arg = argument_new (signal, "height",
					    "u", NIH_DBUS_ARG_OUT);
			arg->symbol = "height";
			nih_list_add (&signal->arguments, &arg->entry);

			arg = argument_new (signal, "velocity",
					    "i", NIH_DBUS_ARG_OUT);
			arg->symbol = "velocity";
			nih_list_add (&signal->arguments, &arg->entry);

			signal = signal_new (interface, "Exploded");
			signal->symbol = "exploded";
			nih_list_add (&interface->signals, &signal->entry);
		}

		str = interface_signals_array (NULL, "my", interface, FALSE,
					       &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Bounce_signal_args[] = {\n"
			     "\t{ \"height\",   \"u\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ \"velocity\", \"i\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Exploded_signal_args[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {\n"
			     "\t{ \"Bounce\",   my_com_netsplit_Nih_Test_Bounce_signal_args,   NULL },\n"
			     "\t{ \"Exploded\", my_com_netsplit_Nih_Test_Exploded_signal_args, NULL },\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusSignal");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_signals");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that the array is returned empty if the interface has
	 * no signals.
	 */
	TEST_FEATURE ("with no signals");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";
		}

		str = interface_signals_array (NULL, "my", interface, FALSE,
					       &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusSignal");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_signals");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}
}


void
test_properties_array (void)
{
	NihList    prototypes;
	Interface *interface = NULL;
	Property * property = NULL;
	char *     str;
	TypeVar *  var;

	TEST_FUNCTION ("interface_properties_array");


	/* Check that we can create an array of an interface's properties,
	 * with getter and setter functions filled in where appropriate.
	 * Each of the columns should be lined up nicely.
	 */
	TEST_FEATURE ("with handler functions");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

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
		}

		str = interface_properties_array (NULL, "my", interface, TRUE,
						  &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {\n"
			     "\t{ \"colour\", \"s\", NIH_DBUS_READWRITE, my_com_netsplit_Nih_Test_colour_get, my_com_netsplit_Nih_Test_colour_set },\n"
			     "\t{ \"size\",   \"u\", NIH_DBUS_READ,      my_com_netsplit_Nih_Test_size_get,   NULL                                },\n"
			     "\t{ \"touch\",  \"b\", NIH_DBUS_WRITE,     NULL,                                my_com_netsplit_Nih_Test_touch_set  },\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusProperty");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_properties");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that we can create an array of an interface's properties
	 * without getter and setter functions filled in.  Each of the
	 * columns should still be lined up.
	 */
	TEST_FEATURE ("without handler functions");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

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
		}

		str = interface_properties_array (NULL, "my", interface, FALSE,
						  &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {\n"
			     "\t{ \"colour\", \"s\", NIH_DBUS_READWRITE, NULL, NULL },\n"
			     "\t{ \"size\",   \"u\", NIH_DBUS_READ,      NULL, NULL },\n"
			     "\t{ \"touch\",  \"b\", NIH_DBUS_WRITE,     NULL, NULL },\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusProperty");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_properties");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that we return an empty array when the interface has no
	 * properties.
	 */
	TEST_FEATURE ("with no properties");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";
		}

		str = interface_properties_array (NULL, "my", interface, FALSE,
						  &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusProperty");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_properties");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}
}


void
test_struct (void)
{
	NihList    prototypes;
	Interface *interface = NULL;
	Method *   method = NULL;
	Signal *   signal = NULL;
	Argument * arg = NULL;
	Property * property = NULL;
	char *     str;
	TypeVar *  var;

	TEST_FUNCTION ("interface_struct");

	/* Check that we can generate the structure variable code for an
	 * interface with many methods, signals and properties.  We want
	 * the members set up for an object implementation, so the method
	 * and property function pointers should be set and not the signal
	 * filter pointer.
	 */
	TEST_FEATURE ("with object");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			method = method_new (interface, "Poke");
			method->symbol = "poke";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_IN);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "Peek");
			method->symbol = "peek";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_OUT);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "IsValidAddress");
			method->symbol = "is_valid_address";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);


			signal = signal_new (interface, "Bounce");
			signal->symbol = "bounce";
			nih_list_add (&interface->signals, &signal->entry);

			arg = argument_new (signal, "height",
					    "u", NIH_DBUS_ARG_OUT);
			arg->symbol = "height";
			nih_list_add (&signal->arguments, &arg->entry);

			arg = argument_new (signal, "velocity",
					    "i", NIH_DBUS_ARG_OUT);
			arg->symbol = "velocity";
			nih_list_add (&signal->arguments, &arg->entry);

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
		}

		str = interface_struct (NULL, "my", interface, TRUE,
					&prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Poke_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Peek_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_IsValidAddress_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {\n"
			     "\t{ \"Poke\",           my_com_netsplit_Nih_Test_Poke_method_args,           my_com_netsplit_Nih_Test_Poke_method           },\n"
			     "\t{ \"Peek\",           my_com_netsplit_Nih_Test_Peek_method_args,           my_com_netsplit_Nih_Test_Peek_method           },\n"
			     "\t{ \"IsValidAddress\", my_com_netsplit_Nih_Test_IsValidAddress_method_args, my_com_netsplit_Nih_Test_IsValidAddress_method },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Bounce_signal_args[] = {\n"
			     "\t{ \"height\",   \"u\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ \"velocity\", \"i\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Exploded_signal_args[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {\n"
			     "\t{ \"Bounce\",   my_com_netsplit_Nih_Test_Bounce_signal_args,   NULL },\n"
			     "\t{ \"Exploded\", my_com_netsplit_Nih_Test_Exploded_signal_args, NULL },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {\n"
			     "\t{ \"colour\", \"s\", NIH_DBUS_READWRITE, my_com_netsplit_Nih_Test_colour_get, my_com_netsplit_Nih_Test_colour_set },\n"
			     "\t{ \"size\",   \"u\", NIH_DBUS_READ,      my_com_netsplit_Nih_Test_size_get,   NULL                                },\n"
			     "\t{ \"touch\",  \"b\", NIH_DBUS_WRITE,     NULL,                                my_com_netsplit_Nih_Test_touch_set  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusInterface my_com_netsplit_Nih_Test = {\n"
			     "\t\"com.netsplit.Nih.Test\",\n"
			     "\tmy_com_netsplit_Nih_Test_methods,\n"
			     "\tmy_com_netsplit_Nih_Test_signals,\n"
			     "\tmy_com_netsplit_Nih_Test_properties\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that we can generate the structure variable code for an
	 * interface with many methods, signals and properties.  We want
	 * the members set up for a proxy implementation, so the signal
	 * filter pointer should be set but not the method or property
	 * function pointers.
	 */
	TEST_FEATURE ("with proxy");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			method = method_new (interface, "Poke");
			method->symbol = "poke";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_IN);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "Peek");
			method->symbol = "peek";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_OUT);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "IsValidAddress");
			method->symbol = "is_valid_address";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);


			signal = signal_new (interface, "Bounce");
			signal->symbol = "bounce";
			nih_list_add (&interface->signals, &signal->entry);

			arg = argument_new (signal, "height",
					    "u", NIH_DBUS_ARG_OUT);
			arg->symbol = "height";
			nih_list_add (&signal->arguments, &arg->entry);

			arg = argument_new (signal, "velocity",
					    "i", NIH_DBUS_ARG_OUT);
			arg->symbol = "velocity";
			nih_list_add (&signal->arguments, &arg->entry);

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
		}

		str = interface_struct (NULL, "my", interface, FALSE,
					&prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Poke_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Peek_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_IsValidAddress_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {\n"
			     "\t{ \"Poke\",           my_com_netsplit_Nih_Test_Poke_method_args,           NULL },\n"
			     "\t{ \"Peek\",           my_com_netsplit_Nih_Test_Peek_method_args,           NULL },\n"
			     "\t{ \"IsValidAddress\", my_com_netsplit_Nih_Test_IsValidAddress_method_args, NULL },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Bounce_signal_args[] = {\n"
			     "\t{ \"height\",   \"u\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ \"velocity\", \"i\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Exploded_signal_args[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {\n"
			     "\t{ \"Bounce\",   my_com_netsplit_Nih_Test_Bounce_signal_args,   my_com_netsplit_Nih_Test_Bounce_signal   },\n"
			     "\t{ \"Exploded\", my_com_netsplit_Nih_Test_Exploded_signal_args, my_com_netsplit_Nih_Test_Exploded_signal },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {\n"
			     "\t{ \"colour\", \"s\", NIH_DBUS_READWRITE, NULL, NULL },\n"
			     "\t{ \"size\",   \"u\", NIH_DBUS_READ,      NULL, NULL },\n"
			     "\t{ \"touch\",  \"b\", NIH_DBUS_WRITE,     NULL, NULL },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusInterface my_com_netsplit_Nih_Test = {\n"
			     "\t\"com.netsplit.Nih.Test\",\n"
			     "\tmy_com_netsplit_Nih_Test_methods,\n"
			     "\tmy_com_netsplit_Nih_Test_signals,\n"
			     "\tmy_com_netsplit_Nih_Test_properties\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that valid code is still returned when there are no
	 * methods.
	 */
	TEST_FEATURE ("with no methods");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			signal = signal_new (interface, "Bounce");
			signal->symbol = "bounce";
			nih_list_add (&interface->signals, &signal->entry);

			arg = argument_new (signal, "height",
					    "u", NIH_DBUS_ARG_OUT);
			arg->symbol = "height";
			nih_list_add (&signal->arguments, &arg->entry);

			arg = argument_new (signal, "velocity",
					    "i", NIH_DBUS_ARG_OUT);
			arg->symbol = "velocity";
			nih_list_add (&signal->arguments, &arg->entry);

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
		}

		str = interface_struct (NULL, "my", interface, TRUE,
					&prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Bounce_signal_args[] = {\n"
			     "\t{ \"height\",   \"u\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ \"velocity\", \"i\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Exploded_signal_args[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {\n"
			     "\t{ \"Bounce\",   my_com_netsplit_Nih_Test_Bounce_signal_args,   NULL },\n"
			     "\t{ \"Exploded\", my_com_netsplit_Nih_Test_Exploded_signal_args, NULL },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {\n"
			     "\t{ \"colour\", \"s\", NIH_DBUS_READWRITE, my_com_netsplit_Nih_Test_colour_get, my_com_netsplit_Nih_Test_colour_set },\n"
			     "\t{ \"size\",   \"u\", NIH_DBUS_READ,      my_com_netsplit_Nih_Test_size_get,   NULL                                },\n"
			     "\t{ \"touch\",  \"b\", NIH_DBUS_WRITE,     NULL,                                my_com_netsplit_Nih_Test_touch_set  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusInterface my_com_netsplit_Nih_Test = {\n"
			     "\t\"com.netsplit.Nih.Test\",\n"
			     "\tmy_com_netsplit_Nih_Test_methods,\n"
			     "\tmy_com_netsplit_Nih_Test_signals,\n"
			     "\tmy_com_netsplit_Nih_Test_properties\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that valid code is still returned when there are no
	 * signals.
	 */
	TEST_FEATURE ("with no signals");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			method = method_new (interface, "Poke");
			method->symbol = "poke";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_IN);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "Peek");
			method->symbol = "peek";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_OUT);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "IsValidAddress");
			method->symbol = "is_valid_address";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);


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
		}

		str = interface_struct (NULL, "my", interface, TRUE,
					&prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Poke_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Peek_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_IsValidAddress_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {\n"
			     "\t{ \"Poke\",           my_com_netsplit_Nih_Test_Poke_method_args,           my_com_netsplit_Nih_Test_Poke_method           },\n"
			     "\t{ \"Peek\",           my_com_netsplit_Nih_Test_Peek_method_args,           my_com_netsplit_Nih_Test_Peek_method           },\n"
			     "\t{ \"IsValidAddress\", my_com_netsplit_Nih_Test_IsValidAddress_method_args, my_com_netsplit_Nih_Test_IsValidAddress_method },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {\n"
			     "\t{ \"colour\", \"s\", NIH_DBUS_READWRITE, my_com_netsplit_Nih_Test_colour_get, my_com_netsplit_Nih_Test_colour_set },\n"
			     "\t{ \"size\",   \"u\", NIH_DBUS_READ,      my_com_netsplit_Nih_Test_size_get,   NULL                                },\n"
			     "\t{ \"touch\",  \"b\", NIH_DBUS_WRITE,     NULL,                                my_com_netsplit_Nih_Test_touch_set  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusInterface my_com_netsplit_Nih_Test = {\n"
			     "\t\"com.netsplit.Nih.Test\",\n"
			     "\tmy_com_netsplit_Nih_Test_methods,\n"
			     "\tmy_com_netsplit_Nih_Test_signals,\n"
			     "\tmy_com_netsplit_Nih_Test_properties\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that valid code is still returned when there are no
	 * properties.
	 */
	TEST_FEATURE ("with no properties");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			method = method_new (interface, "Poke");
			method->symbol = "poke";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_IN);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "Peek");
			method->symbol = "peek";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);

			arg = argument_new (method, "value",
					    "s", NIH_DBUS_ARG_OUT);
			arg->symbol = "value";
			nih_list_add (&method->arguments, &arg->entry);

			method = method_new (interface, "IsValidAddress");
			method->symbol = "is_valid_address";
			nih_list_add (&interface->methods, &method->entry);

			arg = argument_new (method, "address",
					    "u", NIH_DBUS_ARG_IN);
			arg->symbol = "address";
			nih_list_add (&method->arguments, &arg->entry);


			signal = signal_new (interface, "Bounce");
			signal->symbol = "bounce";
			nih_list_add (&interface->signals, &signal->entry);

			arg = argument_new (signal, "height",
					    "u", NIH_DBUS_ARG_OUT);
			arg->symbol = "height";
			nih_list_add (&signal->arguments, &arg->entry);

			arg = argument_new (signal, "velocity",
					    "i", NIH_DBUS_ARG_OUT);
			arg->symbol = "velocity";
			nih_list_add (&signal->arguments, &arg->entry);

			signal = signal_new (interface, "Exploded");
			signal->symbol = "exploded";
			nih_list_add (&interface->signals, &signal->entry);
		}

		str = interface_struct (NULL, "my", interface, TRUE,
					&prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Poke_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Peek_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ \"value\",   \"s\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_IsValidAddress_method_args[] = {\n"
			     "\t{ \"address\", \"u\", NIH_DBUS_ARG_IN  },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {\n"
			     "\t{ \"Poke\",           my_com_netsplit_Nih_Test_Poke_method_args,           my_com_netsplit_Nih_Test_Poke_method           },\n"
			     "\t{ \"Peek\",           my_com_netsplit_Nih_Test_Peek_method_args,           my_com_netsplit_Nih_Test_Peek_method           },\n"
			     "\t{ \"IsValidAddress\", my_com_netsplit_Nih_Test_IsValidAddress_method_args, my_com_netsplit_Nih_Test_IsValidAddress_method },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Bounce_signal_args[] = {\n"
			     "\t{ \"height\",   \"u\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ \"velocity\", \"i\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusArg my_com_netsplit_Nih_Test_Exploded_signal_args[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {\n"
			     "\t{ \"Bounce\",   my_com_netsplit_Nih_Test_Bounce_signal_args,   NULL },\n"
			     "\t{ \"Exploded\", my_com_netsplit_Nih_Test_Exploded_signal_args, NULL },\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusInterface my_com_netsplit_Nih_Test = {\n"
			     "\t\"com.netsplit.Nih.Test\",\n"
			     "\tmy_com_netsplit_Nih_Test_methods,\n"
			     "\tmy_com_netsplit_Nih_Test_signals,\n"
			     "\tmy_com_netsplit_Nih_Test_properties\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that valid code is still returned when there are no
	 * methods, signals or properties.
	 */
	TEST_FEATURE ("with no members");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";
		}

		str = interface_struct (NULL, "my", interface, TRUE,
					&prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "static const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusInterface my_com_netsplit_Nih_Test = {\n"
			     "\t\"com.netsplit.Nih.Test\",\n"
			     "\tmy_com_netsplit_Nih_Test_methods,\n"
			     "\tmy_com_netsplit_Nih_Test_signals,\n"
			     "\tmy_com_netsplit_Nih_Test_properties\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}
}


int my_com_netsplit_Nih_TestA_get_all_notify_called = FALSE;
static DBusPendingCall *   last_pending_call = NULL;
static NihDBusPendingData *last_pending_data = NULL;

void
my_com_netsplit_Nih_TestA_get_all_notify (DBusPendingCall *   pending_call,
					  NihDBusPendingData *pending_data)
{
	my_com_netsplit_Nih_TestA_get_all_notify_called = TRUE;
	last_pending_call = pending_call;
	last_pending_data = pending_data;
}

static void
my_blank_get_handler (void *              data,
		      NihDBusMessage *    message,
		      const MyProperties *value)
{
}

static void
my_blank_error_handler (void *          data,
			NihDBusMessage *message)
{
}

void
test_proxy_get_all_function (void)
{
	pid_t             dbus_pid;
	DBusConnection *  server_conn;
	DBusConnection *  client_conn;
	NihList           prototypes;
	NihList           structs;
	Interface *       interface = NULL;
	Property *        property = NULL;
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
	DBusMessageIter   arrayiter;
	DBusMessageIter   dictiter;
	DBusMessageIter   variter;
	char *            str_value;
	uint32_t          uint32_value;
	NihError *        err;
	NihDBusError *    dbus_err;

	TEST_FUNCTION ("interface_proxy_get_all_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that makes a method call
	 * to obtain the value of all readable D-Bus properties for the given
	 * interface, returning the pending call structure.
	 */
	TEST_FEATURE ("with interface");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (interface, "name",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = "name";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "size",
						 "u", NIH_DBUS_READ);
			property->symbol = "size";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "touch",
						 "b", NIH_DBUS_WRITE);
			property->symbol = "touch";
			nih_list_add (&interface->properties, &property->entry);
		}

		str = interface_proxy_get_all_function (
			NULL, "my", interface,
			&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_interface_proxy_get_all_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_all");
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
		TEST_EQ_STR (arg->type, "MyGetAllReply");
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
		nih_free (interface);
	}


	/* Check that we can use the generated code to make a method call
	 * to obtain the values of the properties.  The function should
	 * return a DBusPendingCall object and we should receive the method
	 * call on the other side.  Returning the reply and blocking the call
	 * should result in our notify function being called with the
	 * pending call that was returned and the pending data with the
	 * expected information.
	 */
	TEST_FEATURE ("with interface (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_com_netsplit_Nih_TestA_get_all_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_all (proxy,
					   my_blank_get_handler,
					   my_blank_error_handler,
					   &proxy, -1);

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
							DBUS_INTERFACE_PROPERTIES,
							"GetAll"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.TestA");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_VARIANT_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "name";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &variter);

		str_value = "Joe Bloggs";
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "size";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &variter);

		uint32_value = 34;
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_close_container (&iter, &arrayiter);

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
		TEST_TRUE (my_com_netsplit_Nih_TestA_get_all_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_get_handler);
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

		my_com_netsplit_Nih_TestA_get_all_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_all (proxy,
					   my_blank_get_handler,
					   my_blank_error_handler,
					   &proxy, -1);

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
							DBUS_INTERFACE_PROPERTIES,
							"GetAll"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.TestA");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_error (method_call,
						"com.netsplit.Nih.Fail",
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
						  "com.netsplit.Nih.Fail"));
		dbus_message_unref (reply);

		/* Check the notify function was called with all the right
		 * things.
		 */
		TEST_TRUE (my_com_netsplit_Nih_TestA_get_all_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_get_handler);
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
	 * with the notify function being called for the timeout error.
	 */
	TEST_FEATURE ("with timeout (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_com_netsplit_Nih_TestA_get_all_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_all (proxy,
					   my_blank_get_handler,
					   my_blank_error_handler,
					   &proxy, 50);

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
							DBUS_INTERFACE_PROPERTIES,
							"GetAll"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.TestA");

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
		TEST_TRUE (my_com_netsplit_Nih_TestA_get_all_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_get_handler);
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
	 * disconnects.  The notify function will be called with the no
	 * reply error.
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

		my_com_netsplit_Nih_TestA_get_all_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_all (proxy,
					   my_blank_get_handler,
					   my_blank_error_handler,
					   &proxy, -1);

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
							DBUS_INTERFACE_PROPERTIES,
							"GetAll"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.TestA");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (method_call);


		/* Close the server connection */
		TEST_DBUS_CLOSE (flakey_conn);


		/* Block the pending call until timeout */
		dbus_pending_call_block (pending_call);
		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		reply = dbus_pending_call_steal_reply (pending_call);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_NO_REPLY));
		dbus_message_unref (reply);

		/* Check the notify function was called with all the right
		 * things.
		 */
		TEST_TRUE (my_com_netsplit_Nih_TestA_get_all_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_get_handler);
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


	/* Check that the pending call can be cancelled by the user.
	 * The notify function should not be called, but the data it
	 * contains should be freed (check valgrind).
	 */
	TEST_FEATURE ("with cancelled call (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_com_netsplit_Nih_TestA_get_all_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_all (proxy,
					   my_blank_get_handler,
					   my_blank_error_handler,
					   &proxy, -1);

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
							DBUS_INTERFACE_PROPERTIES,
							"GetAll"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.TestA");

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
		TEST_FALSE (my_com_netsplit_Nih_TestA_get_all_notify_called);

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

		my_com_netsplit_Nih_TestA_get_all_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_all (proxy,
					   my_blank_get_handler,
					   my_blank_error_handler,
					   &proxy, -1);

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

		TEST_FALSE (my_com_netsplit_Nih_TestA_get_all_notify_called);

		nih_free (proxy);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

static void my_error_handler (void *data, NihDBusMessage *message);

static int my_get_all_handler_called = FALSE;
static int my_error_handler_called = FALSE;
static NihDBusMessage *last_message = NULL;
static DBusConnection *last_conn = NULL;
static DBusMessage *last_msg = NULL;
static NihError *last_error = NULL;

static void
__attribute__ ((unused))
my_get_all_handler (void *              data,
		    NihDBusMessage *    message,
		    const MyProperties *properties)
{
	my_get_all_handler_called++;

	TEST_EQ_P (data, (void *)my_error_handler);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	last_message = message;
	TEST_FREE_TAG (last_message);

	last_conn = message->connection;
	dbus_connection_ref (last_conn);

	last_msg = message->message;
	dbus_message_ref (last_msg);

	TEST_NE_P (properties, NULL);
	TEST_ALLOC_SIZE (properties, sizeof (MyProperties));
	TEST_ALLOC_PARENT (properties, message);

	TEST_EQ_STR (properties->name, "Joe Bloggs");
	TEST_ALLOC_PARENT (properties->name, properties);

	TEST_EQ (properties->size, 34);
}

static void
my_error_handler (void *          data,
		  NihDBusMessage *message)
{
	my_error_handler_called++;

	TEST_EQ_P (data, (void *)my_error_handler);

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
test_proxy_get_all_notify_function (void)
{
	pid_t               dbus_pid;
	NihList             prototypes;
	NihList             typedefs;
	NihList             structs;
	Interface *         interface = NULL;
	Property *          property = NULL;
	char *              str;
	TypeFunc *          func;
	TypeVar *           arg;
	TypeStruct *        structure;
	TypeVar *           var;
	DBusConnection *    server_conn;
	DBusConnection *    client_conn;
	DBusConnection *    flakey_conn;
	dbus_uint32_t       serial;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data = NULL;
	DBusMessage *       method_call;
	DBusMessage *       reply;
	DBusMessageIter     iter;
	DBusMessageIter     arrayiter;
	DBusMessageIter     dictiter;
	DBusMessageIter     variter;
	char *              str_value;
	uint32_t            uint32_value;
	double              double_value;
	NihDBusError *      dbus_err;

	TEST_FUNCTION ("interface_proxy_get_all_notify_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that takes a pending call
	 * and pending data structure, stealing the D-Bus message and
	 * demarshalling the property values from the array of name and
	 * variants argument before making a call to either the handler
	 * for a valid reply or error handler for an invalid reply.
	 * The typedef for the handler function is returned in addition
	 * to the prototype.
	 */
	TEST_FEATURE ("with interface");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (interface, "name",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = "name";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "size",
						 "u", NIH_DBUS_READ);
			property->symbol = "size";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "touch",
						 "b", NIH_DBUS_WRITE);
			property->symbol = "touch";
			nih_list_add (&interface->properties, &property->entry);
		}

		str = interface_proxy_get_all_notify_function (
			NULL, "my", interface,
			&prototypes, &typedefs,
			&structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_interface_proxy_get_all_notify_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_get_all_notify");
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
		TEST_EQ_STR (func->name, "(*MyGetAllReply)");
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
		TEST_EQ_STR (arg->type, "const MyProperties *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "properties");
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
		TEST_EQ_STR (structure->name, "MyProperties");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "name");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "size");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can generate a notify function for an interface
	 * that has a structure property, with the structure type passed
	 * back in the structs array before the struct definition for
	 * the return value.
	 */
	TEST_FEATURE ("with structure property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (interface, "name",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = "name";
			nih_list_add (&interface->properties, &property->entry);


			property = property_new (interface, "size",
						 "u", NIH_DBUS_READ);
			property->symbol = "size";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "touch",
						 "b", NIH_DBUS_WRITE);
			property->symbol = "touch";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (NULL, "birthday",
						 "(iii)", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "birthday");
			nih_list_add (&interface->properties, &property->entry);
		}

		str = interface_proxy_get_all_notify_function (
			NULL, "my", interface,
			&prototypes, &typedefs,
			&structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_interface_proxy_get_all_notify_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_get_all_notify");
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
		TEST_EQ_STR (func->name, "(*MyGetAllReply)");
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
		TEST_EQ_STR (arg->type, "const MyProperties *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "properties");
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
		TEST_EQ_STR (structure->name, "MyBirthday");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "int32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "int32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "int32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item2");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyProperties");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "name");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "size");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "MyBirthday *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "birthday");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to handle a completed
	 * pending call, demarshalling the property values from the array
	 * of name and variants in the reply and passing them in a single
	 * structure to our handler.
	 */
	TEST_FEATURE ("with reply (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

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
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_VARIANT_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "name";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &variter);

		str_value = "Joe Bloggs";
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "size";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &variter);

		uint32_value = 34;
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_close_container (&iter, &arrayiter);

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_TRUE (my_get_all_handler_called);
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


	/* Check that we can use the generated code to handle an error
	 * reply, passing it to the error handler as a raised error
	 * instead of calling the usual handler.
	 */
	TEST_FEATURE ("with error reply (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

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
						"com.netsplit.Nih.Fail",
						"Things didn't work out");
		dbus_message_unref (method_call);

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Fail");
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
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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


	/* Check that the generated code doesn't mind if there's an
	 * extra property in the reply, it simply ignores it; this is to
	 * allow for future extension without breaking the API.
	 */
	TEST_FEATURE ("with extra property (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

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
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_VARIANT_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "name";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &variter);

		str_value = "Joe Bloggs";
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "size";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &variter);

		uint32_value = 34;
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "body_mass";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_DOUBLE_AS_STRING,
						  &variter);

		double_value = 25.5;
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_close_container (&iter, &arrayiter);

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_TRUE (my_get_all_handler_called);
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


	/* Check that the generated code catches a missing property in
	 * the reply and calls the error handler with the invalid arguments
	 * error raised.
	 */
	TEST_FEATURE ("with missing property (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
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
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_VARIANT_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);

		str_value = "size";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &variter);

		uint32_value = 34;
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_close_container (&iter, &arrayiter);

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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
	 * the variant and calls the error handler with the invalid arguments
	 * error raised.
	 */
	TEST_FEATURE ("with incorrect variant member type (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
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
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_VARIANT_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "name";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &variter);

		str_value = "Joe Bloggs";
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "size";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_DOUBLE_AS_STRING,
						  &variter);

		double_value = 3.14;
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_close_container (&iter, &arrayiter);

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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


	/* Check that the generated code catches an invalid key type in
	 * the dictionary and calls the error handler with the invalid
	 * arguments error raised.
	 */
	TEST_FEATURE ("with incorrect dict key type (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
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
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_DOUBLE_AS_STRING
						   DBUS_TYPE_VARIANT_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		double_value = 3.14;
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &variter);

		str_value = "Joe Bloggs";
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		double_value = 4.86;
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &variter);

		uint32_value = 34;
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_close_container (&iter, &arrayiter);

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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
	 * the dictionary and calls the error handler with the invalid
	 * arguments error raised.
	 */
	TEST_FEATURE ("with incorrect dict member type (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
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
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "name";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "Joe Bloggs";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "size";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "34";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_close_container (&iter, &arrayiter);

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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
	 * the array and calls the error handler with the invalid
	 * arguments error raised.
	 */
	TEST_FEATURE ("with incorrect array member type (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
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
						  &arrayiter);

		str_value = "name";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "size";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &arrayiter);

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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
	 * the reply and calls the error handler with the invalid
	 * arguments error raised.
	 */
	TEST_FEATURE ("with incorrect type (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
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

		double_value = 3.14;
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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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


	/* Check that the generated code catches a missing argument in
	 * the reply and calls the error handler with the invalid arguments
	 * error raised.
	 */
	TEST_FEATURE ("with missing argument (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
		dbus_connection_flush (client_conn);

		serial = dbus_message_get_serial (method_call);
		dbus_message_unref (method_call);

		/* Catch it */
		TEST_DBUS_MESSAGE (server_conn, method_call);
		assert (dbus_message_get_serial (method_call) == serial);

		/* Reply to it */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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


	/* Check that the generated code catches an extra argument in
	 * the reply and calls the error handler with the invalid arguments
	 * error raised.
	 */
	TEST_FEATURE ("with extra argument (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");

		pending_call = NULL;
		dbus_connection_send_with_reply (client_conn, method_call,
						 &pending_call, 50);
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
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_VARIANT_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "name";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &variter);

		str_value = "Joe Bloggs";
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &dictiter);


		str_value = "size";
		dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &variter);

		uint32_value = 34;
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&dictiter, &variter);

		dbus_message_iter_close_container (&arrayiter, &dictiter);


		dbus_message_iter_close_container (&iter, &arrayiter);

		double_value = 3.14;
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
				(NihDBusReplyHandler)my_get_all_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_all_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_get_all_notify (pending_call, pending_data);

		TEST_FALSE (my_get_all_handler_called);
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


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_proxy_get_all_sync_function (void)
{
	pid_t           dbus_pid;
	DBusConnection *server_conn;
	DBusConnection *client_conn;
	DBusConnection *flakey_conn;
	NihList         prototypes;
	NihList         structs;
	Interface *     interface = NULL;
	Property *      property = NULL;
	char *          str;
	TypeFunc *      func;
	TypeVar *       arg;
	NihListEntry *  attrib;
	TypeStruct *    structure;
	TypeVar *       var;
	NihDBusProxy *  proxy = NULL;
	void *          parent = NULL;
	pid_t           pid;
	int             status;
	DBusMessage *   method_call;
	DBusMessage *   reply;
	DBusMessageIter iter;
	DBusMessageIter arrayiter;
	DBusMessageIter dictiter;
	DBusMessageIter variter;
	char *          str_value;
	uint32_t        uint32_value;
	double          double_value;
	MyProperties *  properties;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;

	TEST_FUNCTION ("interface_proxy_get_all_sync_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that will make a method
	 * call to obtain the value of all properties and return a struct
	 * containing them in the pointer argument supplied.  The function
	 * returns an integer to indicate success.
	 */
	TEST_FEATURE ("with interface");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (interface, "name",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = "name";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "size",
						 "u", NIH_DBUS_READ);
			property->symbol = "size";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "touch",
						 "b", NIH_DBUS_WRITE);
			property->symbol = "touch";
			nih_list_add (&interface->properties, &property->entry);
		}

		str = interface_proxy_get_all_sync_function (
			NULL, "my", interface,
			&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_interface_proxy_get_all_sync_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_all_sync");
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
		TEST_EQ_STR (arg->type, "MyProperties **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "properties");
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
		TEST_EQ_STR (structure->name, "MyProperties");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "name");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "size");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can generate a function for an interface that
	 * has a structure property, with the structure type passed
	 * back in the structs array before the struct definition for
	 * the return value.
	 */
	TEST_FEATURE ("with structure property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (interface, "name",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = "name";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "size",
						 "u", NIH_DBUS_READ);
			property->symbol = "size";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "touch",
						 "b", NIH_DBUS_WRITE);
			property->symbol = "touch";
			nih_list_add (&interface->properties, &property->entry);

			property = property_new (interface, "birthday",
						 "(iii)", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "birthday");
			nih_list_add (&interface->properties, &property->entry);
		}

		str = interface_proxy_get_all_sync_function (
			NULL, "my", interface,
			&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_interface_proxy_get_all_sync_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_all_sync");
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
		TEST_EQ_STR (arg->type, "MyProperties **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "properties");
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
		TEST_EQ_STR (structure->name, "MyBirthday");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "int32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "int32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "int32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item2");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyProperties");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "name");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "size");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "MyBirthday *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "birthday");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to make a method call
	 * and obtain the value of the properties.
	 */
	TEST_FEATURE ("with method call (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_VARIANT_AS_STRING
							   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
							  &arrayiter);

			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);


			str_value = "name";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &variter);

			str_value = "Joe Bloggs";
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);


			str_value = "size";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_UINT32_AS_STRING,
							  &variter);

			uint32_value = 34;
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_close_container (&iter, &arrayiter);

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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (ret, 0);

		TEST_NE_P (properties, NULL);
		TEST_ALLOC_SIZE (properties, sizeof (MyProperties));
		TEST_ALLOC_PARENT (properties, parent);

		TEST_EQ_STR (properties->name, "Joe Bloggs");
		TEST_ALLOC_PARENT (properties->name, properties);

		TEST_EQ (properties->size, 34);

		nih_free (proxy);
	}


	/* Check that the generated code handles an error returned from
	 * the property get function, returning a raised error.
	 */
	TEST_FEATURE ("with error returned (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_error (method_call,
							"com.netsplit.Nih.Failed",
							"Didn't work out");
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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.Failed");
		TEST_EQ_STR (err->message, "Didn't work out");
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
	}


	/* Check that the generated code returns a raised disconnected
	 * error when called on a disconnected connection.
	 */
	TEST_FEATURE ("with error returned (generated code)");
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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_DISCONNECTED);
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
	}


	/* Check that the generated code doesn't mind if there's an
	 * extra property in the reply, it simply ignores it; this is to
	 * allow for future extension without breaking the API.
	 */
	TEST_FEATURE ("with extra property (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_VARIANT_AS_STRING
							   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
							  &arrayiter);

			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);


			str_value = "name";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &variter);

			str_value = "Joe Bloggs";
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);


			str_value = "size";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_UINT32_AS_STRING,
							  &variter);

			uint32_value = 34;
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);


			str_value = "body_mass";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &variter);

			double_value = 25.5;
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_close_container (&iter, &arrayiter);

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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (ret, 0);

		TEST_NE_P (properties, NULL);
		TEST_ALLOC_SIZE (properties, sizeof (MyProperties));
		TEST_ALLOC_PARENT (properties, parent);

		TEST_EQ_STR (properties->name, "Joe Bloggs");
		TEST_ALLOC_PARENT (properties->name, properties);

		TEST_EQ (properties->size, 34);

		nih_free (proxy);
	}


	/* Check that the generated code catches a missing property in
	 * the reply and results in the function returning a raised error.
	 */
	TEST_FEATURE ("with missing property (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_VARIANT_AS_STRING
							   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
							  &arrayiter);

			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);

			str_value = "size";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_UINT32_AS_STRING,
							  &variter);

			uint32_value = 34;
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_close_container (&iter, &arrayiter);

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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
	}


	/* Check that an incorrect member type in the variant results in
	 * the function returning a raised error.
	 */
	TEST_FEATURE ("with incorrect variant member type (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_VARIANT_AS_STRING
							   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
							  &arrayiter);

			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);

			str_value = "name";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &variter);

			str_value = "Joe Bloggs";
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);

			str_value = "size";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &variter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_close_container (&iter, &arrayiter);

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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
	}


	/* Check that an incorrect key type in the dictionary results in
	 * the function returning a raised error.
	 */
	TEST_FEATURE ("with incorrect dict key type (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_DOUBLE_AS_STRING
							   DBUS_TYPE_VARIANT_AS_STRING
							   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
							  &arrayiter);

			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &variter);

			str_value = "Joe Bloggs";
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);

			double_value = 4.86;
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_UINT32_AS_STRING,
							  &variter);

			uint32_value = 34;
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_close_container (&iter, &arrayiter);

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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
	}


	/* Check that an incorrect member type in the dictionary results in
	 * the function returning a raised error.
	 */
	TEST_FEATURE ("with incorrect dict member type (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_DOUBLE_AS_STRING
							   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
							  &arrayiter);

			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);

			str_value = "name";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			double_value = 3.14;
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);

			str_value = "size";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			double_value = 4.86;
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_close_container (&iter, &arrayiter);

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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
	}


	/* Check that an incorrect member type in the array results in
	 * the function returning a raised error.
	 */
	TEST_FEATURE ("with incorrect array member type (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_STRING_AS_STRING,
							  &arrayiter);

			str_value = "name";
			dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "size";
			dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&iter, &arrayiter);

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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
	}


	/* Check that an incorrect type in the arguments results in the
	 * function returning a raised error.
	 */
	TEST_FEATURE ("with incorrect type (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);

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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
	}


	/* Check that a missing argument results in the function
	 * returning a raised error.
	 */
	TEST_FEATURE ("with missing argument (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
	}


	/* Check that an extra arguments results in the function
	 * returning a raised error.
	 */
	TEST_FEATURE ("with extra argument (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"GetAll"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_VARIANT_AS_STRING
							   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
							  &arrayiter);

			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);


			str_value = "name";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &variter);

			str_value = "Joe Bloggs";
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &dictiter);


			str_value = "size";
			dbus_message_iter_append_basic (&dictiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_open_container (&dictiter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_UINT32_AS_STRING,
							  &variter);

			uint32_value = 34;
			dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_close_container (&dictiter, &variter);

			dbus_message_iter_close_container (&arrayiter, &dictiter);


			dbus_message_iter_close_container (&iter, &arrayiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);

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

		properties = NULL;

		ret = my_get_all_sync (parent, proxy, &properties);

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

			TEST_EQ_P (properties, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (properties, NULL);

		nih_free (proxy);
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

	test_methods_array ();
	test_signals_array ();
	test_properties_array ();

	test_struct ();

	test_proxy_get_all_function ();
	test_proxy_get_all_notify_function ();
	test_proxy_get_all_sync_function ();

	return 0;
}
