/* nih-dbus-tool
 *
 * test_node.c - test suite for nih-dbus-tool/node.c
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

#include "type.h"
#include "node.h"
#include "interface.h"
#include "parse.h"
#include "errors.h"


void
test_path_valid (void)
{
	TEST_FUNCTION ("node_path_valid");

	/* Check that a typical node path is valid. */
	TEST_FEATURE ("with typical node path");
	TEST_TRUE (node_path_valid ("/com/netsplit/Nih/Test"));


	/* Check that an node path is not valid if it is missing the first
	 * slash.
	 */
	TEST_FEATURE ("without first slash");
	TEST_FALSE (node_path_valid ("com/netsplit/Nih/Test"));


	/* Check that an node path is not valid if it has multiple
	 * consecutive slashes.
	 */
	TEST_FEATURE ("with consecutive slashes");
	TEST_FALSE (node_path_valid ("/com//netsplit/Nih/Test"));


	/* Check that an node path is not valid if it ends in a slash. */
	TEST_FEATURE ("with final slash");
	TEST_FALSE (node_path_valid ("/com/netsplit/Nih/Test/"));


	/* Check that the root node path is valid */
	TEST_FEATURE ("with root node path");
	TEST_TRUE (node_path_valid ("/"));


	/* Check that a node path elements may contain numbers */
	TEST_FEATURE ("with numbers in node path");
	TEST_TRUE (node_path_valid ("/com/netsplit/a43b/Test"));


	/* Check that a node path elements may begin with numbers */
	TEST_FEATURE ("with numbers starting node path element");
	TEST_TRUE (node_path_valid ("/com/netsplit/43/Test"));


	/* Check that a node path elements may contain underscores */
	TEST_FEATURE ("with underscore in node path");
	TEST_TRUE (node_path_valid ("/com/netsplit/Nih_Test"));


	/* Check that a node path elements may begin with underscores */
	TEST_FEATURE ("with underscore starting node path element");
	TEST_TRUE (node_path_valid ("/com/netsplit/_Nih/Test"));


	/* Check that other characters are not permitted */
	TEST_FEATURE ("with non-permitted characters");
	TEST_FALSE (node_path_valid ("/com/netsplit/Nih.Test-Thing"));


	/* Check that an empty node path is invalid */
	TEST_FEATURE ("with empty string");
	TEST_FALSE (node_path_valid (""));
}


void
test_new (void)
{
	Node *node;

	TEST_FUNCTION ("node_new");

	/* Check that when given a name, the node structure is allocated,
	 * the defaults filled in and the name copied into the structure.
	 */
	TEST_FEATURE ("with name");
	TEST_ALLOC_FAIL {
		node = node_new (NULL, "test");

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_STR (node->path, "test");
		TEST_ALLOC_PARENT (node->path, node);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);
	}


	/* Check that when a name is not given, NULL is stored instead. */
	TEST_FEATURE ("without name");
	TEST_ALLOC_FAIL {
		node = node_new (NULL, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);
	}
}


void
test_start_tag (void)
{
	ParseContext context;
	ParseStack * parent = NULL;
	ParseStack * entry;
	XML_Parser   xmlp;
	Node *       node;
	char *       attr[5];
	int          ret;
	NihError *   err;
	FILE *       output;

	TEST_FUNCTION ("node_start_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);

	output = tmpfile ();


	/* Check that an node tag for an node with the usual name
	 * attribute results in an Node member being created and pushed
	 * onto the stack with that attribute filled in correctly.
	 */
	TEST_FEATURE ("with node and name");
	TEST_ALLOC_FAIL {
		attr[0] = "name";
		attr[1] = "/com/netsplit/Nih/Test";
		attr[2] = NULL;

		ret = node_start_tag (xmlp, "node", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack), NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_NODE);

		node = entry->node;
		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_ALLOC_PARENT (node, entry);
		TEST_EQ_STR (node->path, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (node->path, node);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (entry);
	}


	/* Check that a node tag may be missing the name attribute, and
	 * that still results in a Node member being created and pushed
	 * onto the stack with that attribute filled in correctly.
	 */
	TEST_FEATURE ("with node but no name");
	TEST_ALLOC_FAIL {
		attr[0] = NULL;

		ret = node_start_tag (xmlp, "node", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack), NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_NODE);

		node = entry->node;
		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_ALLOC_PARENT (node, entry);
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (entry);
	}


	/* Check that a node may appear inside another node tag, but that
	 * an ignored tag is pushed since we don't want to process children.
	 */
	TEST_FEATURE ("with child node");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE,
						   node_new (NULL, NULL));
		}

		attr[0] = "name";
		attr[1] = "/com/netsplit/Nih/Test";
		attr[2] = NULL;

		ret = node_start_tag (xmlp, "node", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_IGNORED);
		TEST_EQ_P (entry->data, NULL);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a node with an invalid name results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid name");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			attr[0] = "name";
			attr[1] = "Test Node";
			attr[2] = NULL;
		}

		ret = node_start_tag (xmlp, "node", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, NODE_INVALID_PATH);
		nih_free (err);
	}


	/* Check that an unknown node attribute results in a warning
	 * being printed to standard error, but is otherwise ignored
	 * and the normal processing finished.
	 */
	TEST_FEATURE ("with unknown attribute");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			attr[0] = "name";
			attr[1] = "/com/netsplit/Nih/Test";
			attr[2] = "frodo";
			attr[3] = "baggins";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = node_start_tag (xmlp, "node", attr);
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
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_NODE);

		node = entry->node;
		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_ALLOC_PARENT (node, entry);
		TEST_EQ_STR (node->path, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (node->path, node);
		TEST_LIST_EMPTY (&node->interfaces);

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown <node> attribute: "
				       "frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
	}


	/* Check that a node on top of a stack entry results in a warning
	 * being printed on standard error and an ignored element being
	 * pushed onto the stack.
	 */
	TEST_FEATURE ("with non-node on stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE,
						   interface_new (NULL, "com.netsplit.Nih.Test"));

			attr[0] = "name";
			attr[1] = "/com/netsplit/Nih/Test";
			attr[2] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = node_start_tag (xmlp, "node", attr);
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

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <node> tag\n");
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
	ParseStack * entry = NULL;
	XML_Parser   xmlp;
	Node *       node = NULL;
	int          ret;
	NihError *   err;
	void *       parent;

	TEST_FUNCTION ("node_end_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);


	/* Check that when we parse the end tag for a node, we pop
	 * the Node object off the stack and place it in the context's
	 * node member.  The stack entry should be freed and removed from
	 * the stack.
	 */
	TEST_FEATURE ("with no parent");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			entry = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);
		}

		TEST_FREE_TAG (entry);

		ret = node_end_tag (xmlp, "node");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_ORPHAN (node);
		TEST_EQ_P (context.node, node);

		nih_free (node);
		context.node = NULL;
	}


	/* Check that when the context has a parent, the new node is
	 * referenced by it.
	 */
	TEST_FEATURE ("with parent");
	context.parent = parent = nih_alloc (NULL, 1);
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			entry = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);

		}

		TEST_FREE_TAG (entry);

		ret = node_end_tag (xmlp, "node");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (node, context.parent);
		TEST_EQ_P (context.node, node);

		nih_free (node);
		context.node = NULL;
	}
	nih_free (parent);


	XML_ParserFree (xmlp);
}


void
test_lookup_interface (void)
{
	Node *     node = NULL;
	Interface *interface1 = NULL;
	Interface *interface2 = NULL;
	Interface *interface3 = NULL;
	Interface *ret;

	TEST_FUNCTION ("node_lookup_interface");


	/* Check that the function returns the interface if there is one
	 * with the given symbol.
	 */
	TEST_FEATURE ("with matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface1 = interface_new (node, "com.netsplit.Nih.Test");
			interface1->symbol = nih_strdup (interface1, "test");
			nih_list_add (&node->interfaces, &interface1->entry);

			interface2 = interface_new (node, "com.netsplit.Nih.Foo");
			nih_list_add (&node->interfaces, &interface2->entry);

			interface3 = interface_new (node, "com.netsplit.Nih.Bar");
			interface3->symbol = nih_strdup (interface3, "bar");
			nih_list_add (&node->interfaces, &interface3->entry);
		}

		ret = node_lookup_interface (node, "bar");

		TEST_EQ_P (ret, interface3);

		nih_free (node);
	}


	/* Check that the function returns the interface if there is one
	 * with no symbol and NULL is given.
	 */
	TEST_FEATURE ("with no specified symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface1 = interface_new (node, "com.netsplit.Nih.Test");
			interface1->symbol = nih_strdup (interface1, "test");
			nih_list_add (&node->interfaces, &interface1->entry);

			interface2 = interface_new (node, "com.netsplit.Nih.Foo");
			nih_list_add (&node->interfaces, &interface2->entry);

			interface3 = interface_new (node, "com.netsplit.Nih.Bar");
			interface3->symbol = nih_strdup (interface3, "bar");
			nih_list_add (&node->interfaces, &interface3->entry);
		}

		ret = node_lookup_interface (node, NULL);

		TEST_EQ_P (ret, interface2);

		nih_free (node);
	}


	/* Check that the function returns NULL if there is no interface
	 * with the given symbol.
	 */
	TEST_FEATURE ("with non-matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface1 = interface_new (node, "com.netsplit.Nih.Test");
			interface1->symbol = nih_strdup (interface1, "test");
			nih_list_add (&node->interfaces, &interface1->entry);

			interface2 = interface_new (node, "com.netsplit.Nih.Foo");
			nih_list_add (&node->interfaces, &interface2->entry);

			interface3 = interface_new (node, "com.netsplit.Nih.Bar");
			interface3->symbol = nih_strdup (interface3, "bar");
			nih_list_add (&node->interfaces, &interface3->entry);
		}

		ret = node_lookup_interface (node, "baz");

		TEST_EQ_P (ret, NULL);

		nih_free (node);
	}
}


