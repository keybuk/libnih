/* nih-dbus-tool
 *
 * test_signal.c - test suite for nih-dbus-tool/signal.c
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
#include <nih-dbus/dbus_proxy.h>

#include "type.h"
#include "node.h"
#include "signal.h"
#include "argument.h"
#include "parse.h"
#include "errors.h"
#include "tests/signal_code.h"


void
test_name_valid (void)
{
	TEST_FUNCTION ("signal_name_valid");

	/* Check that a typical signal name is valid. */
	TEST_FEATURE ("with typical signal name");
	TEST_TRUE (signal_name_valid ("Wibble"));


	/* Check that an signal name is not valid if it is has an
	 * initial period.
	 */
	TEST_FEATURE ("with initial period");
	TEST_FALSE (signal_name_valid (".Wibble"));


	/* Check that an signal name is not valid if it ends with a period
	 */
	TEST_FEATURE ("with final period");
	TEST_FALSE (signal_name_valid ("Wibble."));


	/* Check that an signal name is not valid if it contains a period
	 */
	TEST_FEATURE ("with period");
	TEST_FALSE (signal_name_valid ("Wib.ble"));


	/* Check that a signal name may contain numbers */
	TEST_FEATURE ("with numbers");
	TEST_TRUE (signal_name_valid ("Wib43ble"));


	/* Check that a signal name may not begin with numbers */
	TEST_FEATURE ("with leading digits");
	TEST_FALSE (signal_name_valid ("43Wibble"));


	/* Check that a signal name may end with numbers */
	TEST_FEATURE ("with trailing digits");
	TEST_TRUE (signal_name_valid ("Wibble43"));


	/* Check that a signal name may contain underscores */
	TEST_FEATURE ("with underscore");
	TEST_TRUE (signal_name_valid ("Wib_ble"));


	/* Check that a signal name may begin with underscores */
	TEST_FEATURE ("with initial underscore");
	TEST_TRUE (signal_name_valid ("_Wibble"));


	/* Check that a signal name may end with underscores */
	TEST_FEATURE ("with final underscore");
	TEST_TRUE (signal_name_valid ("Wibble_"));


	/* Check that other characters are not permitted */
	TEST_FEATURE ("with non-permitted characters");
	TEST_FALSE (signal_name_valid ("Wib-ble"));


	/* Check that an empty signal name is invalid */
	TEST_FEATURE ("with empty string");
	TEST_FALSE (signal_name_valid (""));


	/* Check that an signal name may not exceed 255 characters */
	TEST_FEATURE ("with overly long name");
	TEST_FALSE (signal_name_valid ("ReallyLongSignalNameThatNobody"
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
	Signal *signal;

	/* Check that an Signal object is allocated with the structure
	 * filled in properly, but not placed in a list.
	 */
	TEST_FUNCTION ("signal_new");
	TEST_ALLOC_FAIL {
		signal = signal_new (NULL, "Yahoo");

		if (test_alloc_failed) {
			TEST_EQ_P (signal, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_LIST_EMPTY (&signal->entry);
		TEST_EQ_STR (signal->name, "Yahoo");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_P (signal->symbol, NULL);
		TEST_FALSE (signal->deprecated);
		TEST_LIST_EMPTY (&signal->arguments);

		nih_free (signal);
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
	Signal *     signal;
	char *       attr[5];
	int          ret;
	NihError *   err;
	FILE *       output;

	TEST_FUNCTION ("signal_start_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);

	output = tmpfile ();


	/* Check that an signal tag for an interface with the usual name
	 * attribute results in an Signal member being created and pushed
	 * onto the stack with that attribute filled in correctly.
	 */
	TEST_FEATURE ("with signal");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
		}

		attr[0] = "name";
		attr[1] = "TestSignal";
		attr[2] = NULL;

		ret = signal_start_tag (xmlp, "signal", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&interface->signals);

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
		TEST_EQ (entry->type, PARSE_SIGNAL);

		signal = entry->signal;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, entry);
		TEST_EQ_STR (signal->name, "TestSignal");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_P (signal->symbol, NULL);
		TEST_LIST_EMPTY (&signal->arguments);

		TEST_LIST_EMPTY (&interface->signals);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a signal with a missing name attribute results
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

		ret = signal_start_tag (xmlp, "signal", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&interface->signals);

		err = nih_error_get ();
		TEST_EQ (err->number, SIGNAL_MISSING_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that a signal with an invalid name results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid name");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);

			attr[0] = "name";
			attr[1] = "Test Signal";
			attr[2] = NULL;
		}

		ret = signal_start_tag (xmlp, "signal", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&interface->signals);

		err = nih_error_get ();
		TEST_EQ (err->number, SIGNAL_INVALID_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that an unknown signal attribute results in a warning
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
			attr[1] = "TestSignal";
			attr[2] = "frodo";
			attr[3] = "baggins";
			attr[4] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = signal_start_tag (xmlp, "signal", attr);
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
		TEST_EQ (entry->type, PARSE_SIGNAL);

		signal = entry->signal;
		TEST_ALLOC_SIZE (signal, sizeof (Signal));
		TEST_ALLOC_PARENT (signal, entry);
		TEST_EQ_STR (signal->name, "TestSignal");
		TEST_ALLOC_PARENT (signal->name, signal);
		TEST_EQ_P (signal->symbol, NULL);
		TEST_LIST_EMPTY (&signal->arguments);

		TEST_LIST_EMPTY (&interface->signals);

		TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown <signal> attribute: "
				       "frodo\n"));
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a signal on an empty stack (ie. a top-level
	 * signal element) results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with empty stack");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			attr[0] = "name";
			attr[1] = "TestSignal";
			attr[2] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = signal_start_tag (xmlp, "signal", attr);
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

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <signal> tag\n");
		TEST_FILE_END (output);
		TEST_FILE_RESET (output);

		nih_free (entry);
	}


	/* Check that a signal on top of a stack entry that's not an
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
			attr[1] = "TestSignal";
			attr[2] = NULL;
		}

		TEST_DIVERT_STDERR (output) {
			ret = signal_start_tag (xmlp, "signal", attr);
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

		TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <signal> tag\n");
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
	Interface *  interface = NULL;
	Signal *     signal = NULL;
	Signal *     other = NULL;
	int          ret;
	NihError *   err;

	TEST_FUNCTION ("signal_end_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);


	/* Check that when we parse the end tag for a signal, we pop
	 * the Signal object off the stack (freeing and removing it)
	 * and append it to the parent interface's signals list, adding a
	 * reference to the interface as well.  A symbol should be generated
	 * for the signal by convering its name to C style.
	 */
	TEST_FEATURE ("with no assigned symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);

			signal = signal_new (NULL, "TestSignal");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_SIGNAL, signal);
		}

		TEST_FREE_TAG (entry);

		ret = signal_end_tag (xmlp, "signal");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->signals);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (signal, interface);

		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_EQ_P (interface->signals.next, &signal->entry);

		TEST_EQ_STR (signal->symbol, "test_signal");
		TEST_ALLOC_PARENT (signal->symbol, signal);

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

			signal = signal_new (NULL, "TestSignal");
			signal->symbol = nih_strdup (signal, "foo");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_SIGNAL, signal);
		}

		TEST_FREE_TAG (entry);

		ret = signal_end_tag (xmlp, "signal");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->signals);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (signal, interface);

		TEST_LIST_NOT_EMPTY (&interface->signals);
		TEST_EQ_P (interface->signals.next, &signal->entry);

		TEST_EQ_STR (signal->symbol, "foo");
		TEST_ALLOC_PARENT (signal->symbol, signal);

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

			other = signal_new (interface, "Test");
			other->symbol = nih_strdup (other, "test_signal");
			nih_list_add (&interface->signals, &other->entry);

			signal = signal_new (NULL, "TestSignal");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_SIGNAL, signal);
		}

		ret = signal_end_tag (xmlp, "signal");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		if ((! test_alloc_failed)
		    || (err->number != ENOMEM))
			TEST_EQ (err->number, SIGNAL_DUPLICATE_SYMBOL);
		nih_free (err);

		nih_free (entry);
		nih_free (parent);
	}


	XML_ParserFree (xmlp);
}


