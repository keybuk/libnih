/* nih-dbus-tool
 *
 * test_signal.c - test suite for nih-dbus-tool/signal.c
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
	Node *       node = NULL;
	Interface *  interface = NULL;
	Signal *     signal;
	char *       attr[5];
	int          ret = 0;
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
			nih_discard (interface);
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
			nih_discard (interface);

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
			nih_discard (interface);

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
			nih_discard (interface);

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
			node = node_new (NULL, NULL);
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_NODE, node);
			nih_discard (node);

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
			nih_discard (interface);

			signal = signal_new (NULL, "TestSignal");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_SIGNAL, signal);
			nih_discard (signal);
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
			nih_discard (interface);

			signal = signal_new (NULL, "TestSignal");
			signal->symbol = nih_strdup (signal, "foo");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_SIGNAL, signal);
			nih_discard (signal);
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
			nih_discard (interface);

			other = signal_new (interface, "Test");
			other->symbol = nih_strdup (other, "test_signal");
			nih_list_add (&interface->signals, &other->entry);

			signal = signal_new (NULL, "TestSignal");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_SIGNAL, signal);
			nih_discard (signal);
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
test_lookup (void)
{
	Interface *interface = NULL;
	Signal *   signal1 = NULL;
	Signal *   signal2 = NULL;
	Signal *   signal3 = NULL;
	Signal *   ret;

	TEST_FUNCTION ("signal_lookup");


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

		ret = signal_lookup (interface, "bar");

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

		ret = signal_lookup (interface, "baz");

		TEST_EQ_P (ret, NULL);

		nih_free (interface);
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
	NihList           structs;
	Interface *       interface = NULL;
	Signal *          signal = NULL;
	Argument *        argument = NULL;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
	TypeStruct *      structure;
	TypeVar *         var;
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
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			signal = signal_new (NULL, "Signal");
			signal->symbol = nih_strdup (signal, "signal");

			argument = argument_new (signal, "Msg",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "msg");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_object_function (NULL, "my", interface, signal,
					      &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (signal);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_signal_object_function_standard.c");

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

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (signal);
		nih_free (interface);
	}


	/* Check that a signal with no arguments can still have
	 * a correctly generated function.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			signal = signal_new (NULL, "Signal");
			signal->symbol = nih_strdup (signal, "signal");
		}

		str = signal_object_function (NULL, "my", interface, signal,
					      &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (signal);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_signal_object_function_no_args.c");

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

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (signal);
		nih_free (interface);
	}


	/* Check that a signal with a structure argument is correctly
	 * generated, with the structure type passed back in the
	 * structs array.
	 */
	TEST_FEATURE ("with structure argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			signal = signal_new (NULL, "Signal");
			signal->symbol = nih_strdup (signal, "signal");

			argument = argument_new (signal, "structure",
						 "(su)", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "structure");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_object_function (NULL, "my", interface, signal,
					      &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (signal);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_signal_object_function_structure.c");

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
		TEST_EQ_STR (arg->type, "const MySignalStructure *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "structure");
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
		TEST_EQ_STR (structure->name, "MySignalStructure");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (signal);
		nih_free (interface);
	}


	/* Check that a signal with an array argument can have that argument
	 * as NULL if length is zero.
	 */
	TEST_FEATURE ("with array argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			signal = signal_new (NULL, "Signal");
			signal->symbol = nih_strdup (signal, "signal");

			argument = argument_new (signal, "Value",
						 "ai", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "value");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_object_function (NULL, "my", interface, signal,
					      &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (signal);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_signal_object_function_array.c");

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

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (signal);
		nih_free (interface);
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
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			signal = signal_new (NULL, "Signal");
			signal->symbol = nih_strdup (signal, "signal");
			signal->deprecated = TRUE;

			argument = argument_new (signal, "Msg",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "msg");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_object_function (NULL, "my", interface, signal,
					      &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (signal);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_signal_object_function_deprecated.c");

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

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (signal);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int my_signal_handler_called = FALSE;

static void
my_signal_handler (void *          data,
		   NihDBusMessage *message,
		   const char *    msg)
{
	my_signal_handler_called++;
	TEST_EQ_P (data, &my_signal_handler_called);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_ALLOC_PARENT (msg, message);
	TEST_EQ_STR (msg, "this is a test");
}

const NihDBusSignal my_interface_signals[] = {
	{ "Signal", NULL, my_com_netsplit_Nih_Test_Signal_signal },
	{ NULL }
};
const NihDBusInterface my_interface = {
	"com.netsplit.Nih",
	NULL,
	my_interface_signals,
	NULL
};

void
test_proxy_function (void)
{
	pid_t               dbus_pid;
	DBusConnection *    server_conn;
	DBusConnection *    client_conn;
	DBusConnection *    other_conn;
	NihList             prototypes;
	NihList             typedefs;
	NihList             structs;
	Interface *         interface = NULL;
	Signal *            signal = NULL;
	Argument *          argument = NULL;
	char *              str;
	TypeFunc *          func;
	TypeVar *           arg;
	TypeStruct *        structure;
	TypeVar *           var;
	NihListEntry *      attrib;
	NihDBusProxy *      proxy = NULL;
	NihDBusProxySignal *proxied = NULL;
	DBusMessage *       sig;
	DBusMessageIter     iter;
	DBusError           dbus_error;

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
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			signal = signal_new (NULL, "Signal");
			signal->symbol = nih_strdup (signal, "signal");

			argument = argument_new (signal, "Msg",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "msg");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_proxy_function (NULL, "my", interface, signal,
					     &prototypes, &typedefs,
					     &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (signal);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_signal_proxy_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Signal_signal");
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

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (signal);
		nih_free (interface);
	}


	/* Check that we can still generate a filter function for a signal
	 * with no arguments.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			signal = signal_new (NULL, "Signal");
			signal->symbol = nih_strdup (signal, "signal");
		}

		str = signal_proxy_function (NULL, "my", interface, signal,
					     &prototypes, &typedefs,
					     &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (signal);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_signal_proxy_function_no_args.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Signal_signal");
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
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_EMPTY (&func->args);

		TEST_LIST_EMPTY (&func->attribs);
		nih_free (func);

		TEST_LIST_EMPTY (&typedefs);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (signal);
		nih_free (interface);
	}


	/* Check that a signal with a structure argument is correctly
	 * generated, with the structure type passed back in the structs
	 * array.
	 */
	TEST_FEATURE ("with structure argument");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			signal = signal_new (NULL, "Signal");
			signal->symbol = nih_strdup (signal, "signal");

			argument = argument_new (signal, "structure",
						 "(su)", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "structure");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_proxy_function (NULL, "my", interface, signal,
					     &prototypes, &typedefs,
					     &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (signal);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_signal_proxy_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Signal_signal");
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
		TEST_EQ_STR (arg->type, "NihDBusMessage *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "message");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "const MySignalStructure *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "structure");
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
		TEST_EQ_STR (structure->name, "MySignalStructure");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (signal);
		nih_free (interface);
	}


	/* Check that we can use the generated code to catch a signal
	 * with a peer-to-peer proxy and make a call to the handler with the
	 * expected arguments.
	 */
	TEST_FEATURE ("with signal and peer-to-peer (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    NULL,
						    "/com/netsplit/Nih",
						    NULL, NULL);

			proxied = nih_dbus_proxy_connect (proxy,
							  &my_interface,
							  "Signal",
							  (NihDBusSignalHandler)my_signal_handler,
							  &my_signal_handler_called);
		}

		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		dbus_error_free (&dbus_error);

		sig = dbus_message_new_signal ("/com/netsplit/Nih",
					       "com.netsplit.Nih",
					       "Signal");

		dbus_message_iter_init_append (sig, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		dbus_connection_send (server_conn, sig, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (sig);

		my_signal_handler_called = FALSE;

		TEST_DBUS_DISPATCH (client_conn);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		dbus_error_free (&dbus_error);

		TEST_TRUE (my_signal_handler_called);

		TEST_ALLOC_SAFE {
			nih_free (proxied);
			nih_free (proxy);
		}
	}


	/* Check that we can use the generated code to catch a signal
	 * with a unique name proxy and make a call to the handler with the
	 * expected arguments.
	 */
	TEST_FEATURE ("with signal and unique name (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);

			proxied = nih_dbus_proxy_connect (proxy,
							  &my_interface,
							  "Signal",
							  (NihDBusSignalHandler)my_signal_handler,
							  &my_signal_handler_called);
		}

		sig = dbus_message_new_signal ("/com/netsplit/Nih",
					       "com.netsplit.Nih",
					       "Signal");

		dbus_message_iter_init_append (sig, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		dbus_connection_send (server_conn, sig, NULL);
		dbus_connection_flush (server_conn);
		dbus_message_unref (sig);

		my_signal_handler_called = FALSE;

		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (my_signal_handler_called);

		TEST_ALLOC_SAFE {
			nih_free (proxied);
			nih_free (proxy);
		}
	}


	/* Check that we can use the generated code to catch a signal
	 * with a well-known name name proxy and make a call to the handler
	 * with the expected arguments.
	 */
	TEST_FEATURE ("with signal and well-known name (generated code)");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (other_conn);

		assert (dbus_bus_request_name (other_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    "com.netsplit.Nih",
						    "/com/netsplit/Nih",
						    NULL, NULL);

			proxied = nih_dbus_proxy_connect (proxy,
							  &my_interface,
							  "Signal",
							  (NihDBusSignalHandler)my_signal_handler,
							  &my_signal_handler_called);
		}

		sig = dbus_message_new_signal ("/com/netsplit/Nih",
					       "com.netsplit.Nih",
					       "Signal");

		dbus_message_iter_init_append (sig, &iter);

		str = "this is a test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str);

		dbus_connection_send (other_conn, sig, NULL);
		dbus_connection_flush (other_conn);
		dbus_message_unref (sig);

		my_signal_handler_called = FALSE;

		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (my_signal_handler_called);

		TEST_ALLOC_SAFE {
			nih_free (proxied);
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (other_conn);
	}


	/* Check that a deprecated signal generates a warning since we
	 * don't want people catching them (passing the pointer should be
	 * enough I think).
	 */
	TEST_FEATURE ("with deprecated signal");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			signal = signal_new (NULL, "Signal");
			signal->symbol = nih_strdup (signal, "signal");
			signal->deprecated = TRUE;

			argument = argument_new (signal, "Msg",
						 "s", NIH_DBUS_ARG_OUT);
			argument->symbol = nih_strdup (argument, "msg");
			nih_list_add (&signal->arguments, &argument->entry);
		}

		str = signal_proxy_function (NULL, "my", interface, signal,
					     &prototypes, &typedefs,
					     &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&typedefs);
			TEST_LIST_EMPTY (&structs);

			nih_free (signal);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_signal_proxy_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusHandlerResult");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_Signal_signal");
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

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (signal);
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
	Signal *   signal = NULL;
	Argument * arg1 = NULL;
	Argument * arg2 = NULL;
	Argument * arg3 = NULL;
	char *     str;
	TypeVar *  var;


	TEST_FUNCTION ("signal_args_array");


	/* Check that we can generate an array of argument definitions for
	 * a signal, with each name and type lined up with each other and
	 * the final part lined up too.  Arguments without names should have
	 * NULL in place of the name.
	 */
	TEST_FEATURE ("with arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			signal = signal_new (interface, "Signal");
			signal->symbol = "signal";
			nih_list_add (&interface->signals, &signal->entry);

			arg1 = argument_new (signal, "foo",
					     "as", NIH_DBUS_ARG_OUT);
			arg1->symbol = "foo";
			nih_list_add (&signal->arguments, &arg1->entry);

			arg2 = argument_new (signal, "wibble",
					     "i", NIH_DBUS_ARG_OUT);
			arg2->symbol = "wibble";
			nih_list_add (&signal->arguments, &arg2->entry);

			arg3 = argument_new (signal, NULL,
					     "a(iii)", NIH_DBUS_ARG_OUT);
			arg3->symbol = "arg3";
			nih_list_add (&signal->arguments, &arg3->entry);
		}

		str = signal_args_array (NULL, "my", interface, signal,
					 &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusArg my_com_netsplit_Nih_Test_Signal_signal_args[] = {\n"
			     "\t{ \"foo\",    \"as\",     NIH_DBUS_ARG_OUT },\n"
			     "\t{ \"wibble\", \"i\",      NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL,     \"a(iii)\", NIH_DBUS_ARG_OUT },\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusArg");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_Signal_signal_args");
		TEST_ALLOC_PARENT (var->name, var);
		TEST_TRUE (var->array);
		nih_free (var);

		TEST_LIST_EMPTY (&prototypes);

		nih_free (str);
		nih_free (interface);
	}


	/* Check that a signal with no arguments has an empty array
	 * returned.
	 */
	TEST_FEATURE ("with no arguments");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = "test";

			signal = signal_new (interface, "Signal");
			signal->symbol = "signal";
			nih_list_add (&interface->signals, &signal->entry);
		}

		str = signal_args_array (NULL, "my", interface, signal,
					 &prototypes);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&prototypes);

			nih_free (interface);
			continue;
		}

		TEST_EQ_STR (str,
			     "const NihDBusArg my_com_netsplit_Nih_Test_Signal_signal_args[] = {\n"
			     "\t{ NULL }\n"
			     "};\n");

		TEST_LIST_NOT_EMPTY (&prototypes);

		var = (TypeVar *)prototypes.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const NihDBusArg");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "my_com_netsplit_Nih_Test_Signal_signal_args");
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
	test_proxy_function ();

	test_args_array ();

	return 0;
}