void
test_interfaces_array (void)
{
	NihList    prototypes;
	Node *     node = NULL;
	Interface *interface = NULL;
	Method *   method = NULL;
	Signal *   signal = NULL;
	Argument * arg = NULL;
	Property * property = NULL;
	char *     str;
	TypeVar *  var;

	TEST_FUNCTION ("node_interfaces_array");

	/* Check that we can generate the interfaces array code for a
	 * node with multiple interfaces.  We want the members set up for
	 * an object implementation, so the method and property function
	 * pointers should be set and not the signal filter pointer.
	 * Since the interface structures themselves are not made static,
	 * the prototypes should contain those as well.
	 */
	TEST_FEATURE ("with object");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

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

			interface = interface_new (node, "com.netsplit.Nih.Foo");
			interface->symbol = "foo";
			nih_list_add (&node->interfaces, &interface->entry);
		}

		str = node_interfaces_array (NULL, "my", node, TRUE,
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (node);
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
			     "};\n"
			     "\n"

			     "static const NihDBusMethod my_com_netsplit_Nih_Foo_methods[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusSignal my_com_netsplit_Nih_Foo_signals[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusProperty my_com_netsplit_Nih_Foo_properties[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusInterface my_com_netsplit_Nih_Foo = {\n"
			     "\t\"com.netsplit.Nih.Foo\",\n"
			     "\tmy_com_netsplit_Nih_Foo_methods,\n"
			     "\tmy_com_netsplit_Nih_Foo_signals,\n"
			     "\tmy_com_netsplit_Nih_Foo_properties\n"
			     "};\n"
			     "\n"

			     "const NihDBusInterface *my_interfaces[] = {\n"
			     "\t&my_com_netsplit_Nih_Test,\n"
			     "\t&my_com_netsplit_Nih_Foo,\n"
			     "\tNULL\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "extern const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "extern const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Foo");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "extern const NihDBusInterface *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_interfaces");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can generate the interfaces array code for a
	 * node with multiple interfaces.  We want the members set up for
	 * a proxy implementation, so the signal filter pointer should be
	 * set but not the method or property function pointers.
	 * Since the interface structures themselves are not made static,
	 * the prototypes should contain those as well.
	 */
	TEST_FEATURE ("with proxy");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

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

			interface = interface_new (node, "com.netsplit.Nih.Foo");
			interface->symbol = "foo";
			nih_list_add (&node->interfaces, &interface->entry);
		}

		str = node_interfaces_array (NULL, "my", node, FALSE,
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (node);
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
			     "};\n"
			     "\n"

			     "static const NihDBusMethod my_com_netsplit_Nih_Foo_methods[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusSignal my_com_netsplit_Nih_Foo_signals[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "static const NihDBusProperty my_com_netsplit_Nih_Foo_properties[] = {\n"
			     "\t{ NULL }\n"
			     "};\n"
			     "\n"
			     "const NihDBusInterface my_com_netsplit_Nih_Foo = {\n"
			     "\t\"com.netsplit.Nih.Foo\",\n"
			     "\tmy_com_netsplit_Nih_Foo_methods,\n"
			     "\tmy_com_netsplit_Nih_Foo_signals,\n"
			     "\tmy_com_netsplit_Nih_Foo_properties\n"
			     "};\n"
			     "\n"

			     "const NihDBusInterface *my_interfaces[] = {\n"
			     "\t&my_com_netsplit_Nih_Test,\n"
			     "\t&my_com_netsplit_Nih_Foo,\n"
			     "\tNULL\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "extern const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "extern const NihDBusInterface");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Foo");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_FALSE (var->array);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "extern const NihDBusInterface *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_interfaces");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can still generate an interfaces array even when
	 * there are no interfaces.
	 */
	TEST_FEATURE ("with no interfaces");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);
		}

		str = node_interfaces_array (NULL, "my", node, TRUE,
					     &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusInterface *my_interfaces[] = {\n"
			     "\tNULL\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "extern const NihDBusInterface *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_interfaces");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (node);
	}
}