void
test_annotation (void)
{
	Signal *  signal = NULL;
	char *    symbol;
	int       ret;
	NihError *err;

	TEST_FUNCTION ("signal_annotation");


	/* Check that the annotation to mark a signal as deprecated is
	 * handled, and the Signal is marked deprecated.
	 */
	TEST_FEATURE ("with deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
		}

		ret = signal_annotation (signal,
					 "org.freedesktop.DBus.Deprecated",
					 "true");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_FALSE (signal->deprecated);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (signal);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_TRUE (signal->deprecated);

		nih_free (signal);
	}


	/* Check that the annotation to mark a signal as deprecated can be
	 * given a false value to explicitly mark the Signal non-deprecated.
	 */
	TEST_FEATURE ("with explicitly non-deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			signal->deprecated = TRUE;
		}

		ret = signal_annotation (signal,
					 "org.freedesktop.DBus.Deprecated",
					 "false");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_TRUE (signal->deprecated);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (signal);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FALSE (signal->deprecated);

		nih_free (signal);
	}


	/* Check that an annotation to add a symbol to the signal is
	 * handled, and the new symbol is stored in the signal.
	 */
	TEST_FEATURE ("with symbol annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
		}

		ret = signal_annotation (signal,
					 "com.netsplit.Nih.Symbol",
					 "foo");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (signal);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (signal->symbol, "foo");
		TEST_ALLOC_PARENT (signal->symbol, signal);

		nih_free (signal);
	}


	/* Check that an annotation to add a symbol to the signal
	 * replaces any previous symbol applied (e.g. by a previous
	 * annotation).
	 */
	TEST_FEATURE ("with symbol annotation and existing symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
			signal->symbol = nih_strdup (signal, "test_arg");
		}

		symbol = signal->symbol;
		TEST_FREE_TAG (symbol);

		ret = signal_annotation (signal,
					 "com.netsplit.Nih.Symbol",
					 "foo");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (signal);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (symbol);

		TEST_EQ_STR (signal->symbol, "foo");
		TEST_ALLOC_PARENT (signal->symbol, signal);

		nih_free (signal);
	}


	/* Check that an invalid value for the deprecated annotation results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid value for deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
		}

		ret = signal_annotation (signal,
					 "org.freedesktop.DBus.Deprecated",
					 "foo");

		TEST_LT (ret, 0);

		TEST_EQ_P (signal->symbol, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, SIGNAL_ILLEGAL_DEPRECATED);
		nih_free (err);

		nih_free (signal);
	}


	/* Check that an invalid symbol in an annotation results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid symbol in annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
		}

		ret = signal_annotation (signal,
					 "com.netsplit.Nih.Symbol",
					 "foo bar");
		TEST_LT (ret, 0);

		TEST_EQ_P (signal->symbol, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, SIGNAL_INVALID_SYMBOL);
		nih_free (err);

		nih_free (signal);
	}


	/* Check that an unknown annotation results in an error being
	 * raised.
	 */
	TEST_FEATURE ("with unknown annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "TestSignal");
		}

		ret = signal_annotation (signal,
					 "com.netsplit.Nih.Unknown",
					 "true");

		TEST_LT (ret, 0);

		err = nih_error_get ();
		TEST_EQ (err->number, SIGNAL_UNKNOWN_ANNOTATION);
		nih_free (err);

		nih_free (signal);
	}
}



