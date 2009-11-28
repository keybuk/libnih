/* nih-dbus-tool
 *
 * test_parse.c - test suite for nih-dbus-tool/parse.c
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

#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/main.h>
#include <nih/error.h>

#include <nih-dbus/dbus_object.h>

#include "node.h"
#include "interface.h"
#include "method.h"
#include "signal.h"
#include "property.h"
#include "argument.h"
#include "parse.h"
#include "errors.h"


void
test_stack_push (void)
{
	NihList     stack;
	ParseStack *entry;
	ParseStack  base;
	Node *      node = NULL;
	Interface * interface = NULL;
	Method *    method = NULL;
	Signal *    signal = NULL;
	Property *  property = NULL;
	Argument *  argument = NULL;

	TEST_FUNCTION ("parse_stack_push");


	/* Check that we can push a Node object onto the stack; a new
	 * stack entry should be allocated and returned with the Node
	 * stored in its data union, and a reference should be added
	 * as well.
	 */
	TEST_FEATURE ("with node");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			nih_list_init (&stack);
			nih_list_init (&base.entry);
			nih_list_add (&stack, &base.entry);

			node = node_new (NULL, "/com/netsplit/Nih/Test");
		}

		entry = parse_stack_push (NULL, &stack, PARSE_NODE, node);

		if (test_alloc_failed) {
			TEST_EQ_P (entry, NULL);
			TEST_EQ_P (stack.next, &base.entry);

			nih_free (node);
			continue;
		}

		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_NODE);
		TEST_EQ_P (entry->node, node);
		TEST_ALLOC_PARENT (node, entry);

		TEST_LIST_NOT_EMPTY (&stack);
		TEST_EQ_P (stack.next, &entry->entry);

		nih_free (entry);
		nih_free (node);
	}


	/* Check that we can push an Interface object onto the stack; a new
	 * stack entry should be allocated and returned with the Interface
	 * stored in its data union, and a reference should be added
	 * as well.
	 */
	TEST_FEATURE ("with interface");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			nih_list_init (&stack);
			nih_list_init (&base.entry);
			nih_list_add (&stack, &base.entry);

			interface = interface_new (NULL, "com.netsplit.Nih.Test");
		}

		entry = parse_stack_push (NULL, &stack,
					  PARSE_INTERFACE, interface);

		if (test_alloc_failed) {
			TEST_EQ_P (entry, NULL);
			TEST_EQ_P (stack.next, &base.entry);

			nih_free (interface);
			continue;
		}

		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_INTERFACE);
		TEST_EQ_P (entry->interface, interface);
		TEST_ALLOC_PARENT (interface, entry);

		TEST_LIST_NOT_EMPTY (&stack);
		TEST_EQ_P (stack.next, &entry->entry);

		nih_free (entry);
		nih_free (interface);
	}


	/* Check that we can push a Method object onto the stack; a new
	 * stack entry should be allocated and returned with the Method
	 * stored in its data union, and a reference should be added
	 * as well.
	 */
	TEST_FEATURE ("with method");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			nih_list_init (&stack);
			nih_list_init (&base.entry);
			nih_list_add (&stack, &base.entry);

			method = method_new (NULL, "TestMethod");
		}

		entry = parse_stack_push (NULL, &stack, PARSE_METHOD, method);

		if (test_alloc_failed) {
			TEST_EQ_P (entry, NULL);
			TEST_EQ_P (stack.next, &base.entry);

			nih_free (method);
			continue;
		}

		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_METHOD);
		TEST_EQ_P (entry->method, method);
		TEST_ALLOC_PARENT (method, entry);

		TEST_LIST_NOT_EMPTY (&stack);
		TEST_EQ_P (stack.next, &entry->entry);

		nih_free (entry);
		nih_free (method);
	}


	/* Check that we can push a Signal object onto the stack; a new
	 * stack entry should be allocated and returned with the Signal
	 * stored in its data union, and a reference should be added
	 * as well.
	 */
	TEST_FEATURE ("with signal");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			nih_list_init (&stack);
			nih_list_init (&base.entry);
			nih_list_add (&stack, &base.entry);

			signal = signal_new (NULL, "TestSignal");
		}

		entry = parse_stack_push (NULL, &stack, PARSE_SIGNAL, signal);

		if (test_alloc_failed) {
			TEST_EQ_P (entry, NULL);
			TEST_EQ_P (stack.next, &base.entry);

			nih_free (signal);
			continue;
		}

		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_SIGNAL);
		TEST_EQ_P (entry->signal, signal);
		TEST_ALLOC_PARENT (signal, entry);

		TEST_LIST_NOT_EMPTY (&stack);
		TEST_EQ_P (stack.next, &entry->entry);

		nih_free (entry);
		nih_free (signal);
	}


	/* Check that we can push a Property object onto the stack; a new
	 * stack entry should be allocated and returned with the Property
	 * stored in its data union, and a reference should be added
	 * as well.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			nih_list_init (&stack);
			nih_list_init (&base.entry);
			nih_list_add (&stack, &base.entry);

			property = property_new (NULL, "TestProperty",
						 "s", NIH_DBUS_READ);
		}

		entry = parse_stack_push (NULL, &stack,
					  PARSE_PROPERTY, property);

		if (test_alloc_failed) {
			TEST_EQ_P (entry, NULL);
			TEST_EQ_P (stack.next, &base.entry);

			nih_free (property);
			continue;
		}

		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_PROPERTY);
		TEST_EQ_P (entry->property, property);
		TEST_ALLOC_PARENT (property, entry);

		TEST_LIST_NOT_EMPTY (&stack);
		TEST_EQ_P (stack.next, &entry->entry);

		nih_free (entry);
		nih_free (property);
	}


	/* Check that we can push an Argument object onto the stack; a new
	 * stack entry should be allocated and returned with the Argument
	 * stored in its data union, and a reference should be added
	 * as well.
	 */
	TEST_FEATURE ("with argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			nih_list_init (&stack);
			nih_list_init (&base.entry);
			nih_list_add (&stack, &base.entry);

			argument = argument_new (NULL, "test_arg",
						 "i", NIH_DBUS_ARG_IN);
		}

		entry = parse_stack_push (NULL, &stack,
					  PARSE_ARGUMENT, argument);

		if (test_alloc_failed) {
			TEST_EQ_P (entry, NULL);
			TEST_EQ_P (stack.next, &base.entry);

			nih_free (argument);
			continue;
		}

		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ARGUMENT);
		TEST_EQ_P (entry->argument, argument);
		TEST_ALLOC_PARENT (argument, entry);

		TEST_LIST_NOT_EMPTY (&stack);
		TEST_EQ_P (stack.next, &entry->entry);

		nih_free (entry);
		nih_free (argument);
	}


	/* Check that we can push an Annotation reference onto the stack,
	 * which has a NULL pointer for data.  A new stack entry should be
	 * allocated and returned with the right type but NULL data.
	 */
	TEST_FEATURE ("with annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			nih_list_init (&stack);
			nih_list_init (&base.entry);
			nih_list_add (&stack, &base.entry);
		}

		entry = parse_stack_push (NULL, &stack,
					  PARSE_ANNOTATION, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (entry, NULL);
			TEST_EQ_P (stack.next, &base.entry);
			continue;
		}

		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ANNOTATION);
		TEST_EQ_P (entry->data, NULL);

		TEST_LIST_NOT_EMPTY (&stack);
		TEST_EQ_P (stack.next, &entry->entry);

		nih_free (entry);
	}


	/* Check that we can push an ignored reference onto the stack,
	 * which has a NULL pointer for data.  A new stack entry should be
	 * allocated and returned with the right type but NULL data.
	 */
	TEST_FEATURE ("with ignored entity");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			nih_list_init (&stack);
			nih_list_init (&base.entry);
			nih_list_add (&stack, &base.entry);
		}

		entry = parse_stack_push (NULL, &stack,
					  PARSE_IGNORED, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (entry, NULL);
			TEST_EQ_P (stack.next, &base.entry);
			continue;
		}

		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_IGNORED);
		TEST_EQ_P (entry->data, NULL);

		TEST_LIST_NOT_EMPTY (&stack);
		TEST_EQ_P (stack.next, &entry->entry);

		nih_free (entry);
	}
}

