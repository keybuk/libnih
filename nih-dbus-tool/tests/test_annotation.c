/* nih-dbus-tool
 *
 * test_annotation.c - test suite for nih-dbus-tool/annotation.c
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
#include <nih/main.h>
#include <nih/error.h>

#include <nih-dbus/dbus_object.h>

#include "interface.h"
#include "method.h"
#include "signal.h"
#include "property.h"
#include "argument.h"
#include "parse.h"
#include "errors.h"

#include "annotation.h"


void
test_start_tag (void)
{
	ParseContext context;
	ParseStack * parent = NULL;
	ParseStack * entry;
	XML_Parser   xmlp;
	Node *       node = NULL;
	Interface *  interface = NULL;
	Method *     method = NULL;
	Signal *     signal = NULL;
	Property *   property = NULL;
	Argument *   argument = NULL;
	char *       attr[7];
	int          ret = 0;
	NihError *   err;
	FILE *       output;

	TEST_FUNCTION ("annotation_start_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);

	output = tmpfile ();


	/* Check that an annotation for an interface with the usual name and
	 * value attributes is passed to the interface_annotation() function
	 * and results in it being handled with an empty annotation stack
	 * object being pushed onto the stack.
	 */
	TEST_FEATURE ("with interface annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		attr[0] = "name";
		attr[1] = "org.freedesktop.DBus.Deprecated";
		attr[2] = "value";
		attr[3] = "true";
		attr[4] = NULL;

		ret = annotation_start_tag (xmlp, "annotation", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

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
		TEST_EQ (entry->type, PARSE_ANNOTATION);
		TEST_EQ_P (entry->data, NULL);

		TEST_TRUE (interface->deprecated);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an annotation for a method with the usual name and
	 * value attributes is passed to the method_annotation() function
	 * and results in it being handled with an empty annotation stack
	 * object being pushed onto the stack.
	 */
	TEST_FEATURE ("with method annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);
		}

		attr[0] = "name";
		attr[1] = "org.freedesktop.DBus.Deprecated";
		attr[2] = "value";
		attr[3] = "true";
		attr[4] = NULL;

		ret = annotation_start_tag (xmlp, "annotation", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

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
		TEST_EQ (entry->type, PARSE_ANNOTATION);
		TEST_EQ_P (entry->data, NULL);

		TEST_TRUE (method->deprecated);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an annotation for a signal with the usual name and
	 * value attributes is passed to the signal_annotation() function
	 * and results in it being handled with an empty annotation stack
	 * object being pushed onto the stack.
	 */
	TEST_FEATURE ("with signal annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);
		}

		attr[0] = "name";
		attr[1] = "org.freedesktop.DBus.Deprecated";
		attr[2] = "value";
		attr[3] = "true";
		attr[4] = NULL;

		ret = annotation_start_tag (xmlp, "annotation", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

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
		TEST_EQ (entry->type, PARSE_ANNOTATION);
		TEST_EQ_P (entry->data, NULL);

		TEST_TRUE (signal->deprecated);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an annotation for a property with the usual name and
	 * value attributes is passed to the property_annotation() function
	 * and results in it being handled with an empty annotation stack
	 * object being pushed onto the stack.
	 */
	TEST_FEATURE ("with property annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			property = property_new (NULL, "TestProperty",
						 "s", NIH_DBUS_READ);
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_PROPERTY, property);
			nih_discard (property);
		}

		attr[0] = "name";
		attr[1] = "org.freedesktop.DBus.Deprecated";
		attr[2] = "value";
		attr[3] = "true";
		attr[4] = NULL;

		ret = annotation_start_tag (xmlp, "annotation", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

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
		TEST_EQ (entry->type, PARSE_ANNOTATION);
		TEST_EQ_P (entry->data, NULL);

		TEST_TRUE (property->deprecated);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an annotation for an argument with the usual name and
	 * value attributes is passed to the argument_annotation() function
	 * and results in it being handled with an empty annotation stack
	 * object being pushed onto the stack.
	 */
	TEST_FEATURE ("with argument annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			argument = argument_new (NULL, "test_arg",
						 "s", NIH_DBUS_ARG_IN);
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_ARGUMENT, argument);
			nih_discard (argument);
		}

		attr[0] = "name";
		attr[1] = "com.netsplit.Nih.Symbol";
		attr[2] = "value";
		attr[3] = "test";
		attr[4] = NULL;

		ret = annotation_start_tag (xmlp, "annotation", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

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
		TEST_EQ (entry->type, PARSE_ANNOTATION);
		TEST_EQ_P (entry->data, NULL);

		TEST_EQ_STR (argument->symbol, "test");
		TEST_ALLOC_PARENT (argument->symbol, argument);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an annotation with a missing name attribute results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with missing name");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			attr[0] = "value";
			attr[1] = "true";
			attr[2] = NULL;
		}

		ret = annotation_start_tag (xmlp, "annotation", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&interface->methods);

		err = nih_error_get ();
		TEST_EQ (err->number, ANNOTATION_MISSING_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that an annotation with a missing value attribute results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with missing value");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			attr[0] = "name";
			attr[1] = "org.freedesktop.DBus.Deprecated";
			attr[2] = NULL;
		}

		ret = annotation_start_tag (xmlp, "annotation", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&interface->methods);

		err = nih_error_get ();
		TEST_EQ (err->number, ANNOTATION_MISSING_VALUE);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that an unknown annotation attribute results in a warning
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
			attr[1] = "org.freedesktop.DBus.Deprecated";
			attr[2] = "value";
			attr[3] = "true";
			attr[4] = "frodo";
			attr[5] = "baggins";
			attr[6] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = annotation_start_tag (xmlp, "annotation", attr);
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
		TEST_EQ (entry->type, PARSE_ANNOTATION);
		TEST_EQ_P (entry->data, NULL);

		TEST_TRUE (interface->deprecated);

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown <annotation> attribute: "
				       "frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an annotation on an empty stack (ie. a top-level
	 * annotation element) results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with empty stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			attr[0] = "name";
			attr[1] = "org.freedesktop.DBus.Deprecated";
			attr[2] = "value";
			attr[3] = "true";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = annotation_start_tag (xmlp, "annotation", attr);
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

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <annotation> tag\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
	}


	/* Check that an annotation on top of a stack entry that's not an
	 * interface, method, signal, property or argument results in a
	 * warning being printed on standard error and an ignored element
	 * being pushed onto the stack.
	 */
	TEST_FEATURE ("with non-annotated element on stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			node = node_new (NULL, NULL);
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);
			nih_discard (node);

			attr[0] = "name";
			attr[1] = "org.freedesktop.DBus.Deprecated";
			attr[2] = "value";
			attr[3] = "true";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = annotation_start_tag (xmlp, "annotation", attr);
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

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <annotation> tag\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an unknown interface annotation error is converted
	 * into a warning and printed to standard error, and results in
	 * an ignored element being pushed onto the stack.
	 */
	TEST_FEATURE ("with unknown interface annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			attr[0] = "name";
			attr[1] = "com.netsplit.Nih.Unknown";
			attr[2] = "value";
			attr[3] = "true";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = annotation_start_tag (xmlp, "annotation", attr);
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

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown interface annotation: "
				       "com.netsplit.Nih.Unknown\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an unknown method annotation error is converted
	 * into a warning and printed to standard error, and results in
	 * an ignored element being pushed onto the stack.
	 */
	TEST_FEATURE ("with unknown method annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			attr[0] = "name";
			attr[1] = "com.netsplit.Nih.Unknown";
			attr[2] = "value";
			attr[3] = "true";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = annotation_start_tag (xmlp, "annotation", attr);
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

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown method annotation: "
				       "com.netsplit.Nih.Unknown\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an unknown signal annotation error is converted
	 * into a warning and printed to standard error, and results in
	 * an ignored element being pushed onto the stack.
	 */
	TEST_FEATURE ("with unknown signal annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_SIGNAL, signal);
			nih_discard (signal);

			attr[0] = "name";
			attr[1] = "com.netsplit.Nih.Unknown";
			attr[2] = "value";
			attr[3] = "true";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = annotation_start_tag (xmlp, "annotation", attr);
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

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown signal annotation: "
				       "com.netsplit.Nih.Unknown\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an unknown property annotation error is converted
	 * into a warning and printed to standard error, and results in
	 * an ignored element being pushed onto the stack.
	 */
	TEST_FEATURE ("with unknown property annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			property = property_new (NULL, "TestProperty", "s", NIH_DBUS_READ);
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_PROPERTY, property);
			nih_discard (property);

			attr[0] = "name";
			attr[1] = "com.netsplit.Nih.Unknown";
			attr[2] = "value";
			attr[3] = "true";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = annotation_start_tag (xmlp, "annotation", attr);
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

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown property annotation: "
				       "com.netsplit.Nih.Unknown\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an unknown argument annotation error is converted
	 * into a warning and printed to standard error, and results in
	 * an ignored element being pushed onto the stack.
	 */
	TEST_FEATURE ("with unknown argument annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			argument = argument_new (NULL, "test_arg", "s", NIH_DBUS_ARG_IN);
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_ARGUMENT, argument);
			nih_discard (argument);

			attr[0] = "name";
			attr[1] = "com.netsplit.Nih.Unknown";
			attr[2] = "value";
			attr[3] = "true";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = annotation_start_tag (xmlp, "annotation", attr);
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

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown argument annotation: "
				       "com.netsplit.Nih.Unknown\n"));
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
	int          ret;
	NihError *   err;

	/* Check that when we parse the end tag for an annotation, we pop
	 * the top of the stack off and free it.
	 */
	TEST_FUNCTION ("annotation_end_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			method = method_new (NULL, "TestMethod");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_METHOD, method);
			nih_discard (method);

			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_ANNOTATION, NULL);
		}

		TEST_FREE_TAG (entry);

		ret = annotation_end_tag (xmlp, "annotation");

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

		nih_free (parent);
	}


	XML_ParserFree (xmlp);
}


int
main (int   argc,
      char *argv[])
{
	program_name = "test";
	nih_error_init ();

	test_start_tag ();
	test_end_tag ();

	return 0;
}