void
test_lookup_argument (void)
{
	Signal *  signal = NULL;
	Argument *argument1 = NULL;
	Argument *argument2 = NULL;
	Argument *argument3 = NULL;
	Argument *ret;

	TEST_FUNCTION ("signal_lookup_argument");


	/* Check that the function returns the argument if there is one
	 * with the given symbol.
	 */
	TEST_FEATURE ("with matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "com.netsplit.Nih.Test");

			argument1 = argument_new (signal, "Test",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "test");
			nih_list_add (&signal->arguments, &argument1->entry);

			argument2 = argument_new (signal, "Foo",
						  "s", NIH_DBUS_ARG_IN);
			nih_list_add (&signal->arguments, &argument2->entry);

			argument3 = argument_new (signal, "Bar",
						  "s", NIH_DBUS_ARG_IN);
			argument3->symbol = nih_strdup (argument3, "bar");
			nih_list_add (&signal->arguments, &argument3->entry);
		}

		ret = signal_lookup_argument (signal, "bar");

		TEST_EQ_P (ret, argument3);

		nih_free (signal);
	}


	/* Check that the function returns NULL if there is no argument
	 * with the given symbol.
	 */
	TEST_FEATURE ("with non-matching symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "com.netsplit.Nih.Test");

			argument1 = argument_new (signal, "Test",
						  "s", NIH_DBUS_ARG_IN);
			argument1->symbol = nih_strdup (argument1, "test");
			nih_list_add (&signal->arguments, &argument1->entry);

			argument2 = argument_new (signal, "Foo",
						  "s", NIH_DBUS_ARG_IN);
			nih_list_add (&signal->arguments, &argument2->entry);

			argument3 = argument_new (signal, "Bar",
						  "s", NIH_DBUS_ARG_IN);
			argument3->symbol = nih_strdup (argument3, "bar");
			nih_list_add (&signal->arguments, &argument3->entry);
		}

		ret = signal_lookup_argument (signal, "baz");

		TEST_EQ_P (ret, NULL);

		nih_free (signal);
	}
}