void
test_stack_top (void)
{
	NihList     stack;
	ParseStack  entry1;
	ParseStack  entry2;
	ParseStack *ret;

	TEST_FUNCTION ("parse_stack_top");


	/* Check that the first item in the stack is returned when there
	 * are multiple items in the stack.
	 */
	TEST_FEATURE ("with multiple items");
	TEST_ALLOC_FAIL {
		nih_list_init (&stack);
		nih_list_init (&entry1.entry);
		nih_list_add_after (&stack, &entry1.entry);
		nih_list_init (&entry2.entry);
		nih_list_add_after (&stack, &entry2.entry);

		ret = parse_stack_top (&stack);

		TEST_EQ_P (ret, &entry2);
	}


	/* Check that when there is only one item in the stack, that item
	 * is returned.
	 */
	TEST_FEATURE ("with multiple items");
	TEST_ALLOC_FAIL {
		nih_list_init (&stack);
		nih_list_init (&entry1.entry);
		nih_list_add_after (&stack, &entry1.entry);

		ret = parse_stack_top (&stack);

		TEST_EQ_P (ret, &entry1);
	}


	/* Check that when the stack is empty, NULL is returned. */
	TEST_FEATURE ("with empty stack");
	TEST_ALLOC_FAIL {
		nih_list_init (&stack);

		ret = parse_stack_top (&stack);

		TEST_EQ_P (ret, NULL);
	}
}