void
test_object_functions (void)
{
	NihList       prototypes;
	NihList       handlers;
	NihList       externs;
	Node *        node = NULL;
	Interface *   interface = NULL;
	Method *      method = NULL;
	Signal *      signal = NULL;
	Argument *    argument = NULL;
	Property *    property = NULL;
	char *        str;
	TypeFunc *    func;
	TypeVar *     arg;
	NihListEntry *attrib;

	TEST_FUNCTION ("node_object_functions");


	/* Check that we can generate all of the functions needed for a Node's
	 * object implementation, wrapping externally defined functions.  Each
	 * static function should have its prototype returned, each externally
	 * defined function should have the extern prototype returned and
	 * each API function its prototype returned.  Property functions
	 * should only be generated as access allows.
	 */
	TEST_FEATURE ("with node");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&externs);

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
		}

		str = node_object_functions (NULL, "my", node,
					     &prototypes, &handlers, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Poke_method (NihDBusObject * object,\n"
				   "                                      NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tuint32_t        address;\n"
				   "\tchar *          value;\n"
				   "\tconst char *    value_dbus;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Poke method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &address);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Poke method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &value_dbus);\n"
				   "\n"
				   "\tvalue = nih_strdup (message, value_dbus);\n"
				   "\tif (! value) {\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Poke method\"));\n"
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
				   "\tif (my_test_poke (object->data, message, address, value) < 0) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_poke_reply (NihDBusMessage *message)\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Peek_method (NihDBusObject * object,\n"
				   "                                      NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tuint32_t        address;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Peek method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &address);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Peek method\"));\n"
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
				   "\tif (my_test_peek (object->data, message, address, &value) < 0) {\n"
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
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_peek_reply (NihDBusMessage *message,\n"
				   "                    const char *    value)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
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
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_IsValidAddress_method (NihDBusObject * object,\n"
				   "                                                NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tuint32_t        address;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to IsValidAddress method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &address);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to IsValidAddress method\"));\n"
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
				   "\tif (my_test_is_valid_address (object->data, message, address) < 0) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_is_valid_address_reply (NihDBusMessage *message)\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "int\n"
				   "my_test_emit_bounce (DBusConnection *connection,\n"
				   "                     const char *    origin_path,\n"
				   "                     uint32_t        height,\n"
				   "                     int32_t         velocity)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Test\", \"Bounce\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &height)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &velocity)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the signal, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (connection, signal, NULL)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (signal);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "int\n"
				   "my_test_emit_exploded (DBusConnection *connection,\n"
				   "                       const char *    origin_path)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Test\", \"Exploded\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Send the signal, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (connection, signal, NULL)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (signal);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_colour_get (NihDBusObject *  object,\n"
				   "                                     NihDBusMessage * message,\n"
				   "                                     DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_get_colour (object->data, message, &value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Append a variant onto the message to contain the property value. */\n"
				   "\tif (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"s\", &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (iter, &variter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Finish the variant */\n"
				   "\tif (! dbus_message_iter_close_container (iter, &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_colour_set (NihDBusObject *  object,\n"
				   "                                     NihDBusMessage * message,\n"
				   "                                     DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tconst char *    value_dbus;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Recurse into the variant */\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (iter, &variter);\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&variter, &value_dbus);\n"
				   "\n"
				   "\tvalue = nih_strdup (message, value_dbus);\n"
				   "\tif (! value) {\n"
				   "\t\tnih_error_raise_no_memory ();\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_set_colour (object->data, message, value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_size_get (NihDBusObject *  object,\n"
				   "                                   NihDBusMessage * message,\n"
				   "                                   DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tuint32_t        value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_get_size (object->data, message, &value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Append a variant onto the message to contain the property value. */\n"
				   "\tif (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"u\", &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (iter, &variter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Finish the variant */\n"
				   "\tif (! dbus_message_iter_close_container (iter, &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_touch_set (NihDBusObject *  object,\n"
				   "                                    NihDBusMessage * message,\n"
				   "                                    DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tint             value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Recurse into the variant */\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (iter, &variter);\n"
				   "\n"
				   "\t/* Demarshal a int from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_BOOLEAN) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&variter, &value);\n"
				   "\n"
				   "\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_set_touch (object->data, message, value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Foo_Bing_method (NihDBusObject * object,\n"
				   "                                     NihDBusMessage *message)\n"
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
				   "\t\t                                _(\"Invalid arguments to Bing method\"));\n"
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
				   "\tif (my_foo_bing (object->data, message) < 0) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_foo_bing_reply (NihDBusMessage *message)\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "int\n"
				   "my_foo_emit_new_result (DBusConnection *connection,\n"
				   "                        const char *    origin_path)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Foo\", \"NewResult\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Send the signal, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (connection, signal, NULL)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (signal);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

		/* Poke */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Poke_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke_reply");
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


		/* Peek */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Peek_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek_reply");
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
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* IsValidAddress */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_IsValidAddress_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address_reply");
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


		/* bounce */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_emit_bounce");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "origin_path");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "height");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);


		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "velocity");
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


		/* exploded */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_emit_exploded");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "origin_path");
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


		/* colour (get) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_get");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour");
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
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* colour (set) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_set");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour");
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
		TEST_EQ_STR (arg->name, "value");
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


		/* size (get) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_size_get");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size");
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
		TEST_EQ_STR (arg->type, "uint32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* touch (set) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_touch_set");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* Bing */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Foo_Bing_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing_reply");
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


		/* NewResult */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_emit_new_result");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "origin_path");
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
		TEST_LIST_EMPTY (&handlers);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can still generate all of the functions even if
	 * there are no methods in the object implementation.
	 */
	TEST_FEATURE ("with no methods");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&externs);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

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

			signal = signal_new (interface, "NewResult");
			signal->symbol = "new_result";
			nih_list_add (&interface->signals, &signal->entry);
		}

		str = node_object_functions (NULL, "my", node,
					     &prototypes, &handlers, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_test_emit_bounce (DBusConnection *connection,\n"
				   "                     const char *    origin_path,\n"
				   "                     uint32_t        height,\n"
				   "                     int32_t         velocity)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Test\", \"Bounce\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &height)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &velocity)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the signal, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (connection, signal, NULL)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (signal);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "int\n"
				   "my_test_emit_exploded (DBusConnection *connection,\n"
				   "                       const char *    origin_path)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Test\", \"Exploded\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Send the signal, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (connection, signal, NULL)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (signal);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_colour_get (NihDBusObject *  object,\n"
				   "                                     NihDBusMessage * message,\n"
				   "                                     DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_get_colour (object->data, message, &value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Append a variant onto the message to contain the property value. */\n"
				   "\tif (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"s\", &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (iter, &variter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Finish the variant */\n"
				   "\tif (! dbus_message_iter_close_container (iter, &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_colour_set (NihDBusObject *  object,\n"
				   "                                     NihDBusMessage * message,\n"
				   "                                     DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tconst char *    value_dbus;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Recurse into the variant */\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (iter, &variter);\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&variter, &value_dbus);\n"
				   "\n"
				   "\tvalue = nih_strdup (message, value_dbus);\n"
				   "\tif (! value) {\n"
				   "\t\tnih_error_raise_no_memory ();\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_set_colour (object->data, message, value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_size_get (NihDBusObject *  object,\n"
				   "                                   NihDBusMessage * message,\n"
				   "                                   DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tuint32_t        value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_get_size (object->data, message, &value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Append a variant onto the message to contain the property value. */\n"
				   "\tif (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"u\", &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (iter, &variter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Finish the variant */\n"
				   "\tif (! dbus_message_iter_close_container (iter, &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_touch_set (NihDBusObject *  object,\n"
				   "                                    NihDBusMessage * message,\n"
				   "                                    DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tint             value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Recurse into the variant */\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (iter, &variter);\n"
				   "\n"
				   "\t/* Demarshal a int from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_BOOLEAN) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&variter, &value);\n"
				   "\n"
				   "\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_set_touch (object->data, message, value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "int\n"
				   "my_foo_emit_new_result (DBusConnection *connection,\n"
				   "                        const char *    origin_path)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Foo\", \"NewResult\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Send the signal, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (connection, signal, NULL)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (signal);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

		/* bounce */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_emit_bounce");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "origin_path");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "height");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);


		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "velocity");
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


		/* exploded */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_emit_exploded");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "origin_path");
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


		/* colour (get) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_get");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour");
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
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* colour (set) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_set");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour");
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
		TEST_EQ_STR (arg->name, "value");
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


		/* size (get) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_size_get");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size");
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
		TEST_EQ_STR (arg->type, "uint32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* touch (set) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_touch_set");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* NewResult */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_emit_new_result");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "origin_path");
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
		TEST_LIST_EMPTY (&handlers);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can still generate all of the functions even if
	 * there are no signals in the object implementation.
	 */
	TEST_FEATURE ("with no signals");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&externs);

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
		}

		str = node_object_functions (NULL, "my", node,
					     &prototypes, &handlers, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Poke_method (NihDBusObject * object,\n"
				   "                                      NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tuint32_t        address;\n"
				   "\tchar *          value;\n"
				   "\tconst char *    value_dbus;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Poke method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &address);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Poke method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &value_dbus);\n"
				   "\n"
				   "\tvalue = nih_strdup (message, value_dbus);\n"
				   "\tif (! value) {\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Poke method\"));\n"
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
				   "\tif (my_test_poke (object->data, message, address, value) < 0) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_poke_reply (NihDBusMessage *message)\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Peek_method (NihDBusObject * object,\n"
				   "                                      NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tuint32_t        address;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Peek method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &address);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Peek method\"));\n"
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
				   "\tif (my_test_peek (object->data, message, address, &value) < 0) {\n"
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
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_peek_reply (NihDBusMessage *message,\n"
				   "                    const char *    value)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
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
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_IsValidAddress_method (NihDBusObject * object,\n"
				   "                                                NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tuint32_t        address;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to IsValidAddress method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &address);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to IsValidAddress method\"));\n"
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
				   "\tif (my_test_is_valid_address (object->data, message, address) < 0) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_is_valid_address_reply (NihDBusMessage *message)\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_colour_get (NihDBusObject *  object,\n"
				   "                                     NihDBusMessage * message,\n"
				   "                                     DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_get_colour (object->data, message, &value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Append a variant onto the message to contain the property value. */\n"
				   "\tif (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"s\", &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (iter, &variter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Finish the variant */\n"
				   "\tif (! dbus_message_iter_close_container (iter, &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_colour_set (NihDBusObject *  object,\n"
				   "                                     NihDBusMessage * message,\n"
				   "                                     DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tconst char *    value_dbus;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Recurse into the variant */\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (iter, &variter);\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&variter, &value_dbus);\n"
				   "\n"
				   "\tvalue = nih_strdup (message, value_dbus);\n"
				   "\tif (! value) {\n"
				   "\t\tnih_error_raise_no_memory ();\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_set_colour (object->data, message, value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_size_get (NihDBusObject *  object,\n"
				   "                                   NihDBusMessage * message,\n"
				   "                                   DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tuint32_t        value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_get_size (object->data, message, &value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Append a variant onto the message to contain the property value. */\n"
				   "\tif (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"u\", &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (iter, &variter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Finish the variant */\n"
				   "\tif (! dbus_message_iter_close_container (iter, &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_touch_set (NihDBusObject *  object,\n"
				   "                                    NihDBusMessage * message,\n"
				   "                                    DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tint             value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Recurse into the variant */\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (iter, &variter);\n"
				   "\n"
				   "\t/* Demarshal a int from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_BOOLEAN) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&variter, &value);\n"
				   "\n"
				   "\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_set_touch (object->data, message, value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Foo_Bing_method (NihDBusObject * object,\n"
				   "                                     NihDBusMessage *message)\n"
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
				   "\t\t                                _(\"Invalid arguments to Bing method\"));\n"
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
				   "\tif (my_foo_bing (object->data, message) < 0) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_foo_bing_reply (NihDBusMessage *message)\n"
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

		/* Poke */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Poke_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke_reply");
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


		/* Peek */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Peek_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek_reply");
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
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* IsValidAddress */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_IsValidAddress_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address_reply");
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


		/* colour (get) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_get");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour");
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
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* colour (set) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_set");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour");
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
		TEST_EQ_STR (arg->name, "value");
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


		/* size (get) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_size_get");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size");
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
		TEST_EQ_STR (arg->type, "uint32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* touch (set) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_touch_set");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* Bing */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Foo_Bing_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing_reply");
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
		TEST_LIST_EMPTY (&handlers);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can still generate all of the functions even if
	 * there are no properties in the object implementation.
	 */
	TEST_FEATURE ("with no properties");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&externs);

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


			interface = interface_new (node, "com.netsplit.Nih.Foo");
			interface->symbol = "foo";
			nih_list_add (&node->interfaces, &interface->entry);

			method = method_new (interface, "Bing");
			method->symbol = "bing";
			nih_list_add (&interface->methods, &method->entry);

			signal = signal_new (interface, "NewResult");
			signal->symbol = "new_result";
			nih_list_add (&interface->signals, &signal->entry);
		}

		str = node_object_functions (NULL, "my", node,
					     &prototypes, &handlers, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Poke_method (NihDBusObject * object,\n"
				   "                                      NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tuint32_t        address;\n"
				   "\tchar *          value;\n"
				   "\tconst char *    value_dbus;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Poke method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &address);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Poke method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &value_dbus);\n"
				   "\n"
				   "\tvalue = nih_strdup (message, value_dbus);\n"
				   "\tif (! value) {\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Poke method\"));\n"
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
				   "\tif (my_test_poke (object->data, message, address, value) < 0) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_poke_reply (NihDBusMessage *message)\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Peek_method (NihDBusObject * object,\n"
				   "                                      NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tuint32_t        address;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Peek method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &address);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to Peek method\"));\n"
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
				   "\tif (my_test_peek (object->data, message, address, &value) < 0) {\n"
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
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_peek_reply (NihDBusMessage *message,\n"
				   "                    const char *    value)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
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
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_IsValidAddress_method (NihDBusObject * object,\n"
				   "                                                NihDBusMessage *message)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tuint32_t        address;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\n"
				   "\t/* Iterate the arguments to the message and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to IsValidAddress method\"));\n"
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
				   "\tdbus_message_iter_get_basic (&iter, &address);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                                _(\"Invalid arguments to IsValidAddress method\"));\n"
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
				   "\tif (my_test_is_valid_address (object->data, message, address) < 0) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_is_valid_address_reply (NihDBusMessage *message)\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "int\n"
				   "my_test_emit_bounce (DBusConnection *connection,\n"
				   "                     const char *    origin_path,\n"
				   "                     uint32_t        height,\n"
				   "                     int32_t         velocity)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Test\", \"Bounce\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &height)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &velocity)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Send the signal, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (connection, signal, NULL)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (signal);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "int\n"
				   "my_test_emit_exploded (DBusConnection *connection,\n"
				   "                       const char *    origin_path)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Test\", \"Exploded\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Send the signal, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (connection, signal, NULL)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (signal);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Foo_Bing_method (NihDBusObject * object,\n"
				   "                                     NihDBusMessage *message)\n"
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
				   "\t\t                                _(\"Invalid arguments to Bing method\"));\n"
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
				   "\tif (my_foo_bing (object->data, message) < 0) {\n"
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
				   "}\n"
				   "\n"
				   "int\n"
				   "my_foo_bing_reply (NihDBusMessage *message)\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "int\n"
				   "my_foo_emit_new_result (DBusConnection *connection,\n"
				   "                        const char *    origin_path)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Foo\", \"NewResult\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Send the signal, appending it to the outgoing queue. */\n"
				   "\tif (! dbus_connection_send (connection, signal, NULL)) {\n"
				   "\t\tdbus_message_unref (signal);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (signal);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

		/* Poke */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Poke_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke_reply");
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


		/* Peek */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Peek_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek_reply");
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
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* IsValidAddress */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_IsValidAddress_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address_reply");
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


		/* bounce */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_emit_bounce");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "origin_path");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "height");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);


		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "velocity");
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


		/* exploded */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_emit_exploded");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "origin_path");
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


		/* Bing */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Foo_Bing_method");
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

		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing_reply");
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


		/* NewResult */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_emit_new_result");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "origin_path");
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
		TEST_LIST_EMPTY (&handlers);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can still generate all of the functions even if
	 * there are no methods or signals in the object implementation.
	 */
	TEST_FEATURE ("with no methods or signals");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&externs);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

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
		}

		str = node_object_functions (NULL, "my", node,
					     &prototypes, &handlers, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("static int\n"
				   "my_com_netsplit_Nih_Test_colour_get (NihDBusObject *  object,\n"
				   "                                     NihDBusMessage * message,\n"
				   "                                     DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_get_colour (object->data, message, &value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Append a variant onto the message to contain the property value. */\n"
				   "\tif (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"s\", &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (iter, &variter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Finish the variant */\n"
				   "\tif (! dbus_message_iter_close_container (iter, &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_colour_set (NihDBusObject *  object,\n"
				   "                                     NihDBusMessage * message,\n"
				   "                                     DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tconst char *    value_dbus;\n"
				   "\tchar *          value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Recurse into the variant */\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (iter, &variter);\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&variter, &value_dbus);\n"
				   "\n"
				   "\tvalue = nih_strdup (message, value_dbus);\n"
				   "\tif (! value) {\n"
				   "\t\tnih_error_raise_no_memory ();\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to colour property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_set_colour (object->data, message, value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_size_get (NihDBusObject *  object,\n"
				   "                                   NihDBusMessage * message,\n"
				   "                                   DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tuint32_t        value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_get_size (object->data, message, &value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Append a variant onto the message to contain the property value. */\n"
				   "\tif (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"u\", &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_UINT32, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (iter, &variter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Finish the variant */\n"
				   "\tif (! dbus_message_iter_close_container (iter, &variter))\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static int\n"
				   "my_com_netsplit_Nih_Test_touch_set (NihDBusObject *  object,\n"
				   "                                    NihDBusMessage * message,\n"
				   "                                    DBusMessageIter *iter)\n"
				   "{\n"
				   "\tDBusMessageIter variter;\n"
				   "\tint             value;\n"
				   "\n"
				   "\tnih_assert (object != NULL);\n"
				   "\tnih_assert (message != NULL);\n"
				   "\tnih_assert (iter != NULL);\n"
				   "\n"
				   "\t/* Recurse into the variant */\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (iter, &variter);\n"
				   "\n"
				   "\t/* Demarshal a int from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_BOOLEAN) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&variter, &value);\n"
				   "\n"
				   "\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				   "\t\t                             _(\"Invalid arguments to touch property\"));\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (my_test_set_touch (object->data, message, value) < 0)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"));

		/* colour (get) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_get");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour");
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
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* colour (set) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_set");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour");
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
		TEST_EQ_STR (arg->name, "value");
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


		/* size (get) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_size_get");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size");
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
		TEST_EQ_STR (arg->type, "uint32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* touch (set) */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_touch_set");
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

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessageIter *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "iter");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "extern int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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
		TEST_LIST_EMPTY (&handlers);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that when there are no interface members, an empty string
	 * is returned.
	 */
	TEST_FEATURE ("with no members");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&externs);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

			interface = interface_new (node, "com.netsplit.Nih.Foo");
			interface->symbol = "foo";
			nih_list_add (&node->interfaces, &interface->entry);
		}

		str = node_object_functions (NULL, "my", node,
					     &prototypes, &handlers, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, "");

		TEST_LIST_EMPTY (&prototypes);
		TEST_LIST_EMPTY (&handlers);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that when there are no interfaces, an empty string
	 * is returned.
	 */
	TEST_FEATURE ("with no interfaces");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&externs);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);
		}

		str = node_object_functions (NULL, "my", node,
					     &prototypes, &handlers, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, "");

		TEST_LIST_EMPTY (&prototypes);
		TEST_LIST_EMPTY (&handlers);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}
}

