/* nih-dbus-tool
 *
 * test_signal.c - test suite for nih-dbus-tool/signal.c
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
#include "signal.h"
#include "argument.h"
#include "parse.h"
#include "errors.h"


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
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

	attr[0] = NULL;

	ret = signal_start_tag (xmlp, "signal", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->signals);

	err = nih_error_get ();
	TEST_EQ (err->number, SIGNAL_MISSING_NAME);
	nih_free (err);

	nih_free (parent);


	/* Check that a signal with an invalid name results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid name");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

	attr[0] = "name";
	attr[1] = "Test Signal";
	attr[2] = NULL;

	ret = signal_start_tag (xmlp, "signal", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->signals);

	err = nih_error_get ();
	TEST_EQ (err->number, SIGNAL_INVALID_NAME);
	nih_free (err);

	nih_free (parent);


	/* Check that an unknown signal attribute results in a warning
	 * being printed to standard error, but is otherwise ignored
	 * and the normal processing finished.
	 */
	TEST_FEATURE ("with unknown attribute");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

	attr[0] = "name";
	attr[1] = "TestSignal";
	attr[2] = "frodo";
	attr[3] = "baggins";
	attr[4] = NULL;

	TEST_DIVERT_STDERR (output) {
		ret = signal_start_tag (xmlp, "signal", attr);
	}
	rewind (output);

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


	/* Check that a signal on an empty stack (ie. a top-level
	 * signal element) results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with empty stack");
	attr[0] = "name";
	attr[1] = "TestSignal";
	attr[2] = NULL;

	TEST_DIVERT_STDERR (output) {
		ret = signal_start_tag (xmlp, "signal", attr);
	}
	rewind (output);

	TEST_EQ (ret, 0);

	entry = parse_stack_top (&context.stack);
	TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
	TEST_EQ (entry->type, PARSE_IGNORED);
	TEST_EQ_P (entry->data, NULL);

	TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <signal> tag\n");
	TEST_FILE_END (output);
	TEST_FILE_RESET (output);

	nih_free (entry);


	/* Check that a signal on top of a stack entry that's not an
	 * interface results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with non-interface on stack");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_NODE, node_new (NULL, NULL));

	attr[0] = "name";
	attr[1] = "TestSignal";
	attr[2] = NULL;

	TEST_DIVERT_STDERR (output) {
		ret = signal_start_tag (xmlp, "signal", attr);
	}
	rewind (output);

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
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

	other = signal_new (interface, "Test");
	other->symbol = nih_strdup (other, "test_signal");
	nih_list_add (&interface->signals, &other->entry);

	signal = signal_new (NULL, "TestSignal");
	entry = parse_stack_push (NULL, &context.stack,
				  PARSE_SIGNAL, signal);

	ret = signal_end_tag (xmlp, "signal");

	TEST_LT (ret, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, SIGNAL_DUPLICATE_SYMBOL);
	nih_free (err);

	nih_free (entry);
	nih_free (parent);


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
	signal = signal_new (NULL, "TestSignal");

	ret = signal_annotation (signal,
				 "org.freedesktop.DBus.Deprecated",
				 "foo");

	TEST_LT (ret, 0);

	TEST_EQ_P (signal->symbol, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, SIGNAL_ILLEGAL_DEPRECATED);
	nih_free (err);

	nih_free (signal);


	/* Check that an invalid symbol in an annotation results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid symbol in annotation");
	signal = signal_new (NULL, "TestSignal");

	ret = signal_annotation (signal,
				 "com.netsplit.Nih.Symbol",
				 "foo bar");
	TEST_LT (ret, 0);

	TEST_EQ_P (signal->symbol, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, SIGNAL_INVALID_SYMBOL);
	nih_free (err);

	nih_free (signal);


	/* Check that an unknown annotation results in an error being
	 * raised.
	 */
	TEST_FEATURE ("with unknown annotation");
	signal = signal_new (NULL, "TestSignal");

	ret = signal_annotation (signal,
				 "com.netsplit.Nih.Unknown",
				 "true");

	TEST_LT (ret, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, SIGNAL_UNKNOWN_ANNOTATION);
	nih_free (err);

	nih_free (signal);
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