void
test_start_tag (void)
{
	XML_ParsingStatus status;
	ParseContext      context;
	ParseStack *      parent = NULL;
	ParseStack *      entry;
	XML_Parser        xmlp;
	Node *            node = NULL;
	Interface *       interface = NULL;
	Method *          method = NULL;
	Signal *          signal;
	Property *        property;
	Argument *        argument;
	char *            attr[7];
	NihError *        err;
	FILE *            output;

	TEST_FUNCTION ("parse_start_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));

	output = tmpfile ();


	/* Check that a node tag is handled by calling node_start_tag()
	 * and all of the expected side-effects occur.  In case of error,
	 * the parser should be stopped and an error raised.
	 */
	TEST_FEATURE ("with node");
	TEST_ALLOC_FAIL {
		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		attr[0] = "name";
		attr[1] = "/com/netsplit/Nih/Test";
		attr[2] = NULL;

		parse_start_tag (xmlp, "node", attr);

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

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


	/* Check that a interface tag is handled by calling interface_start_tag()
	 * and all of the expected side-effects occur.  In case of error,
	 * the parser should be stopped and an error raised.
	 */
	TEST_FEATURE ("with interface");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);
			nih_discard (node);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		attr[0] = "name";
		attr[1] = "com.netsplit.Nih.Test";
		attr[2] = NULL;

		parse_start_tag (xmlp, "interface", attr);

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_INTERFACE);

		interface = entry->interface;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, entry);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a method tag is handled by calling method_start_tag()
	 * and all of the expected side-effects occur.  In case of error,
	 * the parser should be stopped and an error raised.
	 */
	TEST_FEATURE ("with method");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		attr[0] = "name";
		attr[1] = "TestMethod";
		attr[2] = NULL;

		parse_start_tag (xmlp, "method", attr);

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_METHOD);

		method = entry->method;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, entry);
		TEST_EQ_STR (method->name, "TestMethod");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_LIST_EMPTY (&method->arguments);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a signal tag is handled by calling signal_start_tag()
	 * and all of the expected side-effects occur.  In case of error,
	 * the parser should be stopped and an error raised.
	 */
	TEST_FEATURE ("with signal");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		attr[0] = "name";
		attr[1] = "TestSignal";
		attr[2] = NULL;

		parse_start_tag (xmlp, "signal", attr);

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_SIGNAL);

		signal = entry->signal;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, entry);
		TEST_EQ_STR (signal->name, "TestSignal");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a property tag is handled by calling property_start_tag()
	 * and all of the expected side-effects occur.  In case of error,
	 * the parser should be stopped and an error raised.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		attr[0] = "name";
		attr[1] = "TestProperty";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = "access";
		attr[5] = "read";
		attr[6] = NULL;

		parse_start_tag (xmlp, "property", attr);

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_PROPERTY);

		property = entry->property;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, entry);
		TEST_EQ_STR (property->name, "TestProperty");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a argument tag is handled by calling argument_start_tag()
	 * and all of the expected side-effects occur.  In case of error,
	 * the parser should be stopped and an error raised.
	 */
	TEST_FEATURE ("with argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		attr[0] = "name";
		attr[1] = "test_arg";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = NULL;

		parse_start_tag (xmlp, "arg", attr);

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ARGUMENT);

		argument = entry->argument;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, entry);
		TEST_EQ_STR (argument->name, "test_arg");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a annotation tag is handled by calling
	 * annotation_start_tag() and all of the expected side-effects
	 * occur.  In case of error, the parser should be stopped and
	 * an error raised.
	 */
	TEST_FEATURE ("with annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		attr[0] = "name";
		attr[1] = "org.freedesktop.DBus.Deprecated";
		attr[2] = "value";
		attr[3] = "true";
		attr[4] = NULL;

		parse_start_tag (xmlp, "annotation", attr);

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_ANNOTATION);
		TEST_EQ_P (entry->data, NULL);

		TEST_TRUE (method->deprecated);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that when an ignored tag is on the stack, another ignored
	 * tag is pushed with no other side-effects.
	 */
	TEST_FEATURE ("with ignored tag on stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_IGNORED, NULL);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		attr[0] = "name";
		attr[1] = "TestMethod";
		attr[2] = NULL;

		parse_start_tag (xmlp, "method", attr);

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_IGNORED);
		TEST_EQ_P (entry->data, NULL);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that the function has no effect when parsing is finished
	 * (ie. when an error occurs or EOF has been reached).
	 */
	TEST_FEATURE ("with finished parser");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);
		assert (XML_StopParser (xmlp, FALSE) == XML_STATUS_OK);

		attr[0] = "name";
		attr[1] = "TestMethod";
		attr[2] = NULL;

		parse_start_tag (xmlp, "method", attr);

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_FINISHED);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		nih_free (parent);
	}


	/* Check that an error while handling a tag stops the parser so that
	 * no further parsing is performed.
	 */
	TEST_FEATURE ("with error while handling");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		attr[0] = "name";
		attr[1] = "Test Method";
		attr[2] = NULL;

		parse_start_tag (xmlp, "method", attr);

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_FINISHED);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		err = nih_error_get ();
		TEST_EQ (err->number, METHOD_INVALID_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that an unknown tag results in a warning being printed
	 * to standard error and an ignored tag being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with unknown tag");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			assert (XML_ParserReset (xmlp, "UTF-8"));
			XML_SetUserData (xmlp, &context);

			XML_Parse (xmlp, "", 0, 0);

			attr[0] = "name";
			attr[1] = "TestWidget";
			attr[2] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			parse_start_tag (xmlp, "widget", attr);
		}
		rewind (output);

		XML_GetParsingStatus (xmlp, &status);

		if (test_alloc_failed
		    && (status.parsing == XML_FINISHED)) {
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_EQ_P (parse_stack_top (&context.stack), parent);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_FILE_RESET (output);

			nih_free (parent);
			continue;
		}

		TEST_EQ (status.parsing, XML_PARSING);

		entry = parse_stack_top (&context.stack);
		TEST_NE_P (entry, NULL);
		TEST_NE_P (entry, parent);
		TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
		TEST_EQ (entry->type, PARSE_IGNORED);
		TEST_EQ_P (entry->data, NULL);

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unknown tag: widget\n");
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
	XML_ParsingStatus status;
	ParseContext      context;
	ParseStack *      parent = NULL;
	ParseStack *      entry = NULL;
	XML_Parser        xmlp;
	Node *            node = NULL;
	Interface *       interface = NULL;
	Method *          method = NULL;
	Signal *          signal = NULL;
	Property *        property = NULL;
	Argument *        argument = NULL;
	NihError *        err;

	TEST_FUNCTION ("node_end_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));


	/* Check that a node end tag is handled by calling node_end_tag()
	 * and all of the expected side-effects occur.  In case of error,
	 * the parser should be stopped and an error raised.
	 */
	TEST_FEATURE ("with node");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_NODE, node);
			nih_discard (node);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		TEST_FREE_TAG (entry);

		parse_end_tag (xmlp, "node");

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_NOT_FREE (entry);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (node, NULL);
		TEST_EQ_P (context.node, node);

		nih_free (node);
		context.node = NULL;
	}


	/* Check that an interface end tag is handled by calling
	 * interface_end_tag() and all of the expected side-effects
	 * occur.  In case of error the parser should be stopped and
	 * an error raised.
	 */
	TEST_FEATURE ("with interface");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, "/com/netsplit/Nih/Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);
			nih_discard (node);

			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		TEST_FREE_TAG (entry);

		parse_end_tag (xmlp, "interface");

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&node->interfaces);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (interface, node);

		TEST_LIST_NOT_EMPTY (&node->interfaces);
		TEST_EQ_P (node->interfaces.next, &interface->entry);

		nih_free (parent);
	}


	/* Check that a method end tag is handled by calling
	 * method_end_tag() and all of the expected side-effects
	 * occur.  In case of error the parser should be stopped and
	 * an error raised.
	 */
	TEST_FEATURE ("with method");
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

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		TEST_FREE_TAG (entry);

		parse_end_tag (xmlp, "method");

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->methods);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (method, interface);

		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_EQ_P (interface->methods.next, &method->entry);

		nih_free (parent);
	}


	/* Check that a signal end tag is handled by calling
	 * signal_end_tag() and all of the expected side-effects
	 * occur.  In case of error the parser should be stopped and
	 * an error raised.
	 */
	TEST_FEATURE ("with signal");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			signal = signal_new (NULL, "TestSignal");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_SIGNAL, signal);
			nih_discard (signal);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		TEST_FREE_TAG (entry);

		parse_end_tag (xmlp, "signal");

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->signals);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (signal, interface);

		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_EQ_P (interface->signals.next, &signal->entry);

		nih_free (parent);
	}


	/* Check that a property end tag is handled by calling
	 * property_end_tag() and all of the expected side-effects
	 * occur.  In case of error the parser should be stopped and
	 * an error raised.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			property = property_new (NULL, "TestProperty",
						 "s", NIH_DBUS_READ);
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_PROPERTY, property);
			nih_discard (property);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		TEST_FREE_TAG (entry);

		parse_end_tag (xmlp, "property");

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->properties);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (property, interface);

		TEST_LIST_NOT_EMPTY (&interface->properties);
		TEST_EQ_P (interface->properties.next, &property->entry);

		nih_free (parent);
	}


	/* Check that an argument end tag is handled by calling
	 * property_end_tag() and all of the expected side-effects
	 * occur.  In case of error the parser should be stopped and
	 * an error raised.
	 */
	TEST_FEATURE ("with argument");
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

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		TEST_FREE_TAG (entry);

		parse_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (argument, method);

		TEST_LIST_NOT_EMPTY (&method->arguments);
		TEST_EQ_P (method->arguments.next, &argument->entry);

		nih_free (parent);
	}


	/* Check that an annotation end tag is handled by calling
	 * property_end_tag() and all of the expected side-effects
	 * occur.  In case of error the parser should be stopped and
	 * an error raised.
	 */
	TEST_FEATURE ("with annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ANNOTATION, NULL);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		TEST_FREE_TAG (entry);

		parse_end_tag (xmlp, "annotation");

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_NOT_FREE (entry);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		TEST_FREE (entry);

		nih_free (parent);
	}


	/* Check that an ignored tag is freed and no other handling
	 * takes place.
	 */
	TEST_FEATURE ("with ignored tag");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_IGNORED, NULL);
		}

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);

		TEST_FREE_TAG (entry);

		parse_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_PARSING);

		TEST_FREE (entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_free (parent);
	}


	/* Check that no handling takes place when parsing is finished,
	 * and even the stack is not freed (so as to provide context to
	 * the error, and because the error may cause a stack imbalance).
	 */
	TEST_FEATURE ("with finished parser");
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

		assert (XML_ParserReset (xmlp, "UTF-8"));
		XML_SetUserData (xmlp, &context);

		XML_Parse (xmlp, "", 0, 0);
		assert (XML_StopParser (xmlp, FALSE) == XML_STATUS_OK);

		TEST_FREE_TAG (entry);

		parse_end_tag (xmlp, "arg");

		if (test_alloc_failed) {
			XML_GetParsingStatus (xmlp, &status);
			TEST_EQ (status.parsing, XML_FINISHED);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&method->arguments);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		XML_GetParsingStatus (xmlp, &status);
		TEST_EQ (status.parsing, XML_FINISHED);

		TEST_NOT_FREE (entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_free (entry);
		nih_free (parent);
	}


	XML_ParserFree (xmlp);
}