void
test_object_function (void)
{
	pid_t             dbus_pid;
	DBusConnection *  server_conn;
	DBusConnection *  client_conn;
	NihList           prototypes;
	Signal *          signal = NULL;
	Argument *        argument = NULL;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
	NihListEntry *    attrib;
	DBusMessageIter   iter;
	DBusMessage *     sig;
	DBusError         dbus_error;
	int               ret;

	TEST_FUNCTION ("signal_object_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that marhals its arguments
	 * into a D-Bus message and sends it as a signal.
	 */
	TEST_FEATURE ("with signal");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "MySignal");
			signal->symbol = nih_strdup (signal, "my_signal");

			argument = argument_new (signal, "Msg",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "msg");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_object_function (NULL, "com.netsplit.Nih.Test",
					      signal, "my_emit_signal",
					      &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (signal);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_emit_signal (DBusConnection *connection,\n"
				   "                const char *    origin_path,\n"
				   "                const char *    msg)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\tnih_assert (msg != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Test\", \"MySignal\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &msg)) {\n"
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
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_emit_signal");
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
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "msg");
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
		nih_free (signal);
	}


	/* Check that a signal with no arguments can still have
	 * a correctly generated function.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "MySignal");
			signal->symbol = nih_strdup (signal, "my_signal");
		}

		str = signal_object_function (NULL, "com.netsplit.Nih.Test",
					      signal, "my_emit_signal",
					      &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (signal);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_emit_signal (DBusConnection *connection,\n"
				   "                const char *    origin_path)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Test\", \"MySignal\");\n"
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

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_emit_signal");
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

		nih_free (str);
		nih_free (signal);
	}


	/* Check that we can use the generated code to emit a signal and
	 * that we can receive it.
	 */
	TEST_FEATURE ("with signal (generated code)");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (server_conn, "type='signal'", &dbus_error);

		ret = my_emit_signal (client_conn, "/com/netsplit/Nih/Test",
				      "this is a test");

		if (test_alloc_failed
		    && (ret < 0)) {
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_DBUS_MESSAGE (server_conn, sig);
		TEST_EQ (dbus_message_get_type (sig),
			 DBUS_MESSAGE_TYPE_SIGNAL);

		dbus_message_iter_init (sig, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str);
		TEST_EQ_STR (str, "this is a test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (sig);
	}


	/* Check that a deprecated signal does not have the attribute
	 * added, since we want to be able to emit it without a gcc
	 * warning.
	 */
	TEST_FEATURE ("with deprecated signal");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "MySignal");
			signal->symbol = nih_strdup (signal, "my_signal");
			signal->deprecated = TRUE;

			argument = argument_new (signal, "Msg",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "msg");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_object_function (NULL, "com.netsplit.Nih.Test",
					      signal, "my_emit_signal",
					      &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);

			nih_free (signal);
			continue;
		}

		TEST_EQ_STR (str, ("int\n"
				   "my_emit_signal (DBusConnection *connection,\n"
				   "                const char *    origin_path,\n"
				   "                const char *    msg)\n"
				   "{\n"
				   "\tDBusMessage *   signal;\n"
				   "\tDBusMessageIter iter;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (origin_path != NULL);\n"
				   "\tnih_assert (msg != NULL);\n"
				   "\n"
				   "\t/* Construct the message. */\n"
				   "\tsignal = dbus_message_new_signal (origin_path, \"com.netsplit.Nih.Test\", \"MySignal\");\n"
				   "\tif (! signal)\n"
				   "\t\treturn -1;\n"
				   "\n"
				   "\tdbus_message_iter_init_append (signal, &iter);\n"
				   "\n"
				   "\t/* Marshal a char * onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &msg)) {\n"
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
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_emit_signal");
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
		TEST_EQ_STR (arg->type, "const char *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "msg");
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
		nih_free (signal);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int my_signal_handler_called = FALSE;
static NihDBusProxy *last_proxy = NULL;

static void
my_signal_handler (void *          data,
		   NihDBusProxy *  proxy,
		   NihDBusMessage *message,
		   const char *    msg)
{
	my_signal_handler_called++;
	TEST_EQ_P (data, &my_signal_handler_called);

	last_proxy = proxy;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_EQ_P (message->connection, proxy->connection);
	TEST_NE_P (message->message, NULL);

	TEST_ALLOC_PARENT (msg, message);
	TEST_EQ_STR (msg, "this is a test");
}

const NihDBusSignal    my_signal    = { "MySignal", NULL, my_signal_filter };
const NihDBusInterface my_interface = { "com.netsplit.Nih", NULL, NULL, NULL };

void
test_proxy_function (void)
{
	pid_t               dbus_pid;
	DBusConnection *    server_conn;
	DBusConnection *    client_conn;
	NihList             prototypes;
	NihList             typedefs;
	Signal *            signal = NULL;
	Argument *          argument = NULL;
	char *              str;
	TypeFunc *          func;
	TypeVar *           arg;
	NihListEntry *      attrib;
	NihDBusProxy *      proxy = NULL;
	NihDBusProxySignal *proxied = NULL;
	DBusMessage *       sig;
	DBusMessageIter     iter;

	TEST_FUNCTION ("signal_proxy_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a filter function that checks and
	 * demarshals the arguments of a received signal and calls a
	 * handler function for it.
	 */
	TEST_FEATURE ("with signal");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);

		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "MySignal");
			signal->symbol = nih_strdup (signal, "my_signal");

			argument = argument_new (signal, "Msg",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "msg");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_proxy_function (NULL,
					     signal, "my_signal_filter",
					     "MySignalHandler",
					     &prototypes, &typedefs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);

			nih_free (signal);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "my_signal_filter (DBusConnection *    connection,\n"
				   "                  DBusMessage *       signal,\n"
				   "                  NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tchar *          msg;\n"
				   "\tconst char *    msg_dbus;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->proxy->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->proxy->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->proxy->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->proxy->name))\n"
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
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &msg_dbus);\n"
				   "\n"
				   "\tmsg = nih_strdup (message, msg_dbus);\n"
				   "\tif (! msg) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t}\n"
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
				   "\t((MySignalHandler)proxied->handler) (proxied->proxy->data, proxied->proxy, message, msg);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_signal_filter");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MySignalHandler)");
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
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
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
		TEST_EQ_STR (arg->name, "msg");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&typedefs);

		nih_free (str);
		nih_free (signal);
	}


	/* Check that we can still generate a filter function for a signal
	 * with no arguments.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);

		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "MySignal");
			signal->symbol = nih_strdup (signal, "my_signal");
		}

		str = signal_proxy_function (NULL,
					     signal, "my_signal_filter",
					     "MySignalHandler",
					     &prototypes, &typedefs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);

			nih_free (signal);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "my_signal_filter (DBusConnection *    connection,\n"
				   "                  DBusMessage *       signal,\n"
				   "                  NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->proxy->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->proxy->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->proxy->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->proxy->name))\n"
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
				   "\t((MySignalHandler)proxied->handler) (proxied->proxy->data, proxied->proxy, message);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_signal_filter");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MySignalHandler)");
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
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
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
		nih_free (signal);
	}


	/* Check that we can use the generated code to catch a signal
	 * and make a call to the handler with the expected arguments.
	 */
	TEST_FEATURE ("with signal (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, &my_signal_handler_called);

			proxied = nih_dbus_proxy_connect (proxy,
							  &my_interface,
							  &my_signal,
							  (NihDBusSignalHandler)my_signal_handler);
		}

		sig = dbus_message_new_signal ("/com/netsplit/Nih",
					       "com.netsplit.Nih",
					       "MySignal");

		dbus_message_iter_init_append (sig, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		dbus_connection_send (server_conn, sig, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (sig);

		my_signal_handler_called = FALSE;
		last_proxy = NULL;

		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (my_signal_handler_called);
		TEST_EQ_P (last_proxy, proxy);

		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}
	}


	/* Check that a deprecated signal generates a warning since we
	 * don't want people catching them (passing the pointer should be
	 * enough I think).
	 */
	TEST_FEATURE ("with deprecated signal");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);

		TEST_ALLOC_SAFE {
			signal = signal_new (NULL, "MySignal");
			signal->symbol = nih_strdup (signal, "my_signal");
			signal->deprecated = TRUE;

			argument = argument_new (signal, "Msg",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "msg");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_proxy_function (NULL,
					     signal, "my_signal_filter",
					     "MySignalHandler",
					     &prototypes, &typedefs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);

			nih_free (signal);
			continue;
		}

		TEST_EQ_STR (str, ("DBusHandlerResult\n"
				   "my_signal_filter (DBusConnection *    connection,\n"
				   "                  DBusMessage *       signal,\n"
				   "                  NihDBusProxySignal *proxied)\n"
				   "{\n"
				   "\tDBusMessageIter iter;\n"
				   "\tNihDBusMessage *message;\n"
				   "\tchar *          msg;\n"
				   "\tconst char *    msg_dbus;\n"
				   "\n"
				   "\tnih_assert (connection != NULL);\n"
				   "\tnih_assert (signal != NULL);\n"
				   "\tnih_assert (proxied != NULL);\n"
				   "\tnih_assert (connection == proxied->proxy->connection);\n"
				   "\n"
				   "\tif (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (! dbus_message_has_path (signal, proxied->proxy->path))\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\n"
				   "\tif (proxied->proxy->name)\n"
				   "\t\tif (! dbus_message_has_sender (signal, proxied->proxy->name))\n"
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
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&iter, &msg_dbus);\n"
				   "\n"
				   "\tmsg = nih_strdup (message, msg_dbus);\n"
				   "\tif (! msg) {\n"
				   "\t\tnih_free (message);\n"
				   "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				   "\t}\n"
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
				   "\t((MySignalHandler)proxied->handler) (proxied->proxy->data, proxied->proxy, message, msg);\n"
				   "\tnih_error_pop_context ();\n"
				   "\tnih_free (message);\n"
				   "\n"
				   "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_signal_filter");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&typedefs);

		func = (TypeFunc *)typedefs.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "typedef void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "(*MySignalHandler)");
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
		TEST_EQ_STR (arg->type, "NihDBusProxy *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "proxy");
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
		TEST_EQ_STR (arg->name, "msg");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_NOT_EMPTY (&func->attribs);

		attrib = (NihListEntry *)func->attribs.next;
		TEST_ALLOC_SIZE (attrib, sizeof (NihListEntry *));
		TEST_ALLOC_PARENT (attrib, func);
		TEST_EQ_STR (attrib->str, "deprecated");
		TEST_ALLOC_PARENT (attrib->str, attrib);
		nih_free (attrib);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&typedefs);

		nih_free (str);
		nih_free (signal);
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
	test_proxy_function ();

	return 0;
}
