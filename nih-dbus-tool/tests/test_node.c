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

	return 0;
}