void
test_parse_xml (void)
{
	FILE *     fp;
	FILE *     output;
	Node *     node = NULL;
	Interface *interface;
	Method *   method;
	Signal *   signal;
	Property * property;
	Argument * argument;

	TEST_FUNCTION ("parse_xml");
	fp = tmpfile ();
	output = tmpfile ();

	/* Check that a file containing a single node entity is parsed
	 * successfully, returning a Node structure with no information
	 * attached.
	 */
	TEST_FEATURE ("with empty node");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node/>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that the node entity may have a name attribute, which is
	 * stored into the Node structure's name member.
	 */
	TEST_FEATURE ("with named node");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node name=\"/com/netsplit/Nih/Test\"/>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_STR (node->path, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (node->path, node);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a node may define an interface, which appears in
	 * its interfaces list as an Interface structure with the name
	 * filled in.
	 */
	TEST_FEATURE ("with single empty interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\"/>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may be marked deprecated using an
	 * annotation tag inside it.
	 */
	TEST_FEATURE ("with deprecated interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                value=\"true\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_TRUE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may be explicitly marked as not deprecated
	 * using an annotation tag inside it.
	 */
	TEST_FEATURE ("with explicitly non-deprecated interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                value=\"false\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may have an alternate C symbol fragment
	 * given by using an annotation tag inside it.
	 */
	TEST_FEATURE ("with alternative symbol for interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                value=\"ITest\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "ITest");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that when multiple alternative symbols are given for an
	 * interface, the last is used.
	 */
	TEST_FEATURE ("with multiple alternative symbols for interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                value=\"itest\"/>\n");
		fprintf (fp, "    <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                value=\"ITest\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "ITest");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may define a method, which appears in
	 * its methods list as a Method structure with the name filled
	 * in.
	 */
	TEST_FEATURE ("with argument-less method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may be marked as deprecated using an
	 * annotation tag within it.
	 */
	TEST_FEATURE ("with deprecated method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_TRUE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may be marked as explicitly not deprecated
	 * using an annotation tag within it.
	 */
	TEST_FEATURE ("with explicitly non-deprecated method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"false\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may have an alternate C symbol fragment
	 * supplied using an annotation tag within it.
	 */
	TEST_FEATURE ("with alternate symbol for method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"wib\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wib");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that when multiple alternative symbols are given for a
	 * method, the last is used.
	 */
	TEST_FEATURE ("with multiple alternative symbols for method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"wob\"/>\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"wib\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wib");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may be marked to not expect a reply using an
	 * annotation tag within it.
	 */
	TEST_FEATURE ("with no reply method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Method.NoReply\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_TRUE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may be marked as explicitly to expect a reply
	 * using an annotation tag within it.
	 */
	TEST_FEATURE ("with explicitly replying method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Method.NoReply\"\n");
		fprintf (fp, "                  value=\"false\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may be marked as asynchronous in implementation
	 * using an annotation tag within it.
	 */
	TEST_FEATURE ("with asynchronous method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Method.Async\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_TRUE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may be marked as explicitly synchronous in
	 * implementation using an annotation tag within it.
	 */
	TEST_FEATURE ("with explicitly synchronous method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Method.Async\"\n");
		fprintf (fp, "                  value=\"false\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may define an argument, which appears in its
	 * arguments list as an Argument structure.  For methods, the
	 * argument direction should default to in.
	 */
	TEST_FEATURE ("with argument to method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an argument may have its direction explicitly defined
	 * as "in".
	 */
	TEST_FEATURE ("with input argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an argument may have its argument explicitly defined
	 * as "out".
	 */
	TEST_FEATURE ("with output argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that the name attribute of an argument may be optional
	 * and that NULL is stored instead.
	 */
	TEST_FEATURE ("with unnamed argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg type=\"s\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->symbol, "arg1");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that multiple arguments may have no name attribute, and
	 * that a symbol is generated for each one.
	 */
	TEST_FEATURE ("with multiple unnamed arguments");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg type=\"s\"/>\n");
		fprintf (fp, "      <arg type=\"i\"/>\n");
		fprintf (fp, "      <arg type=\"s\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->symbol, "arg1");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->symbol, "arg2");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->symbol, "arg3");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an argument may have an alternate C symbol supplied
	 * using an annotation within it.
	 */
	TEST_FEATURE ("with alternate symbol for argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\">\n");
		fprintf (fp, "        <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                    value=\"wibble_str\"/>\n");
		fprintf (fp, "      </arg>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "wibble_str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that when multiple alternative symbols are given for an
	 * argument, the last is used.
	 */
	TEST_FEATURE ("with multiple alternative symbols for argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\">\n");
		fprintf (fp, "        <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                    value=\"wibble\"/>\n");
		fprintf (fp, "        <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                    value=\"wibble_str\"/>\n");
		fprintf (fp, "      </arg>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "wibble_str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may define multiple arguments, each one
	 * added to the arguments list as a separate Argument structure.
	 */
	TEST_FEATURE ("with multiple arguments to method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "      <arg name=\"len\" type=\"i\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "      <arg name=\"result\" type=\"s\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "len");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "len");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "result");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "result");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may define multiple methods, each one
	 * with its own arguments and each added to the methods list as a
	 * separate Method structure.
	 */
	TEST_FEATURE ("with multiple methods");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "      <arg name=\"len\" type=\"i\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "      <arg name=\"result\" type=\"s\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "    <method name=\"Wobble\">\n");
		fprintf (fp, "      <arg name=\"bounce\" type=\"i\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "    <method name=\"Flounce\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "len");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "len");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "result");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "result");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_NOT_EMPTY (&interface->methods);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wobble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wobble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "bounce");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "bounce");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_NOT_EMPTY (&interface->methods);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Flounce");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "flounce");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may define a signal, which appears in
	 * its signals list as a Signal structure with the name filled
	 * in.
	 */
	TEST_FEATURE ("with argument-less signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a signal can be marked as deprecated using an
	 * annotation tag within it.
	 */
	TEST_FEATURE ("with deprecated signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_TRUE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a signal can be marked as explicitly not deprecated
	 * using an annotation tag within it.
	 */
	TEST_FEATURE ("with explicitly non-deprecated signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"false\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an alternate C symbol fragment can be supplied for a
	 * signal using an annotation tag within it.
	 */
	TEST_FEATURE ("with alternate symbol for signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"wib\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wib");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that when multiple alternative symbols are given for a
	 * signal, the last is used.
	 */
	TEST_FEATURE ("with multiple alternative symbols for signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"wob\"/>\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"wib\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wib");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a signal may define an argument, which appears in its
	 * arguments list as an Argument structure.  For signals, the
	 * argument direction should default to out.
	 */
	TEST_FEATURE ("with argument to signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an argument may have its argument explicitly defined
	 * as "out".
	 */
	TEST_FEATURE ("with output argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that the name attribute of an argument may be optional
	 * and that NULL is stored instead.
	 */
	TEST_FEATURE ("with unnamed argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <arg type=\"s\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->symbol, "arg1");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that multiple arguments may have no name attribute and
	 * that a symbol is generated for each one.
	 */
	TEST_FEATURE ("with multiple unnamed arguments");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <arg type=\"s\"/>\n");
		fprintf (fp, "      <arg type=\"i\"/>\n");
		fprintf (fp, "      <arg type=\"s\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->symbol, "arg1");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->symbol, "arg2");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_P (argument->name, NULL);
		TEST_EQ_STR (argument->symbol, "arg3");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a signal may define multiple arguments, each one
	 * added to the arguments list as a separate Argument structure.
	 */
	TEST_FEATURE ("with multiple arguments to signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"/>\n");
		fprintf (fp, "      <arg name=\"len\" type=\"i\"/>\n");
		fprintf (fp, "      <arg name=\"result\" type=\"s\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "len");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "len");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "result");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "result");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may define multiple signals, each one
	 * with its own arguments and each added to the signals list as a
	 * separate Signal structure.
	 */
	TEST_FEATURE ("with multiple signals");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"/>\n");
		fprintf (fp, "      <arg name=\"len\" type=\"i\"/>\n");
		fprintf (fp, "      <arg name=\"result\" type=\"s\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "    <signal name=\"Wobble\">\n");
		fprintf (fp, "      <arg name=\"bounce\" type=\"i\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "    <signal name=\"Flounce\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "len");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "len");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "result");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "result");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_NOT_EMPTY (&interface->signals);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wobble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wobble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "bounce");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "bounce");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_NOT_EMPTY (&interface->signals);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Flounce");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "flounce");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may define a read-only property, which
	 * appears in its properties list as a Property structure with the
	 * name, type and access filled in.
	 */
	TEST_FEATURE ("with read-only property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may define a write-only property, which
	 * appears in its properties list as a Property structure with the
	 * name, type and access filled in.
	 */
	TEST_FEATURE ("with write-only property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"secret\" type=\"s\"\n");
		fprintf (fp, "              access=\"write\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "secret");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "secret");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_WRITE);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may define a read/write property, which
	 * appears in its properties list as a Property structure with the
	 * name, type and access filled in.
	 */
	TEST_FEATURE ("with read/write property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"nickname\" type=\"s\"\n");
		fprintf (fp, "              access=\"readwrite\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "nickname");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "nickname");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READWRITE);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a property can be marked as deprecated using an
	 * annotation tag within it.
	 */
	TEST_FEATURE ("with deprecated property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </property>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_TRUE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a property can be marked as explicitly not deprecated
	 * using an annotation tag within it.
	 */
	TEST_FEATURE ("with explicitly non-deprecated property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"false\"/>\n");
		fprintf (fp, "    </property>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an alternate C symbol fragment for a property can be
	 * specified using an annotation tag within it.
	 */
	TEST_FEATURE ("with alternate symbol for property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"sz\"/>\n");
		fprintf (fp, "    </property>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "sz");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that when multiple alternative symbols are given for a
	 * property, the last is used.
	 */
	TEST_FEATURE ("with multiple alternative symbols for property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"Size\"/>\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"sz\"/>\n");
		fprintf (fp, "    </property>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "sz");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may define multiple properties, each with
	 * their own name, type and access and each stored as a Property
	 * structure in the interface's properties list.
	 */
	TEST_FEATURE ("with multiple properties");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\"/>\n");
		fprintf (fp, "    <property name=\"secret\" type=\"s\"\n");
		fprintf (fp, "              access=\"write\"/>\n");
		fprintf (fp, "    <property name=\"nickname\" type=\"s\"\n");
		fprintf (fp, "              access=\"readwrite\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "secret");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "secret");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_WRITE);

		nih_list_remove (&property->entry);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "nickname");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "nickname");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READWRITE);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may define any number of methods,
	 * signals and properties each one placed in the appropriate part
	 * of the list.
	 */
	TEST_FEATURE ("with methods, signals and properties");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "      <arg name=\"len\" type=\"i\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "      <arg name=\"result\" type=\"s\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "    <method name=\"Wobble\">\n");
		fprintf (fp, "      <arg name=\"bounce\" type=\"i\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "    <method name=\"Flounce\"/>\n");
		fprintf (fp, "    <signal name=\"Honk\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"/>\n");
		fprintf (fp, "      <arg name=\"len\" type=\"i\"/>\n");
		fprintf (fp, "      <arg name=\"result\" type=\"s\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "    <signal name=\"Bonk\">\n");
		fprintf (fp, "      <arg name=\"bounce\" type=\"i\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "    <signal name=\"Flonk\"/>\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\"/>\n");
		fprintf (fp, "    <property name=\"secret\" type=\"s\"\n");
		fprintf (fp, "              access=\"write\"/>\n");
		fprintf (fp, "    <property name=\"nickname\" type=\"s\"\n");
		fprintf (fp, "              access=\"readwrite\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "len");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "len");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "result");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "result");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_NOT_EMPTY (&interface->methods);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wobble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wobble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "bounce");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "bounce");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_NOT_EMPTY (&interface->methods);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Flounce");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "flounce");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Honk");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "honk");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "len");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "len");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "result");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "result");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_NOT_EMPTY (&interface->signals);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Bonk");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "bonk");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "bounce");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "bounce");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_NOT_EMPTY (&interface->signals);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Flonk");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "flonk");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "secret");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "secret");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_WRITE);

		nih_list_remove (&property->entry);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "nickname");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "nickname");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READWRITE);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a node may define multiple interfaces, each one with
	 * their own set of methods, signals and properties and each of those
	 * with their own arguments and annotations.
	 */
	TEST_FEATURE ("with multiple interfaces");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "      <arg name=\"len\" type=\"i\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "      <arg name=\"result\" type=\"s\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "    <method name=\"Wobble\">\n");
		fprintf (fp, "      <arg name=\"bounce\" type=\"i\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "    <method name=\"Flounce\"/>\n");
		fprintf (fp, "    <signal name=\"Honk\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"/>\n");
		fprintf (fp, "      <arg name=\"len\" type=\"i\"/>\n");
		fprintf (fp, "      <arg name=\"result\" type=\"s\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "    <signal name=\"Bonk\">\n");
		fprintf (fp, "      <arg name=\"bounce\" type=\"i\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "    <signal name=\"Flonk\"/>\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\"/>\n");
		fprintf (fp, "    <property name=\"secret\" type=\"s\"\n");
		fprintf (fp, "              access=\"write\"/>\n");
		fprintf (fp, "    <property name=\"nickname\" type=\"s\"\n");
		fprintf (fp, "              access=\"readwrite\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Peer\">\n");
		fprintf (fp, "    <method name=\"Register\">\n");
		fprintf (fp, "      <arg name=\"name\" type=\"s\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "      <arg name=\"id\" type=\"i\"\n");
		fprintf (fp, "           direction=\"out\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "    <signal name=\"NewPeer\">\n");
		fprintf (fp, "      <arg name=\"name\" type=\"s\"/>\n");
		fprintf (fp, "      <arg name=\"id\" type=\"i\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "len");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "len");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "result");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "result");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_NOT_EMPTY (&interface->methods);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wobble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wobble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "bounce");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "bounce");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_NOT_EMPTY (&interface->methods);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Flounce");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "flounce");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Honk");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "honk");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "len");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "len");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "result");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "result");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_NOT_EMPTY (&interface->signals);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Bonk");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "bonk");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "bounce");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "bounce");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_NOT_EMPTY (&interface->signals);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Flonk");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "flonk");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "secret");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "secret");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_WRITE);

		nih_list_remove (&property->entry);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "nickname");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "nickname");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READWRITE);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Peer");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "peer");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Register");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "register");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "name");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "name");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "id");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "id");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "NewPeer");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "new_peer");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "name");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "name");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_NOT_EMPTY (&signal->arguments);

		argument = (Argument *)signal->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, signal);
		TEST_EQ_STR (argument->name, "id");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "id");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "i");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_OUT);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a node may have child node entities, which may
	 * have their own contents, but that they entire thing is ignored
	 * without so much as a warning.
	 */
	TEST_FEATURE ("with child node");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node name=\"/\">\n");
		fprintf (fp, "  <node name=\"child\">\n");
		fprintf (fp, "    <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "      <method name=\"Wibble\">\n");
		fprintf (fp, "        <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "             direction=\"in\"/>\n");
		fprintf (fp, "        <arg name=\"len\" type=\"i\"\n");
		fprintf (fp, "             direction=\"in\"/>\n");
		fprintf (fp, "        <arg name=\"result\" type=\"s\"\n");
		fprintf (fp, "             direction=\"out\"/>\n");
		fprintf (fp, "      </method>\n");
		fprintf (fp, "      <method name=\"Wobble\">\n");
		fprintf (fp, "        <arg name=\"bounce\" type=\"i\"\n");
		fprintf (fp, "             direction=\"out\"/>\n");
		fprintf (fp, "      </method>\n");
		fprintf (fp, "      <method name=\"Flounce\"/>\n");
		fprintf (fp, "      <signal name=\"Honk\">\n");
		fprintf (fp, "        <arg name=\"str\" type=\"s\"/>\n");
		fprintf (fp, "        <arg name=\"len\" type=\"i\"/>\n");
		fprintf (fp, "        <arg name=\"result\" type=\"s\"/>\n");
		fprintf (fp, "      </signal>\n");
		fprintf (fp, "      <signal name=\"Bonk\">\n");
		fprintf (fp, "        <arg name=\"bounce\" type=\"i\"/>\n");
		fprintf (fp, "      </signal>\n");
		fprintf (fp, "      <signal name=\"Flonk\"/>\n");
		fprintf (fp, "      <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "                access=\"read\"/>\n");
		fprintf (fp, "      <property name=\"secret\" type=\"s\"\n");
		fprintf (fp, "                access=\"write\"/>\n");
		fprintf (fp, "      <property name=\"nickname\" type=\"s\"\n");
		fprintf (fp, "                access=\"readwrite\"/>\n");
		fprintf (fp, "    </interface>\n");
		fprintf (fp, "  </node>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed) {
			TEST_EQ_P (node, NULL);

			TEST_FILE_MATCH (output, ("test:foo:[0-9]*:[0-9]*: "
						  "Cannot allocate memory\n"));
			TEST_FILE_END (output);
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_STR (node->path, "/");
		TEST_ALLOC_PARENT (node->path, node);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown node attribute generates a warning, but
	 * otherwise returns a node.
	 */
	TEST_FEATURE ("with unknown node attribute");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node name=\"/com/netsplit/Nih/Test\" \n");
		fprintf (fp, "      frodo=\"baggins\"/>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_STR (node->path, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (node->path, node);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:1:0: "
				       "Ignored unknown <node> attribute: frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown attribute to the interface tag generates
	 * a warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown interface attribute");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\"\n");
		fprintf (fp, "             frodo=\"baggins\"/>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:2:2: "
				       "Ignored unknown <interface> attribute: frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown attribute to the method tag generates
	 * a warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown method attribute");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\" frodo=\"baggins\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:3:4: "
				       "Ignored unknown <method> attribute: frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown attribute to the signal tag generates
	 * a warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown signal attribute");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\" frodo=\"baggins\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:3:4: "
				       "Ignored unknown <signal> attribute: frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown attribute to the property tag generates
	 * a warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown property attribute");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\" frodo=\"baggins\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:3:4: "
				       "Ignored unknown <property> attribute: frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown attribute to the argument tag generates
	 * a warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown argument attribute");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "           frodo=\"baggins\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:4:6: "
				       "Ignored unknown <arg> attribute: frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown attribute to the annotation tag generates
	 * a warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown annotation attribute");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"true\" frodo=\"baggins\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_TRUE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:4:6: "
				       "Ignored unknown <annotation> attribute: frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a node tag not inside another node tag or at the
	 * top level is ignored generating a warning but that a Node is
	 * otherwise returned.
	 */
	TEST_FEATURE ("with node tag outside of top-level or node");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <node name=\"child\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:3:4: Ignored unexpected <node> tag\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface tag outside of a node tag is ignored
	 * generating a warning but that a Node is otherwise returned.
	 */
	TEST_FEATURE ("with interface tag outside of node");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <interface name=\"com.netsplit.Nih.Inner\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:3:4: Ignored unexpected <interface> tag\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method tag outside of an interface tag is ignored
	 * generating a warning but that a Node is otherwise returned.
	 */
	TEST_FEATURE ("with method tag outside of interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <method name=\"Wibble\"/>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:2:2: Ignored unexpected <method> tag\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a signal tag outside of an interface tag is ignored
	 * generating a warning but that a Node is otherwise returned.
	 */
	TEST_FEATURE ("with signal tag outside of interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <signal name=\"Wibble\"/>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:2:2: Ignored unexpected <signal> tag\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a property tag outside of an interface tag is ignored
	 * generating a warning but that a Node is otherwise returned.
	 */
	TEST_FEATURE ("with property tag outside of interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <property name=\"size\" type=\"i\" access=\"read\"/>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:2:2: Ignored unexpected <property> tag\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an argument tag outside of a method or signal tag is
	 * ignored generating a warning but that a Node is otherwise returned.
	 */
	TEST_FEATURE ("with argument tag outside of method or signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <arg name=\"foo\" type=\"s\"/>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:2:2: Ignored unexpected <arg> tag\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Make sure that properties don't have arguments.
	 */
	TEST_FEATURE ("with argument tag for property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\" access=\"read\">\n");
		fprintf (fp, "      <arg name=\"foo\" type=\"s\"/>\n");
		fprintf (fp, "    </property>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:4:6: Ignored unexpected <arg> tag\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an annotation tag is ignored with a warning if it
	 * is given for a node.
	 */
	TEST_FEATURE ("with annotation for node");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <annotation name=\"com.netsplit.Nih.Test\"\n");
		fprintf (fp, "              value=\"foo\"/>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:2:2: Ignored unexpected <annotation> tag\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown annotation for an interface generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown annotation for interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <annotation name=\"com.netsplit.Apple.Jack\"\n");
		fprintf (fp, "                value=\"true\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:3:4: Ignored unknown interface annotation: "
				       "com.netsplit.Apple.Jack\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown annotation for a method generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown annotation for method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Apple.Jack\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:4:6: Ignored unknown method annotation: "
				       "com.netsplit.Apple.Jack\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown annotation for a signal generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown annotation for signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Apple.Jack\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:4:6: Ignored unknown signal annotation: "
				       "com.netsplit.Apple.Jack\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that the method no reply annotation for a signal generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with no reply annotation for signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Method.NoReply\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:4:6: Ignored unknown signal annotation: "
				       "org.freedesktop.DBus.Method.NoReply\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that the method async annotation for a signal generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with async annotation for signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Method.Async\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		signal = (Signal *)interface->signals.next;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, interface);
		TEST_EQ_STR (signal->name, "Wibble");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_STR (signal->symbol, "wibble");
		TEST_ALLOC_PARENT (signal->symbol, signal);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_list_remove (&signal->entry);
		TEST_LIST_EMPTY (&interface->signals);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:4:6: Ignored unknown signal annotation: "
				       "com.netsplit.Nih.Method.Async\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown annotation for a property generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown annotation for property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Apple.Jack\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </property>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:5:6: Ignored unknown property annotation: "
				       "com.netsplit.Apple.Jack\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method no reply annotation for a property generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with no reply annotation for property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Method.NoReply\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </property>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:5:6: Ignored unknown property annotation: "
				       "org.freedesktop.DBus.Method.NoReply\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method async annotation for a property generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with async annotation for property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Method.Async\"\n");
		fprintf (fp, "                  value=\"true\"/>\n");
		fprintf (fp, "    </property>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_NOT_EMPTY (&interface->properties);

		property = (Property *)interface->properties.next;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, interface);
		TEST_EQ_STR (property->name, "size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->symbol, "size");
		TEST_ALLOC_PARENT (property->symbol, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_FALSE (property->deprecated);
		TEST_EQ (property->access, NIH_DBUS_READ);

		nih_list_remove (&property->entry);
		TEST_LIST_EMPTY (&interface->properties);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:5:6: Ignored unknown property annotation: "
				       "com.netsplit.Nih.Method.Async\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown annotation for an argument generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with unknown annotation for argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\">\n");
		fprintf (fp, "        <annotation name=\"com.netsplit.Apple.Jack\"\n");
		fprintf (fp, "                    value=\"true\"/>\n");
		fprintf (fp, "      </arg>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:5:8: Ignored unknown argument annotation: "
				       "com.netsplit.Apple.Jack\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a deprecated annotation for an argument generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with deprecated annotation for argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\">\n");
		fprintf (fp, "        <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                    value=\"true\"/>\n");
		fprintf (fp, "      </arg>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:5:8: Ignored unknown argument annotation: "
				       "org.freedesktop.DBus.Deprecated\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}



	/* Check that a method no reply annotation for an argument generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with no reply annotation for argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\">\n");
		fprintf (fp, "        <annotation name=\"org.freedesktop.DBus.Method.NoReply\"\n");
		fprintf (fp, "                    value=\"true\"/>\n");
		fprintf (fp, "      </arg>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:5:8: Ignored unknown argument annotation: "
				       "org.freedesktop.DBus.Method.NoReply\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an async method annotation for an argument generates a
	 * warning but is otherwise ignored.
	 */
	TEST_FEATURE ("with async annotation for argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\">\n");
		fprintf (fp, "        <annotation name=\"com.netsplit.Nih.Method.Async\"\n");
		fprintf (fp, "                    value=\"true\"/>\n");
		fprintf (fp, "      </arg>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_NOT_EMPTY (&node->interfaces);

		interface = (Interface *)node->interfaces.next;
		TEST_ALLOC_SIZE (interface, sizeof (Interface));
		TEST_ALLOC_PARENT (interface, node);
		TEST_EQ_STR (interface->name, "com.netsplit.Nih.Test");
		TEST_ALLOC_PARENT (interface->name, interface);
		TEST_EQ_STR (interface->symbol, "test");
		TEST_ALLOC_PARENT (interface->symbol, interface);
		TEST_FALSE (interface->deprecated);
		TEST_LIST_NOT_EMPTY (&interface->methods);
		TEST_LIST_EMPTY (&interface->signals);
		TEST_LIST_EMPTY (&interface->properties);

		method = (Method *)interface->methods.next;
		TEST_ALLOC_SIZE (method, sizeof (Method));
		TEST_ALLOC_PARENT (method, interface);
		TEST_EQ_STR (method->name, "Wibble");
		TEST_ALLOC_PARENT (method->name, method);
		TEST_EQ_STR (method->symbol, "wibble");
		TEST_ALLOC_PARENT (method->symbol, method);
		TEST_FALSE (method->deprecated);
		TEST_FALSE (method->no_reply);
		TEST_FALSE (method->async);
		TEST_LIST_NOT_EMPTY (&method->arguments);

		argument = (Argument *)method->arguments.next;
		TEST_ALLOC_SIZE (argument, sizeof (Argument));
		TEST_ALLOC_PARENT (argument, method);
		TEST_EQ_STR (argument->name, "str");
		TEST_ALLOC_PARENT (argument->name, argument);
		TEST_EQ_STR (argument->symbol, "str");
		TEST_ALLOC_PARENT (argument->symbol, argument);
		TEST_EQ_STR (argument->type, "s");
		TEST_ALLOC_PARENT (argument->type, argument);
		TEST_EQ (argument->direction, NIH_DBUS_ARG_IN);

		nih_list_remove (&argument->entry);
		TEST_LIST_EMPTY (&method->arguments);

		nih_list_remove (&method->entry);
		TEST_LIST_EMPTY (&interface->methods);

		nih_list_remove (&interface->entry);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:5:8: Ignored unknown argument annotation: "
				       "com.netsplit.Nih.Method.Async\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown tag within the parsing stack is ignored
	 * generating a warning but that a Node is otherwise returned.
	 */
	TEST_FEATURE ("with unknown tag");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <flirble/>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:2:2: Ignored unknown tag: flirble\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that the contents of an unknown tag are ignored, even if
	 * they are tags we'd normally parse.  This ensures that future
	 * specifications can add them in a way we ignore.
	 */
	TEST_FEATURE ("with unknown tag and contents");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <flirble>\n");
		fprintf (fp, "    <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "      <method name=\"Wibble\">\n");
		fprintf (fp, "        <arg name=\"str\" type=\"s\"\n");
		fprintf (fp, "             direction=\"in\"/>\n");
		fprintf (fp, "        <arg name=\"len\" type=\"i\"\n");
		fprintf (fp, "             direction=\"in\"/>\n");
		fprintf (fp, "        <arg name=\"result\" type=\"s\"\n");
		fprintf (fp, "             direction=\"out\"/>\n");
		fprintf (fp, "      </method>\n");
		fprintf (fp, "      <method name=\"Wobble\">\n");
		fprintf (fp, "        <arg name=\"bounce\" type=\"i\"\n");
		fprintf (fp, "             direction=\"out\"/>\n");
		fprintf (fp, "      </method>\n");
		fprintf (fp, "      <method name=\"Flounce\"/>\n");
		fprintf (fp, "      <signal name=\"Honk\">\n");
		fprintf (fp, "        <arg name=\"str\" type=\"s\"/>\n");
		fprintf (fp, "        <arg name=\"len\" type=\"i\"/>\n");
		fprintf (fp, "        <arg name=\"result\" type=\"s\"/>\n");
		fprintf (fp, "      </signal>\n");
		fprintf (fp, "      <signal name=\"Bonk\">\n");
		fprintf (fp, "        <arg name=\"bounce\" type=\"i\"/>\n");
		fprintf (fp, "      </signal>\n");
		fprintf (fp, "      <signal name=\"Flonk\"/>\n");
		fprintf (fp, "      <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "                access=\"read\"/>\n");
		fprintf (fp, "      <property name=\"secret\" type=\"s\"\n");
		fprintf (fp, "                access=\"write\"/>\n");
		fprintf (fp, "      <property name=\"nickname\" type=\"s\"\n");
		fprintf (fp, "                access=\"readwrite\"/>\n");
		fprintf (fp, "    </interface>\n");
		fprintf (fp, "  </flirble>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_NE_P (node, NULL);

		TEST_ALLOC_SIZE (node, sizeof (Node));
		TEST_EQ_P (node->path, NULL);
		TEST_LIST_EMPTY (&node->interfaces);

		nih_free (node);

		TEST_FILE_EQ (output, ("test:foo:2:2: Ignored unknown tag: flirble\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an unknown tag may not be the root */
	TEST_FEATURE ("with unknown root tag");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<flirble/>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown tag: flirble\n"));
		TEST_FILE_EQ (output, ("test:foo: No node present\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid node name is rejected at the point of
	 * parsing.
	 */
	TEST_FEATURE ("with invalid node name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node name=\"com/netsplit/Nih/Test\"/>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:2:0: "
				       "Invalid object path in <node> name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an interface may not be missing its name.
	 */
	TEST_FEATURE ("with missing interface name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface/>");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:2:14: "
				       "<interface> missing required name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid interface name is rejected at the point
	 * of parsing.
	 */
	TEST_FEATURE ("with invalid interface name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\".com.netsplit.Nih.Test\"/>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:2:44: "
				       "Invalid interface name in <interface> name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a method may not be missing its name.
	 */
	TEST_FEATURE ("with missing method name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:3:13: "
				       "<method> missing required name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid method name is rejected at the point
	 * of parsing.
	 */
	TEST_FEATURE ("with invalid method name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"foo bar\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:3:28: "
				       "Invalid method name in <method> name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a signal may not be missing its name.
	 */
	TEST_FEATURE ("with missing signal name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:3:13: "
				       "<signal> missing required name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid signal name is rejected at the point
	 * of parsing.
	 */
	TEST_FEATURE ("with invalid signal name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"foo bar\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:3:28: "
				       "Invalid signal name in <signal> name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a property may not be missing its name.
	 */
	TEST_FEATURE ("with missing property name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property type=\"s\"\n");
		fprintf (fp, "              access=\"read\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:29: "
				       "<property> missing required name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid property name is rejected at the point
	 * of parsing.
	 */
	TEST_FEATURE ("with invalid property name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"foo bar\" type=\"s\"\n");
		fprintf (fp, "              access=\"read\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:29: "
				       "Invalid property name in <property> name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a property may not be missing its type.
	 */
	TEST_FEATURE ("with missing property type");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"nick\"\n");
		fprintf (fp, "              access=\"read\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:29: "
				       "<property> missing required type attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid property type is rejected at the point
	 * of parsing.
	 */
	TEST_FEATURE ("with invalid property type");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"nick\" type=\"si\"\n");
		fprintf (fp, "              access=\"read\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:29: "
				       "Invalid D-Bus type in <property> type attribute: "
				       "Exactly one complete type required in signature\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that a property may not be missing its access (there is no
	 * default according to the specification).
	 */
	TEST_FEATURE ("with missing property access");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"nick\" type=\"s\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:3:36: "
				       "<property> missing required access attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid property access is rejected at the point
	 * of parsing.
	 */
	TEST_FEATURE ("with invalid property access");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"nick\" type=\"s\"\n");
		fprintf (fp, "              access=\"sneak\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:30: "
				       "Illegal value for <property> access attribute, "
				       "expected 'read', 'write' or 'readwrite'\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid argument name is rejected at the point
	 * of parsing.
	 */
	TEST_FEATURE ("with invalid argument name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"foo bar\" type=\"s\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:36: "
				       "Invalid argument name in <arg> name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an argument may not be missing its type.
	 */
	TEST_FEATURE ("with missing argument type");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"foo\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:23: "
				       "<arg> missing required type attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid argument type is rejected at the point
	 * of parsing.
	 */
	TEST_FEATURE ("with invalid argument type");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"foo\" type=\"!\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:32: "
				       "Invalid D-Bus type in <arg> type attribute: "
				       "Unknown typecode\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid argument direction is rejected at the point
	 * of parsing.
	 */
	TEST_FEATURE ("with invalid argument direction");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"foo\" type=\"s\"\n");
		fprintf (fp, "           direction=\"widdershins\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:5:36: "
				       "Illegal value for <arg> direction attribute, "
				       "expected 'in' or 'out'\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that "in" is an invalid argument direction for signals,
	 * and is rejected at the point of parsing.
	 */
	TEST_FEATURE ("with invalid argument direction for signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"foo\" type=\"s\"\n");
		fprintf (fp, "           direction=\"in\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:5:27: "
				       "Illegal value for <arg> direction attribute, "
				       "expected 'out'\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an annotation may not be missing its name.
	 */
	TEST_FEATURE ("with missing annotation name");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation value=\"true\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:32: "
				       "<annotation> missing required name attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an annotation may not be missing its value.
	 */
	TEST_FEATURE ("with missing annotation value");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:58: "
				       "<annotation> missing required value attribute\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an illegal value for the deprecated annotation on an
	 * interface results in an error.
	 */
	TEST_FEATURE ("with illegal value for deprecated interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                value=\"frodo\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:4:31: Illegal value for org.freedesktop.DBus.Deprecated interface annotation, "
				       "expected 'true' or 'false'\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid C symbol part for the annotation on an
	 * interface results in an error.
	 */
	TEST_FEATURE ("with invalid symbol for interface");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                value=\"foo bar\"/>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, "test:foo:4:33: Invalid C symbol for interface\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an illegal value for the deprecated annotation on a
	 * method results in an error.
	 */
	TEST_FEATURE ("with illegal value for deprecated method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"frodo\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:5:33: Illegal value for org.freedesktop.DBus.Deprecated method annotation, "
				       "expected 'true' or 'false'\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid C symbol part for the annotation on a
	 * method results in an error.
	 */
	TEST_FEATURE ("with invalid symbol for method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"foo bar\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, "test:foo:5:35: Invalid C symbol for method\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an illegal value for the no reply annotation on a
	 * method results in an error.
	 */
	TEST_FEATURE ("with illegal value for no reply method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Method.NoReply\"\n");
		fprintf (fp, "                  value=\"frodo\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:5:33: Illegal value for org.freedesktop.DBus.Method.NoReply method annotation, "
				       "expected 'true' or 'false'\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an illegal value for the async annotation on a
	 * method results in an error.
	 */
	TEST_FEATURE ("with illegal value for async method");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Method.Async\"\n");
		fprintf (fp, "                  value=\"frodo\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:5:33: Illegal value for com.netsplit.Nih.Method.Async method annotation, "
				       "expected 'true' or 'false'\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an illegal value for the deprecated annotation on a
	 * signal results in an error.
	 */
	TEST_FEATURE ("with illegal value for deprecated signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"frodo\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:5:33: Illegal value for org.freedesktop.DBus.Deprecated signal annotation, "
				       "expected 'true' or 'false'\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid C symbol part for the annotation on a
	 * signal results in an error.
	 */
	TEST_FEATURE ("with invalid symbol for signal");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"foo bar\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, "test:foo:5:35: Invalid C symbol for signal\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an illegal value for the deprecated annotation on a
	 * property results in an error.
	 */
	TEST_FEATURE ("with illegal value for deprecated property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\">\n");
		fprintf (fp, "      <annotation name=\"org.freedesktop.DBus.Deprecated\"\n");
		fprintf (fp, "                  value=\"frodo\"/>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:6:33: Illegal value for org.freedesktop.DBus.Deprecated property annotation, "
				       "expected 'true' or 'false'\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid C symbol part for the annotation on a
	 * property results in an error.
	 */
	TEST_FEATURE ("with invalid symbol for property");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <property name=\"size\" type=\"i\"\n");
		fprintf (fp, "              access=\"read\">\n");
		fprintf (fp, "      <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                  value=\"foo bar\"/>\n");
		fprintf (fp, "    </property>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, "test:foo:6:35: Invalid C symbol for property\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that an invalid C symbol part for the annotation on an
	 * argument results in an error.
	 */
	TEST_FEATURE ("with invalid symbol for argument");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <method name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"str\" type=\"s\">\n");
		fprintf (fp, "        <annotation name=\"com.netsplit.Nih.Symbol\"\n");
		fprintf (fp, "                    value=\"foo bar\"/>\n");
		fprintf (fp, "      </arg>\n");
		fprintf (fp, "    </method>\n");
		fprintf (fp, "  </interface>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, "test:foo:6:37: Invalid C symbol for argument\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	/* Check that ordinary XML parsing errors are picked up by the
	 * parser and treated as errors.
	 */
	TEST_FEATURE ("with XML error");
	TEST_ALLOC_FAIL {
		TEST_FILE_RESET (fp);

		fprintf (fp, "<node>\n");
		fprintf (fp, "  <interface name=\"com.netsplit.Nih.Test\">\n");
		fprintf (fp, "    <signal name=\"Wibble\">\n");
		fprintf (fp, "      <arg name=\"foo\" type=\"s\"/>\n");
		fprintf (fp, "    </signal>\n");
		fprintf (fp, "  </elephant>\n");
		fprintf (fp, "</node>\n");
		fflush (fp);
		rewind (fp);

		TEST_DIVERT_STDERR (output) {
			node = parse_xml (NULL, fileno (fp), "foo");
		}
		rewind (output);

		if (test_alloc_failed
		    && (node == NULL)) {
			TEST_FILE_RESET (output);
			continue;
		}

		TEST_EQ_P (node, NULL);

		TEST_FILE_EQ (output, ("test:foo:6:4: XML parse error: "
				       "mismatched tag\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);
	}


	fclose (fp);
	fclose (output);
}


int
main (int   argc,
      char *argv[])
{
	program_name = "test";
	nih_error_init ();

	test_stack_push ();
	test_stack_top ();
	test_start_tag ();
	test_end_tag ();
	test_parse_xml ();

	return 0;
}