void
test_proxy_functions (void)
{
	NihList       prototypes;
	NihList       typedefs;
	NihList       externs;
	Node *        node = NULL;
	Interface *   interface = NULL;
	Method *      method = NULL;
	Signal *      signal = NULL;
	Argument *    argument = NULL;
	Property *    property = NULL;
	char *        str;
	TypeFunc *    func;
	TypeVar *     arg;
	NihListEntry *attrib;

	TEST_FUNCTION ("node_proxy_functions");


	/* Check that we can generate all of the functions needed for a remote
	 * Node's proxy implementation.  Each static function should have its
	 * prototype returned, each callback or handler function should have
	 * its typedef returned and each API function its prototype returned.
	 * Property functions should only be generated as access allows.
	 */
	TEST_FEATURE ("with node");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&externs);

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
		}

		str = node_proxy_functions (NULL, "my", node,
					    &prototypes, &typedefs, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("DBusPendingCall *\n"
				   "my_test_poke (NihDBusProxy *      proxy,\n"
				   "              uint32_t            address,\n"
				   "              const char *        value,\n"
				   "              MyTestPokeReply     handler,\n"
				   "              NihDBusErrorHandler error_handler,\n"
				   "              void *              data,\n"
				   "              int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Poke\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_Poke_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_Poke_notify (DBusPendingCall *   pending_call,\n"
				   "                                      NihDBusPendingData *pending_data)\n"
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
				   "\t\t((MyTestPokeReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_poke_sync (const void *  parent,\n"
				   "                   NihDBusProxy *proxy,\n"
				   "                   uint32_t      address,\n"
				   "                   const char *  value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Poke\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_peek (NihDBusProxy *      proxy,\n"
				   "              uint32_t            address,\n"
				   "              MyTestPeekReply     handler,\n"
				   "              NihDBusErrorHandler error_handler,\n"
				   "              void *              data,\n"
				   "              int                 timeout)\n"
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
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Peek\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_Peek_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_Peek_notify (DBusPendingCall *   pending_call,\n"
				   "                                      NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tchar *          value;\n"
				   "\tconst char *    value_dbus;\n"
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
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&iter, &value_dbus);\n"
				   "\n"
				   "\t\tvalue = nih_strdup (message, value_dbus);\n"
				   "\t\tif (! value) {\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tmessage = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
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
				   "\t\t((MyTestPeekReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_peek_sync (const void *  parent,\n"
				   "                   NihDBusProxy *proxy,\n"
				   "                   uint32_t      address,\n"
				   "                   char **       value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar *          value_local;\n"
				   "\tconst char *    value_local_dbus;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Peek\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&iter, &value_local_dbus);\n"
				   "\n"
				   "\t\tvalue_local = nih_strdup (parent, value_local_dbus);\n"
				   "\t\tif (! value_local) {\n"
				   "\t\t\t*value = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\t*value = value_local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (value_local);\n"
				   "\t\t*value = NULL;\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_is_valid_address (NihDBusProxy *            proxy,\n"
				   "                          uint32_t                  address,\n"
				   "                          MyTestIsValidAddressReply handler,\n"
				   "                          NihDBusErrorHandler       error_handler,\n"
				   "                          void *                    data,\n"
				   "                          int                       timeout)\n"
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
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"IsValidAddress\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_IsValidAddress_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_IsValidAddress_notify (DBusPendingCall *   pending_call,\n"
				   "                                                NihDBusPendingData *pending_data)\n"
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
				   "\t\t((MyTestIsValidAddressReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_is_valid_address_sync (const void *  parent,\n"
				   "                               NihDBusProxy *proxy,\n"
				   "                               uint32_t      address)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"IsValidAddress\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Bounce_signal (DBusConnection *    connection,\n"
				   "                                        DBusMessage *       signal,\n"
				   "                                        NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tuint32_t        height;\n"
				   "\tint32_t         velocity;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->name))\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tmessage = nih_dbus_message_new (NULL, connection, signal);\n"
				   "\tif (! message)\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t/* Iterate the arguments to the signal and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &height);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t/* Demarshal a int32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &velocity);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestBounceHandler)proxied->handler) (proxied->data, message, height, velocity);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Exploded_signal (DBusConnection *    connection,\n"
				   "                                          DBusMessage *       signal,\n"
				   "                                          NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->name))\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tmessage = nih_dbus_message_new (NULL, connection, signal);\n"
				   "\tif (! message)\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t/* Iterate the arguments to the signal and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestExplodedHandler)proxied->handler) (proxied->data, message);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_get_colour (NihDBusProxy *       proxy,\n"
				   "                    MyTestGetColourReply handler,\n"
				   "                    NihDBusErrorHandler  error_handler,\n"
				   "                    void *               data,\n"
				   "                    int                  timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler != NULL) && (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_get_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_colour_get_notify (DBusPendingCall *   pending_call,\n"
				   "                                            NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tconst char *    value_dbus;\n"
				   "\tchar *          value;\n"
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
				   "\t\t * over and recurse into the arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
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
				   "\t\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&variter, &value_dbus);\n"
				   "\n"
				   "\t\tvalue = nih_strdup (message, value_dbus);\n"
				   "\t\tif (! value) {\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tmessage = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
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
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestGetColourReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_get_colour_sync (const void *  parent,\n"
				   "                         NihDBusProxy *proxy,\n"
				   "                         char **       value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\tconst char *    local_dbus;\n"
				   "\tchar *          local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
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
				   "\t/* Iterate the method arguments, recursing into the variant */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&variter, &local_dbus);\n"
				   "\n"
				   "\t\tlocal = nih_strdup (parent, local_dbus);\n"
				   "\t\tif (! local) {\n"
				   "\t\t\t*value = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\t\t*value = local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_set_colour (NihDBusProxy *       proxy,\n"
				   "                    const char *         value,\n"
				   "                    MyTestSetColourReply handler,\n"
				   "                    NihDBusErrorHandler  error_handler,\n"
				   "                    void *               data,\n"
				   "                    int                  timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusMessageIter     variter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"s\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_set_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_colour_set_notify (DBusPendingCall *   pending_call,\n"
				   "                                            NihDBusPendingData *pending_data)\n"
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
				   "\t/* Create a message context for the reply, and check\n"
				   "\t * there are no arguments.\n"
				   "\t */\n"
				   "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyTestSetColourReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_set_colour_sync (const void *  parent,\n"
				   "                         NihDBusProxy *proxy,\n"
				   "                         const char *  value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"s\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\t/* Check the reply has no arguments */\n"
				   "\tdbus_message_unref (method_call);\n"
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
 				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_get_size (NihDBusProxy *      proxy,\n"
				   "                  MyTestGetSizeReply  handler,\n"
				   "                  NihDBusErrorHandler error_handler,\n"
				   "                  void *              data,\n"
				   "                  int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler != NULL) && (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"size\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_size_get_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_size_get_notify (DBusPendingCall *   pending_call,\n"
				   "                                          NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tuint32_t        value;\n"
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
				   "\t\t * over and recurse into the arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
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
				   "\t\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\t\t/* Demarshal a uint32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&variter, &value);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
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
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestGetSizeReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_get_size_sync (const void *  parent,\n"
				   "                       NihDBusProxy *proxy,\n"
				   "                       uint32_t *    value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\tuint32_t        local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"size\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
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
				   "\t/* Iterate the method arguments, recursing into the variant */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a uint32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&variter, &local);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\t\t*value = local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_set_touch (NihDBusProxy *      proxy,\n"
				   "                   int                 value,\n"
				   "                   MyTestSetTouchReply handler,\n"
				   "                   NihDBusErrorHandler error_handler,\n"
				   "                   void *              data,\n"
				   "                   int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusMessageIter     variter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"touch\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"b\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_touch_set_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_touch_set_notify (DBusPendingCall *   pending_call,\n"
				   "                                           NihDBusPendingData *pending_data)\n"
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
				   "\t/* Create a message context for the reply, and check\n"
				   "\t * there are no arguments.\n"
				   "\t */\n"
				   "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyTestSetTouchReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_set_touch_sync (const void *  parent,\n"
				   "                        NihDBusProxy *proxy,\n"
				   "                        int           value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"touch\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"b\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\t/* Check the reply has no arguments */\n"
				   "\tdbus_message_unref (method_call);\n"
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
 				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_foo_bing (NihDBusProxy *      proxy,\n"
				   "             MyFooBingReply      handler,\n"
				   "             NihDBusErrorHandler error_handler,\n"
				   "             void *              data,\n"
				   "             int                 timeout)\n"
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
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Foo\", \"Bing\");\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Foo_Bing_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Foo_Bing_notify (DBusPendingCall *   pending_call,\n"
				   "                                     NihDBusPendingData *pending_data)\n"
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
				   "\t\t((MyFooBingReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_foo_bing_sync (const void *  parent,\n"
				   "                  NihDBusProxy *proxy)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Foo\", \"Bing\");\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Foo_NewResult_signal (DBusConnection *    connection,\n"
				   "                                          DBusMessage *       signal,\n"
				   "                                          NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->name))\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tmessage = nih_dbus_message_new (NULL, connection, signal);\n"
				   "\tif (! message)\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t/* Iterate the arguments to the signal and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\t((MyFooNewResultHandler)proxied->handler) (proxied->data, message);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"));


		/* Poke */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestPokeReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Poke_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestPokeReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* Peek */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestPeekReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Peek_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestPeekReply)");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* IsValidAddress */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestIsValidAddressReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_IsValidAddress_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestIsValidAddressReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
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


		/* Bounce */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Bounce_signal");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "signal");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxySignal *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxied");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestBounceHandler)");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "height");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "velocity");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		/* Exploded */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Exploded_signal");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "signal");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxySignal *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxied");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestExplodedHandler)");
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


		/* colour (get) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour");
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
		TEST_EQ_STR (arg->type, "MyTestGetColourReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_get_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestGetColourReply)");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour_sync");
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
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* colour (set) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestSetColourReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_set_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestSetColourReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour_sync");
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
		TEST_EQ_STR (arg->name, "value");
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


		/* size (get) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size");
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
		TEST_EQ_STR (arg->type, "MyTestGetSizeReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_size_get_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestGetSizeReply)");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* touch (set) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestSetTouchReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_touch_set_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestSetTouchReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch_sync");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* Bing */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing");
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
		TEST_EQ_STR (arg->type, "MyFooBingReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Foo_Bing_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyFooBingReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing_sync");
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


		/* NewResult */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Foo_NewResult_signal");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "signal");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxySignal *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxied");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyFooNewResultHandler)");
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


		TEST_LIST_EMPTY (&prototypes);
		TEST_LIST_EMPTY (&typedefs);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can still generate all of the functions even if
	 * there are no methods in the object implementation.
	 */
	TEST_FEATURE ("with no methods");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&externs);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

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

			signal = signal_new (interface, "NewResult");
			signal->symbol = "new_result";
			nih_list_add (&interface->signals, &signal->entry);
		}

		str = node_proxy_functions (NULL, "my", node,
					    &prototypes, &typedefs, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Bounce_signal (DBusConnection *    connection,\n"
				   "                                        DBusMessage *       signal,\n"
				   "                                        NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tuint32_t        height;\n"
				   "\tint32_t         velocity;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->name))\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tmessage = nih_dbus_message_new (NULL, connection, signal);\n"
				   "\tif (! message)\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t/* Iterate the arguments to the signal and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &height);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t/* Demarshal a int32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &velocity);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestBounceHandler)proxied->handler) (proxied->data, message, height, velocity);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Exploded_signal (DBusConnection *    connection,\n"
				   "                                          DBusMessage *       signal,\n"
				   "                                          NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->name))\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tmessage = nih_dbus_message_new (NULL, connection, signal);\n"
				   "\tif (! message)\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t/* Iterate the arguments to the signal and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestExplodedHandler)proxied->handler) (proxied->data, message);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_get_colour (NihDBusProxy *       proxy,\n"
				   "                    MyTestGetColourReply handler,\n"
				   "                    NihDBusErrorHandler  error_handler,\n"
				   "                    void *               data,\n"
				   "                    int                  timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler != NULL) && (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_get_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_colour_get_notify (DBusPendingCall *   pending_call,\n"
				   "                                            NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tconst char *    value_dbus;\n"
				   "\tchar *          value;\n"
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
				   "\t\t * over and recurse into the arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
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
				   "\t\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&variter, &value_dbus);\n"
				   "\n"
				   "\t\tvalue = nih_strdup (message, value_dbus);\n"
				   "\t\tif (! value) {\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tmessage = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
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
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestGetColourReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_get_colour_sync (const void *  parent,\n"
				   "                         NihDBusProxy *proxy,\n"
				   "                         char **       value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\tconst char *    local_dbus;\n"
				   "\tchar *          local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
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
				   "\t/* Iterate the method arguments, recursing into the variant */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&variter, &local_dbus);\n"
				   "\n"
				   "\t\tlocal = nih_strdup (parent, local_dbus);\n"
				   "\t\tif (! local) {\n"
				   "\t\t\t*value = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\t\t*value = local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_set_colour (NihDBusProxy *       proxy,\n"
				   "                    const char *         value,\n"
				   "                    MyTestSetColourReply handler,\n"
				   "                    NihDBusErrorHandler  error_handler,\n"
				   "                    void *               data,\n"
				   "                    int                  timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusMessageIter     variter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"s\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_set_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_colour_set_notify (DBusPendingCall *   pending_call,\n"
				   "                                            NihDBusPendingData *pending_data)\n"
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
				   "\t/* Create a message context for the reply, and check\n"
				   "\t * there are no arguments.\n"
				   "\t */\n"
				   "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyTestSetColourReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_set_colour_sync (const void *  parent,\n"
				   "                         NihDBusProxy *proxy,\n"
				   "                         const char *  value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"s\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\t/* Check the reply has no arguments */\n"
				   "\tdbus_message_unref (method_call);\n"
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
 				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_get_size (NihDBusProxy *      proxy,\n"
				   "                  MyTestGetSizeReply  handler,\n"
				   "                  NihDBusErrorHandler error_handler,\n"
				   "                  void *              data,\n"
				   "                  int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler != NULL) && (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"size\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_size_get_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_size_get_notify (DBusPendingCall *   pending_call,\n"
				   "                                          NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tuint32_t        value;\n"
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
				   "\t\t * over and recurse into the arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
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
				   "\t\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\t\t/* Demarshal a uint32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&variter, &value);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
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
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestGetSizeReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_get_size_sync (const void *  parent,\n"
				   "                       NihDBusProxy *proxy,\n"
				   "                       uint32_t *    value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\tuint32_t        local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"size\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
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
				   "\t/* Iterate the method arguments, recursing into the variant */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a uint32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&variter, &local);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\t\t*value = local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_set_touch (NihDBusProxy *      proxy,\n"
				   "                   int                 value,\n"
				   "                   MyTestSetTouchReply handler,\n"
				   "                   NihDBusErrorHandler error_handler,\n"
				   "                   void *              data,\n"
				   "                   int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusMessageIter     variter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"touch\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"b\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_touch_set_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_touch_set_notify (DBusPendingCall *   pending_call,\n"
				   "                                           NihDBusPendingData *pending_data)\n"
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
				   "\t/* Create a message context for the reply, and check\n"
				   "\t * there are no arguments.\n"
				   "\t */\n"
				   "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyTestSetTouchReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_set_touch_sync (const void *  parent,\n"
				   "                        NihDBusProxy *proxy,\n"
				   "                        int           value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"touch\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"b\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\t/* Check the reply has no arguments */\n"
				   "\tdbus_message_unref (method_call);\n"
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
 				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Foo_NewResult_signal (DBusConnection *    connection,\n"
				   "                                          DBusMessage *       signal,\n"
				   "                                          NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->name))\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tmessage = nih_dbus_message_new (NULL, connection, signal);\n"
				   "\tif (! message)\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t/* Iterate the arguments to the signal and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\t((MyFooNewResultHandler)proxied->handler) (proxied->data, message);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"));


		/* Bounce */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Bounce_signal");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "signal");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxySignal *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxied");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestBounceHandler)");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "height");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "velocity");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		/* Exploded */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Exploded_signal");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "signal");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxySignal *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxied");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestExplodedHandler)");
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


		/* colour (get) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour");
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
		TEST_EQ_STR (arg->type, "MyTestGetColourReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_get_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestGetColourReply)");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour_sync");
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
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* colour (set) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestSetColourReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_set_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestSetColourReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour_sync");
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
		TEST_EQ_STR (arg->name, "value");
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


		/* size (get) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size");
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
		TEST_EQ_STR (arg->type, "MyTestGetSizeReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_size_get_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestGetSizeReply)");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* touch (set) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestSetTouchReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_touch_set_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestSetTouchReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch_sync");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* NewResult */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Foo_NewResult_signal");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "signal");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxySignal *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxied");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyFooNewResultHandler)");
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


		TEST_LIST_EMPTY (&prototypes);
		TEST_LIST_EMPTY (&typedefs);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can still generate all of the functions even if
	 * there are no signals in the object implementation.
	 */
	TEST_FEATURE ("with no signals");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&externs);

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
		}

		str = node_proxy_functions (NULL, "my", node,
					    &prototypes, &typedefs, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("DBusPendingCall *\n"
				   "my_test_poke (NihDBusProxy *      proxy,\n"
				   "              uint32_t            address,\n"
				   "              const char *        value,\n"
				   "              MyTestPokeReply     handler,\n"
				   "              NihDBusErrorHandler error_handler,\n"
				   "              void *              data,\n"
				   "              int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Poke\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_Poke_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_Poke_notify (DBusPendingCall *   pending_call,\n"
				   "                                      NihDBusPendingData *pending_data)\n"
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
				   "\t\t((MyTestPokeReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_poke_sync (const void *  parent,\n"
				   "                   NihDBusProxy *proxy,\n"
				   "                   uint32_t      address,\n"
				   "                   const char *  value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Poke\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_peek (NihDBusProxy *      proxy,\n"
				   "              uint32_t            address,\n"
				   "              MyTestPeekReply     handler,\n"
				   "              NihDBusErrorHandler error_handler,\n"
				   "              void *              data,\n"
				   "              int                 timeout)\n"
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
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Peek\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_Peek_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_Peek_notify (DBusPendingCall *   pending_call,\n"
				   "                                      NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tchar *          value;\n"
				   "\tconst char *    value_dbus;\n"
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
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&iter, &value_dbus);\n"
				   "\n"
				   "\t\tvalue = nih_strdup (message, value_dbus);\n"
				   "\t\tif (! value) {\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tmessage = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
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
				   "\t\t((MyTestPeekReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_peek_sync (const void *  parent,\n"
				   "                   NihDBusProxy *proxy,\n"
				   "                   uint32_t      address,\n"
				   "                   char **       value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar *          value_local;\n"
				   "\tconst char *    value_local_dbus;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Peek\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&iter, &value_local_dbus);\n"
				   "\n"
				   "\t\tvalue_local = nih_strdup (parent, value_local_dbus);\n"
				   "\t\tif (! value_local) {\n"
				   "\t\t\t*value = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\t*value = value_local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (value_local);\n"
				   "\t\t*value = NULL;\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_is_valid_address (NihDBusProxy *            proxy,\n"
				   "                          uint32_t                  address,\n"
				   "                          MyTestIsValidAddressReply handler,\n"
				   "                          NihDBusErrorHandler       error_handler,\n"
				   "                          void *                    data,\n"
				   "                          int                       timeout)\n"
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
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"IsValidAddress\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_IsValidAddress_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_IsValidAddress_notify (DBusPendingCall *   pending_call,\n"
				   "                                                NihDBusPendingData *pending_data)\n"
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
				   "\t\t((MyTestIsValidAddressReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_is_valid_address_sync (const void *  parent,\n"
				   "                               NihDBusProxy *proxy,\n"
				   "                               uint32_t      address)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"IsValidAddress\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_get_colour (NihDBusProxy *       proxy,\n"
				   "                    MyTestGetColourReply handler,\n"
				   "                    NihDBusErrorHandler  error_handler,\n"
				   "                    void *               data,\n"
				   "                    int                  timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler != NULL) && (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_get_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_colour_get_notify (DBusPendingCall *   pending_call,\n"
				   "                                            NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tconst char *    value_dbus;\n"
				   "\tchar *          value;\n"
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
				   "\t\t * over and recurse into the arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
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
				   "\t\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&variter, &value_dbus);\n"
				   "\n"
				   "\t\tvalue = nih_strdup (message, value_dbus);\n"
				   "\t\tif (! value) {\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tmessage = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
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
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestGetColourReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_get_colour_sync (const void *  parent,\n"
				   "                         NihDBusProxy *proxy,\n"
				   "                         char **       value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\tconst char *    local_dbus;\n"
				   "\tchar *          local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
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
				   "\t/* Iterate the method arguments, recursing into the variant */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&variter, &local_dbus);\n"
				   "\n"
				   "\t\tlocal = nih_strdup (parent, local_dbus);\n"
				   "\t\tif (! local) {\n"
				   "\t\t\t*value = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\t\t*value = local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_set_colour (NihDBusProxy *       proxy,\n"
				   "                    const char *         value,\n"
				   "                    MyTestSetColourReply handler,\n"
				   "                    NihDBusErrorHandler  error_handler,\n"
				   "                    void *               data,\n"
				   "                    int                  timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusMessageIter     variter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"s\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_set_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_colour_set_notify (DBusPendingCall *   pending_call,\n"
				   "                                            NihDBusPendingData *pending_data)\n"
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
				   "\t/* Create a message context for the reply, and check\n"
				   "\t * there are no arguments.\n"
				   "\t */\n"
				   "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyTestSetColourReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_set_colour_sync (const void *  parent,\n"
				   "                         NihDBusProxy *proxy,\n"
				   "                         const char *  value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"s\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\t/* Check the reply has no arguments */\n"
				   "\tdbus_message_unref (method_call);\n"
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
 				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_get_size (NihDBusProxy *      proxy,\n"
				   "                  MyTestGetSizeReply  handler,\n"
				   "                  NihDBusErrorHandler error_handler,\n"
				   "                  void *              data,\n"
				   "                  int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler != NULL) && (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"size\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_size_get_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_size_get_notify (DBusPendingCall *   pending_call,\n"
				   "                                          NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tuint32_t        value;\n"
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
				   "\t\t * over and recurse into the arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
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
				   "\t\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\t\t/* Demarshal a uint32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&variter, &value);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
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
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestGetSizeReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_get_size_sync (const void *  parent,\n"
				   "                       NihDBusProxy *proxy,\n"
				   "                       uint32_t *    value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\tuint32_t        local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"size\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
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
				   "\t/* Iterate the method arguments, recursing into the variant */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a uint32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&variter, &local);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\t\t*value = local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_set_touch (NihDBusProxy *      proxy,\n"
				   "                   int                 value,\n"
				   "                   MyTestSetTouchReply handler,\n"
				   "                   NihDBusErrorHandler error_handler,\n"
				   "                   void *              data,\n"
				   "                   int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusMessageIter     variter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"touch\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"b\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_touch_set_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_touch_set_notify (DBusPendingCall *   pending_call,\n"
				   "                                           NihDBusPendingData *pending_data)\n"
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
				   "\t/* Create a message context for the reply, and check\n"
				   "\t * there are no arguments.\n"
				   "\t */\n"
				   "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyTestSetTouchReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_set_touch_sync (const void *  parent,\n"
				   "                        NihDBusProxy *proxy,\n"
				   "                        int           value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"touch\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"b\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\t/* Check the reply has no arguments */\n"
				   "\tdbus_message_unref (method_call);\n"
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
 				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_foo_bing (NihDBusProxy *      proxy,\n"
				   "             MyFooBingReply      handler,\n"
				   "             NihDBusErrorHandler error_handler,\n"
				   "             void *              data,\n"
				   "             int                 timeout)\n"
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
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Foo\", \"Bing\");\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Foo_Bing_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Foo_Bing_notify (DBusPendingCall *   pending_call,\n"
				   "                                     NihDBusPendingData *pending_data)\n"
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
				   "\t\t((MyFooBingReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_foo_bing_sync (const void *  parent,\n"
				   "                  NihDBusProxy *proxy)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Foo\", \"Bing\");\n"
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


		/* Poke */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestPokeReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Poke_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestPokeReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* Peek */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestPeekReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Peek_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestPeekReply)");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* IsValidAddress */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestIsValidAddressReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_IsValidAddress_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestIsValidAddressReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
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


		/* colour (get) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour");
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
		TEST_EQ_STR (arg->type, "MyTestGetColourReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_get_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestGetColourReply)");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour_sync");
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
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* colour (set) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestSetColourReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_set_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestSetColourReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour_sync");
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
		TEST_EQ_STR (arg->name, "value");
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


		/* size (get) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size");
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
		TEST_EQ_STR (arg->type, "MyTestGetSizeReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_size_get_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestGetSizeReply)");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* touch (set) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestSetTouchReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_touch_set_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestSetTouchReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch_sync");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* Bing */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing");
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
		TEST_EQ_STR (arg->type, "MyFooBingReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Foo_Bing_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyFooBingReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing_sync");
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
		TEST_LIST_EMPTY (&typedefs);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can still generate all of the functions even if
	 * there are no prototypes in the object implementation.
	 */
	TEST_FEATURE ("with no properties");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&externs);

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


			interface = interface_new (node, "com.netsplit.Nih.Foo");
			interface->symbol = "foo";
			nih_list_add (&node->interfaces, &interface->entry);

			method = method_new (interface, "Bing");
			method->symbol = "bing";
			nih_list_add (&interface->methods, &method->entry);

			signal = signal_new (interface, "NewResult");
			signal->symbol = "new_result";
			nih_list_add (&interface->signals, &signal->entry);
		}

		str = node_proxy_functions (NULL, "my", node,
					    &prototypes, &typedefs, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("DBusPendingCall *\n"
				   "my_test_poke (NihDBusProxy *      proxy,\n"
				   "              uint32_t            address,\n"
				   "              const char *        value,\n"
				   "              MyTestPokeReply     handler,\n"
				   "              NihDBusErrorHandler error_handler,\n"
				   "              void *              data,\n"
				   "              int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Poke\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_Poke_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_Poke_notify (DBusPendingCall *   pending_call,\n"
				   "                                      NihDBusPendingData *pending_data)\n"
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
				   "\t\t((MyTestPokeReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_poke_sync (const void *  parent,\n"
				   "                   NihDBusProxy *proxy,\n"
				   "                   uint32_t      address,\n"
				   "                   const char *  value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Poke\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_peek (NihDBusProxy *      proxy,\n"
				   "              uint32_t            address,\n"
				   "              MyTestPeekReply     handler,\n"
				   "              NihDBusErrorHandler error_handler,\n"
				   "              void *              data,\n"
				   "              int                 timeout)\n"
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
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Peek\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_Peek_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_Peek_notify (DBusPendingCall *   pending_call,\n"
				   "                                      NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tchar *          value;\n"
				   "\tconst char *    value_dbus;\n"
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
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&iter, &value_dbus);\n"
				   "\n"
				   "\t\tvalue = nih_strdup (message, value_dbus);\n"
				   "\t\tif (! value) {\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tmessage = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
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
				   "\t\t((MyTestPeekReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_peek_sync (const void *  parent,\n"
				   "                   NihDBusProxy *proxy,\n"
				   "                   uint32_t      address,\n"
				   "                   char **       value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tchar *          value_local;\n"
				   "\tconst char *    value_local_dbus;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"Peek\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&iter, &value_local_dbus);\n"
				   "\n"
				   "\t\tvalue_local = nih_strdup (parent, value_local_dbus);\n"
				   "\t\tif (! value_local) {\n"
				   "\t\t\t*value = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t\t*value = value_local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (value_local);\n"
				   "\t\t*value = NULL;\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_is_valid_address (NihDBusProxy *            proxy,\n"
				   "                          uint32_t                  address,\n"
				   "                          MyTestIsValidAddressReply handler,\n"
				   "                          NihDBusErrorHandler       error_handler,\n"
				   "                          void *                    data,\n"
				   "                          int                       timeout)\n"
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
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"IsValidAddress\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_IsValidAddress_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_IsValidAddress_notify (DBusPendingCall *   pending_call,\n"
				   "                                                NihDBusPendingData *pending_data)\n"
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
				   "\t\t((MyTestIsValidAddressReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_is_valid_address_sync (const void *  parent,\n"
				   "                               NihDBusProxy *proxy,\n"
				   "                               uint32_t      address)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Test\", \"IsValidAddress\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\t/* Marshal a uint32_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &address)) {\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Bounce_signal (DBusConnection *    connection,\n"
				   "                                        DBusMessage *       signal,\n"
				   "                                        NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tuint32_t        height;\n"
				   "\tint32_t         velocity;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->name))\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tmessage = nih_dbus_message_new (NULL, connection, signal);\n"
				   "\tif (! message)\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t/* Iterate the arguments to the signal and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &height);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\t/* Demarshal a int32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &velocity);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestBounceHandler)proxied->handler) (proxied->data, message, height, velocity);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Test_Exploded_signal (DBusConnection *    connection,\n"
				   "                                          DBusMessage *       signal,\n"
				   "                                          NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->name))\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tmessage = nih_dbus_message_new (NULL, connection, signal);\n"
				   "\tif (! message)\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t/* Iterate the arguments to the signal and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestExplodedHandler)proxied->handler) (proxied->data, message);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_foo_bing (NihDBusProxy *      proxy,\n"
				   "             MyFooBingReply      handler,\n"
				   "             NihDBusErrorHandler error_handler,\n"
				   "             void *              data,\n"
				   "             int                 timeout)\n"
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
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Foo\", \"Bing\");\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Foo_Bing_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Foo_Bing_notify (DBusPendingCall *   pending_call,\n"
				   "                                     NihDBusPendingData *pending_data)\n"
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
				   "\t\t((MyFooBingReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_foo_bing_sync (const void *  parent,\n"
				   "                  NihDBusProxy *proxy)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"com.netsplit.Nih.Foo\", \"Bing\");\n"
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
				   "}\n"
				   "\n"
				   "\n"
				   "static DBusHandlerResult\n"
				   "my_com_netsplit_Nih_Foo_NewResult_signal (DBusConnection *    connection,\n"
				   "                                          DBusMessage *       signal,\n"
				   "                                          NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->name))\n"
				   "\t\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tmessage = nih_dbus_message_new (NULL, connection, signal);\n"
				   "\tif (! message)\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\n"
				   "\t/* Iterate the arguments to the signal and demarshal into arguments\n"
				   "\t * for our own function call.\n"
				   "\t */\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tnih_error_push_context ();\n"
				   "\t((MyFooNewResultHandler)proxied->handler) (proxied->data, message);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"));


		/* Poke */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestPokeReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Poke_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestPokeReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_poke_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* Peek */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestPeekReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Peek_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestPeekReply)");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_peek_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* IsValidAddress */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestIsValidAddressReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_IsValidAddress_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestIsValidAddressReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_is_valid_address_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "address");
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


		/* Bounce */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Bounce_signal");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "signal");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxySignal *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxied");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestBounceHandler)");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "height");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "int32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "velocity");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		/* Exploded */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Exploded_signal");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "signal");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxySignal *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxied");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestExplodedHandler)");
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


		/* Bing */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing");
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
		TEST_EQ_STR (arg->type, "MyFooBingReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Foo_Bing_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyFooBingReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_foo_bing_sync");
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


		/* NewResult */
		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Foo_NewResult_signal");
		TEST_ALLOC_PARENT (func->name, func);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusConnection *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "connection");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "DBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "signal");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "NihDBusProxySignal *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxied");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyFooNewResultHandler)");
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


		TEST_LIST_EMPTY (&prototypes);
		TEST_LIST_EMPTY (&typedefs);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that we can still generate all of the functions even if
	 * there are no methods or signals in the object implementation.
	 */
	TEST_FEATURE ("with no methods or signals");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&externs);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

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
		}

		str = node_proxy_functions (NULL, "my", node,
					    &prototypes, &typedefs, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, ("DBusPendingCall *\n"
				   "my_test_get_colour (NihDBusProxy *       proxy,\n"
				   "                    MyTestGetColourReply handler,\n"
				   "                    NihDBusErrorHandler  error_handler,\n"
				   "                    void *               data,\n"
				   "                    int                  timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler != NULL) && (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_get_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_colour_get_notify (DBusPendingCall *   pending_call,\n"
				   "                                            NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tconst char *    value_dbus;\n"
				   "\tchar *          value;\n"
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
				   "\t\t * over and recurse into the arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
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
				   "\t\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&variter, &value_dbus);\n"
				   "\n"
				   "\t\tvalue = nih_strdup (message, value_dbus);\n"
				   "\t\tif (! value) {\n"
				   "\t\t\tnih_free (message);\n"
				   "\t\t\tmessage = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
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
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestGetColourReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_get_colour_sync (const void *  parent,\n"
				   "                         NihDBusProxy *proxy,\n"
				   "                         char **       value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\tconst char *    local_dbus;\n"
				   "\tchar *          local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
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
				   "\t/* Iterate the method arguments, recursing into the variant */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&variter, &local_dbus);\n"
				   "\n"
				   "\t\tlocal = nih_strdup (parent, local_dbus);\n"
				   "\t\tif (! local) {\n"
				   "\t\t\t*value = NULL;\n"
				   "\t\t\tgoto enomem;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\t\t*value = local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_set_colour (NihDBusProxy *       proxy,\n"
				   "                    const char *         value,\n"
				   "                    MyTestSetColourReply handler,\n"
				   "                    NihDBusErrorHandler  error_handler,\n"
				   "                    void *               data,\n"
				   "                    int                  timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusMessageIter     variter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"s\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_set_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_colour_set_notify (DBusPendingCall *   pending_call,\n"
				   "                                            NihDBusPendingData *pending_data)\n"
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
				   "\t/* Create a message context for the reply, and check\n"
				   "\t * there are no arguments.\n"
				   "\t */\n"
				   "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyTestSetColourReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_set_colour_sync (const void *  parent,\n"
				   "                         NihDBusProxy *proxy,\n"
				   "                         const char *  value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"colour\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"s\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\t/* Check the reply has no arguments */\n"
				   "\tdbus_message_unref (method_call);\n"
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
 				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_get_size (NihDBusProxy *      proxy,\n"
				   "                  MyTestGetSizeReply  handler,\n"
				   "                  NihDBusErrorHandler error_handler,\n"
				   "                  void *              data,\n"
				   "                  int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler != NULL) && (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"size\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_size_get_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_size_get_notify (DBusPendingCall *   pending_call,\n"
				   "                                          NihDBusPendingData *pending_data)\n"
				   "{\n"
				   "\tDBusMessage *   reply;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tDBusError       error;\n"
				   "\tuint32_t        value;\n"
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
				   "\t\t * over and recurse into the arguments.\n"
				   "\t\t */\n"
				   "\t\tmessage = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
				   "\t\tif (! message)\n"
				   "\t\t\tgoto enomem;\n"
				   "\n"
				   "\t\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
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
				   "\t\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\t\t/* Demarshal a uint32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {\n"
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
				   "\t\tdbus_message_iter_get_basic (&variter, &value);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
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
				   "\tnih_error_push_context ();\n"
				   "\t((MyTestGetSizeReply)pending_data->handler) (pending_data->data, message, value);\n"
				   "\tnih_error_pop_context ();\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_get_size_sync (const void *  parent,\n"
				   "                       NihDBusProxy *proxy,\n"
				   "                       uint32_t *    value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\tuint32_t        local;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert (value != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Get\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"size\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
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
				   "\t/* Iterate the method arguments, recursing into the variant */\n"
				   "\tdbus_message_iter_init (reply, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&iter, &variter);\n"
				   "\n"
				   "\tdbus_message_iter_next (&iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t}\n"
				   "\n"
				   "\tdo {\n"
				   "\t\t__label__ enomem;\n"
				   "\n"
				   "\t\t/* Demarshal a uint32_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {\n"
				   "\t\t\tdbus_message_unref (reply);\n"
				   "\t\t\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&variter, &local);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&variter);\n"
				   "\n"
				   "\t\t*value = local;\n"
				   "\tenomem: __attribute__ ((unused));\n"
				   "\t} while (! *value);\n"
				   "\n"
				   "\tdbus_message_unref (reply);\n"
				   "\n"
				   "\treturn 0;\n"
				   "}\n"
				   "\n"
				   "\n"
				   "DBusPendingCall *\n"
				   "my_test_set_touch (NihDBusProxy *      proxy,\n"
				   "                   int                 value,\n"
				   "                   MyTestSetTouchReply handler,\n"
				   "                   NihDBusErrorHandler error_handler,\n"
				   "                   void *              data,\n"
				   "                   int                 timeout)\n"
				   "{\n"
				   "\tDBusMessage *       method_call;\n"
				   "\tDBusMessageIter     iter;\n"
				   "\tDBusMessageIter     variter;\n"
				   "\tDBusPendingCall *   pending_call;\n"
				   "\tNihDBusPendingData *pending_data;\n"
				   "\tconst char *        interface;\n"
				   "\tconst char *        property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\tnih_assert ((handler == NULL) || (error_handler != NULL));\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"touch\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"b\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (NULL);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\tNIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_touch_set_notify,\n"
				   "\t                                        pending_data, (DBusFreeFunction)nih_discard));\n"
				   "\n"
				   "\treturn pending_call;\n"
				   "}\n"
				   "\n"
				   "static void\n"
				   "my_com_netsplit_Nih_Test_touch_set_notify (DBusPendingCall *   pending_call,\n"
				   "                                           NihDBusPendingData *pending_data)\n"
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
				   "\t/* Create a message context for the reply, and check\n"
				   "\t * there are no arguments.\n"
				   "\t */\n"
				   "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				   "\tdbus_message_iter_init (message->message, &iter);\n"
				   "\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				   "\t\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				   "\t\tpending_data->error_handler (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\n"
				   "\t\tnih_free (message);\n"
				   "\t\tdbus_message_unref (reply);\n"
				   "\t\treturn;\n"
				   "\t}\n"
				   "\n"
				   "\t/* Call the handler function */\n"
				   "\tif (pending_data->handler) {\n"
				   "\t\tnih_error_push_context ();\n"
				   "\t\t((MyTestSetTouchReply)pending_data->handler) (pending_data->data, message);\n"
				   "\t\tnih_error_pop_context ();\n"
				   "\t}\n"
				   "\n"
				   "\tnih_free (message);\n"
				   "\tdbus_message_unref (reply);\n"
				   "}\n"
				   "\n"
				   "int\n"
				   "my_test_set_touch_sync (const void *  parent,\n"
				   "                        NihDBusProxy *proxy,\n"
				   "                        int           value)\n"
				   "{\n"
				   "\tDBusMessage *   method_call;\n"
				   "\tDBusMessageIter iter;\n"
				   "\tDBusMessageIter variter;\n"
				   "\tDBusError       error;\n"
				   "\tDBusMessage *   reply;\n"
				   "\tconst char *    interface;\n"
				   "\tconst char *    property;\n"
				   "\n"
				   "\tnih_assert (proxy != NULL);\n"
				   "\n"
				   "\t/* Construct the method call message. */\n"
				   "\tmethod_call = dbus_message_new_method_call (proxy->name, proxy->path, \"org.freedesktop.DBus.Properties\", \"Set\");\n"
				   "\tif (! method_call)\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\n"
				   "\tdbus_message_iter_init_append (method_call, &iter);\n"
				   "\n"
				   "\tinterface = \"com.netsplit.Nih.Test\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tproperty = \"touch\";\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"b\", &variter)) {\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\t/* Marshal a int onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {\n"
				   "\t\tdbus_message_iter_close_container (&iter, &variter);\n"
				   "\t\tdbus_message_unref (method_call);\n"
				   "\t\tnih_return_no_memory_error (-1);\n"
				   "\t}\n"
				   "\n"
				   "\tif (! dbus_message_iter_close_container (&iter, &variter)) {\n"
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
				   "\t/* Check the reply has no arguments */\n"
				   "\tdbus_message_unref (method_call);\n"
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


		/* colour (get) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour");
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
		TEST_EQ_STR (arg->type, "MyTestGetColourReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_get_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestGetColourReply)");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_colour_sync");
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
		TEST_EQ_STR (arg->type, "char **");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* colour (set) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour");
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
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestSetColourReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_colour_set_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestSetColourReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_colour_sync");
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
		TEST_EQ_STR (arg->name, "value");
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


		/* size (get) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size");
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
		TEST_EQ_STR (arg->type, "MyTestGetSizeReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_size_get_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestGetSizeReply)");
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
		TEST_EQ_STR (arg->type, "uint32_t");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_get_size_sync");
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
		TEST_EQ_STR (arg->type, "uint32_t *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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


		/* touch (set) */
		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);
		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MyTestSetTouchReply");
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


		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "static void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_touch_set_notify");
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


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MyTestSetTouchReply)");
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


		TEST_LIST_NOT_EMPTY (&externs);

		func = (TypeFunc *)externs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_test_set_touch_sync");
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
		TEST_EQ_STR (arg->type, "int");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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
		TEST_LIST_EMPTY (&typedefs);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that when there are no interface members, an empty string
	 * is returned.
	 */
	TEST_FEATURE ("with no members");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&externs);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);

			interface = interface_new (node, "com.netsplit.Nih.Test");
			interface->symbol = "test";
			nih_list_add (&node->interfaces, &interface->entry);

			interface = interface_new (node, "com.netsplit.Nih.Foo");
			interface->symbol = "foo";
			nih_list_add (&node->interfaces, &interface->entry);
		}

		str = node_proxy_functions (NULL, "my", node,
					    &prototypes, &typedefs, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, "");

		TEST_LIST_EMPTY (&prototypes);
		TEST_LIST_EMPTY (&typedefs);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}


	/* Check that when there are no interfaces, an empty string
	 * is returned.
	 */
	TEST_FEATURE ("with no interfaces");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&externs);

		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);
		}

		str = node_proxy_functions (NULL, "my", node,
					    &prototypes, &typedefs, &externs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&externs);

			nih_free (node);
			continue;
		}

		TEST_EQ_STR (str, "");

		TEST_LIST_EMPTY (&prototypes);
		TEST_LIST_EMPTY (&typedefs);
		TEST_LIST_EMPTY (&externs);

		nih_free (str);
		nih_free (node);
	}
}


int
main (int   argc,
      char *argv[])
{
	program_name = "test";
	nih_error_init ();

	test_path_valid ();
	test_new ();
	test_start_tag ();
	test_end_tag ();
	test_lookup_interface ();

	test_interfaces_array ();
	test_object_functions ();
	test_proxy_functions ();

	return 0;
}
