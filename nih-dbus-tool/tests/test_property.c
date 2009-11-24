/* nih-dbus-tool
 *
 * test_property.c - test suite for nih-dbus-tool/property.c
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
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/main.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>
#include <nih-dbus/dbus_message.h>
#include <nih-dbus/errors.h>

#include "type.h"
#include "node.h"
#include "property.h"
#include "parse.h"
#include "errors.h"
#include "tests/property_code.h"


void
test_name_valid (void)
{
	TEST_FUNCTION ("property_name_valid");

	/* Check that a typical property name is valid. */
	TEST_FEATURE ("with typical property name");
	TEST_TRUE (property_name_valid ("Wibble"));


	/* Check that an property name is not valid if it is has an
	 * initial period.
	 */
	TEST_FEATURE ("with initial period");
	TEST_FALSE (property_name_valid (".Wibble"));


	/* Check that an property name is not valid if it ends with a period
	 */
	TEST_FEATURE ("with final period");
	TEST_FALSE (property_name_valid ("Wibble."));


	/* Check that an property name is not valid if it contains a period
	 */
	TEST_FEATURE ("with period");
	TEST_FALSE (property_name_valid ("Wib.ble"));


	/* Check that a property name may contain numbers */
	TEST_FEATURE ("with numbers");
	TEST_TRUE (property_name_valid ("Wib43ble"));


	/* Check that a property name may not begin with numbers */
	TEST_FEATURE ("with leading digits");
	TEST_FALSE (property_name_valid ("43Wibble"));


	/* Check that a property name may end with numbers */
	TEST_FEATURE ("with trailing digits");
	TEST_TRUE (property_name_valid ("Wibble43"));


	/* Check that a property name may contain underscores */
	TEST_FEATURE ("with underscore");
	TEST_TRUE (property_name_valid ("Wib_ble"));


	/* Check that a property name may begin with underscores */
	TEST_FEATURE ("with initial underscore");
	TEST_TRUE (property_name_valid ("_Wibble"));


	/* Check that a property name may end with underscores */
	TEST_FEATURE ("with final underscore");
	TEST_TRUE (property_name_valid ("Wibble_"));


	/* Check that other characters are not permitted */
	TEST_FEATURE ("with non-permitted characters");
	TEST_FALSE (property_name_valid ("Wib-ble"));


	/* Check that an empty property name is invalid */
	TEST_FEATURE ("with empty string");
	TEST_FALSE (property_name_valid (""));


	/* Check that an property name may not exceed 255 characters */
	TEST_FEATURE ("with overly long name");
	TEST_FALSE (property_name_valid ("ReallyLongPropertyNameThatNobo"
					 "dyInTheirRightMindWouldEverUse"
					 "NotInTheLeastBecauseThenYoudEn"
					 "dUpWithAnEvenLongerInterfaceNa"
					 "meAndThatJustWontWorkWhenCombi"
					 "nedButStillWeTestThisShitJustI"
					 "ncaseSomeoneTriesItBecauseThat"
					 "sWhatTestDrivenDevelopmentIsAl"
					 "lAboutYayDoneNow"));
}


void
test_new (void)
{
	Property *property;

	/* Check that an Property object is allocated with the structure
	 * filled in properly, but not placed in a list.
	 */
	TEST_FUNCTION ("property_new");
	TEST_ALLOC_FAIL {
		property = property_new (NULL, "Size", "i", NIH_DBUS_READ);

		if (test_alloc_failed) {
			TEST_EQ_P (property, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_LIST_EMPTY (&property->entry);
		TEST_EQ_STR (property->name, "Size");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_STR (property->type, "i");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_EQ_P (property->symbol, NULL);
		TEST_EQ (property->access, NIH_DBUS_READ);
		TEST_FALSE (property->deprecated);

		nih_free (property);
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
	Property *   property;
	char *       attr[9];
	int          ret;
	NihError *   err;
	FILE *       output;

	TEST_FUNCTION ("property_start_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);

	output = tmpfile ();


	/* Check that an property tag for an interface with the usual name,
	 * and type attributes and with an access attribute of read results
	 * in an Property member being created and pushed onto the stack
	 * with the attributes filled in correctly for a read-only property.
	 */
	TEST_FEATURE ("with read-only property");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		attr[0] = "name";
		attr[1] = "TestProperty";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = "access";
		attr[5] = "read";
		attr[6] = NULL;

		ret = property_start_tag (xmlp, "property", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&interface->properties);

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
		TEST_EQ (entry->type, PARSE_PROPERTY);

		property = entry->property;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, entry);
		TEST_EQ_STR (property->name, "TestProperty");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_P (property->symbol, NULL);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_EQ (property->access, NIH_DBUS_READ);

		TEST_LIST_EMPTY (&interface->properties);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an property tag for an interface with the usual name,
	 * and type attributes and with an access attribute of write results
	 * in an Property member being created and pushed onto the stack
	 * with the attributes filled in correctly for a write-only property.
	 */
	TEST_FEATURE ("with write-only property");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		attr[0] = "name";
		attr[1] = "TestProperty";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = "access";
		attr[5] = "write";
		attr[6] = NULL;

		ret = property_start_tag (xmlp, "property", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&interface->properties);

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
		TEST_EQ (entry->type, PARSE_PROPERTY);

		property = entry->property;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, entry);
		TEST_EQ_STR (property->name, "TestProperty");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_P (property->symbol, NULL);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_EQ (property->access, NIH_DBUS_WRITE);

		TEST_LIST_EMPTY (&interface->properties);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that an property tag for an interface with the usual name,
	 * and type attributes and with an access attribute of readwrite
	 * results in an Property member being created and pushed onto the
	 * stack with the attributes filled in correctly for a read/write
	 * property.
	 */
	TEST_FEATURE ("with read/write property");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);
		}

		attr[0] = "name";
		attr[1] = "TestProperty";
		attr[2] = "type";
		attr[3] = "s";
		attr[4] = "access";
		attr[5] = "readwrite";
		attr[6] = NULL;

		ret = property_start_tag (xmlp, "property", attr);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_EQ_P (parse_stack_top (&context.stack),
				   parent);

			TEST_LIST_EMPTY (&interface->properties);

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
		TEST_EQ (entry->type, PARSE_PROPERTY);

		property = entry->property;
		TEST_ALLOC_SIZE (property, sizeof (Property));
		TEST_ALLOC_PARENT (property, entry);
		TEST_EQ_STR (property->name, "TestProperty");
		TEST_ALLOC_PARENT (property->name, property);
		TEST_EQ_P (property->symbol, NULL);
		TEST_EQ_STR (property->type, "s");
		TEST_ALLOC_PARENT (property->type, property);
		TEST_EQ (property->access, NIH_DBUS_READWRITE);

		TEST_LIST_EMPTY (&interface->properties);

		nih_free (entry);
		nih_free (parent);
	}


	/* Check that a property with a missing name attribute results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with missing name");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			attr[0] = "type";
			attr[1] = "s";
			attr[2] = "access";
			attr[3] = "read";
			attr[4] = NULL;
		}

		ret = property_start_tag (xmlp, "property", attr);

		TEST_LT (ret, 0);

		TEST_EQ_P (parse_stack_top (&context.stack), parent);

		TEST_LIST_EMPTY (&interface->properties);

		err = nih_error_get ();
		TEST_EQ (err->number, PROPERTY_MISSING_NAME);
		nih_free (err);

		nih_free (parent);
	}


	/* Check that a property with an invalid name results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid name");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);
	nih_discard (interface);

	attr[0] = "name";
	attr[1] = "Test Property";
	attr[2] = "type";
	attr[3] = "s";
	attr[4] = "access";
	attr[5] = "readwrite";
	attr[6] = NULL;

	ret = property_start_tag (xmlp, "property", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->properties);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_INVALID_NAME);
	nih_free (err);

	nih_free (parent);


	/* Check that a property with a missing type attribute results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with missing type");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);
	nih_discard (interface);

	attr[0] = "name";
	attr[1] = "TestProperty";
	attr[2] = "access";
	attr[3] = "read";
	attr[4] = NULL;

	ret = property_start_tag (xmlp, "property", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->properties);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_MISSING_TYPE);
	nih_free (err);

	nih_free (parent);


	/* Check that a property with an invalid type results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid type");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);
	nih_discard (interface);

	attr[0] = "name";
	attr[1] = "TestProperty";
	attr[2] = "type";
	attr[3] = "si";
	attr[4] = "access";
	attr[5] = "readwrite";
	attr[6] = NULL;

	ret = property_start_tag (xmlp, "property", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->properties);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_INVALID_TYPE);
	nih_free (err);

	nih_free (parent);


	/* Check that a property with a missing access attribute results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with missing access");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);
	nih_discard (interface);

	attr[0] = "name";
	attr[1] = "TestProperty";
	attr[2] = "type";
	attr[3] = "s";
	attr[4] = NULL;

	ret = property_start_tag (xmlp, "property", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->properties);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_MISSING_ACCESS);
	nih_free (err);

	nih_free (parent);


	/* Check that a property with an invalid access results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid access");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);
	nih_discard (interface);

	attr[0] = "name";
	attr[1] = "TestProperty";
	attr[2] = "type";
	attr[3] = "s";
	attr[4] = "access";
	attr[5] = "sideways";
	attr[6] = NULL;

	ret = property_start_tag (xmlp, "property", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->properties);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_ILLEGAL_ACCESS);
	nih_free (err);

	nih_free (parent);


	/* Check that an unknown property attribute results in a warning
	 * being printed to standard error, but is otherwise ignored
	 * and the normal processing finished.
	 */
	TEST_FEATURE ("with unknown attribute");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);
	nih_discard (interface);

	attr[0] = "name";
	attr[1] = "TestProperty";
	attr[2] = "type";
	attr[3] = "s";
	attr[4] = "access";
	attr[5] = "read";
	attr[6] = "frodo";
	attr[7] = "baggins";
	attr[8] = NULL;

	TEST_DIVERT_STDERR (output) {
		ret = property_start_tag (xmlp, "property", attr);
	}
	rewind (output);

	TEST_EQ (ret, 0);

	entry = parse_stack_top (&context.stack);
	TEST_NE_P (entry, parent);
	TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
	TEST_EQ (entry->type, PARSE_PROPERTY);

	property = entry->property;
	TEST_ALLOC_SIZE (property, sizeof (Property));
	TEST_ALLOC_PARENT (property, entry);
	TEST_EQ_STR (property->name, "TestProperty");
	TEST_ALLOC_PARENT (property->name, property);
	TEST_EQ_P (property->symbol, NULL);

	TEST_LIST_EMPTY (&interface->properties);

	TEST_FILE_EQ (output, ("test:foo:1:0: Ignored unknown <property> attribute: "
			       "frodo\n"));
	TEST_FILE_END (output);
	TEST_FILE_RESET (output);

	nih_free (entry);
	nih_free (parent);


	/* Check that a property on an empty stack (ie. a top-level
	 * property element) results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with empty stack");
	attr[0] = "name";
	attr[1] = "TestProperty";
	attr[2] = "type";
	attr[3] = "s";
	attr[4] = "access";
	attr[5] = "read";
	attr[6] = NULL;

	TEST_DIVERT_STDERR (output) {
		ret = property_start_tag (xmlp, "property", attr);
	}
	rewind (output);

	TEST_EQ (ret, 0);

	entry = parse_stack_top (&context.stack);
	TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
	TEST_EQ (entry->type, PARSE_IGNORED);
	TEST_EQ_P (entry->data, NULL);

	TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <property> tag\n");
	TEST_FILE_END (output);
	TEST_FILE_RESET (output);

	nih_free (entry);


	/* Check that a property on top of a stack entry that's not an
	 * interface results in a warning being printed on
	 * standard error and an ignored element being pushed onto the
	 * stack.
	 */
	TEST_FEATURE ("with non-interface on stack");
	node = node_new (NULL, NULL);
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_NODE, node);
	nih_discard (node);

	attr[0] = "name";
	attr[1] = "TestProperty";
	attr[2] = "type";
	attr[3] = "s";
	attr[4] = "access";
	attr[5] = "read";
	attr[6] = NULL;

	TEST_DIVERT_STDERR (output) {
		ret = property_start_tag (xmlp, "property", attr);
	}
	rewind (output);

	TEST_EQ (ret, 0);

	entry = parse_stack_top (&context.stack);
	TEST_NE_P (entry, parent);
	TEST_ALLOC_SIZE (entry, sizeof (ParseStack));
	TEST_EQ (entry->type, PARSE_IGNORED);
	TEST_EQ_P (entry->data, NULL);

	TEST_FILE_EQ (output, "test:foo:1:0: Ignored unexpected <property> tag\n");
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
	Property *   property = NULL;
	Property *   other = NULL;
	int          ret;
	NihError *   err;

	TEST_FUNCTION ("property_end_tag");
	context.parent = NULL;
	nih_list_init (&context.stack);
	context.filename = "foo";
	context.node = NULL;

	assert (xmlp = XML_ParserCreate ("UTF-8"));
	XML_SetUserData (xmlp, &context);


	/* Check that when we parse the end tag for a property, we pop
	 * the Property object off the stack (freeing and removing it)
	 * and append it to the parent interface's properties list, adding a
	 * reference to the interface as well.  A symbol should be generated
	 * for the property by convering its name to C style.
	 */
	TEST_FEATURE ("with no assigned symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			parent = parse_stack_push (NULL, &context.stack,
						   PARSE_INTERFACE, interface);
			nih_discard (interface);

			property = property_new (NULL, "TestProperty", "s",
						 NIH_DBUS_READ);
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_PROPERTY, property);
			nih_discard (property);
		}

		TEST_FREE_TAG (entry);

		ret = property_end_tag (xmlp, "property");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->properties);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (property, interface);

		TEST_LIST_NOT_EMPTY (&interface->properties);
		TEST_EQ_P (interface->properties.next, &property->entry);

		TEST_EQ_STR (property->symbol, "test_property");
		TEST_ALLOC_PARENT (property->symbol, property);

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

			property = property_new (NULL, "TestProperty",
						 "s", NIH_DBUS_READ);
			property->symbol = nih_strdup (property, "foo");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_PROPERTY, property);
			nih_discard (property);
		}

		TEST_FREE_TAG (entry);

		ret = property_end_tag (xmlp, "property");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_NOT_FREE (entry);
			TEST_LIST_EMPTY (&interface->properties);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (entry);
			nih_free (parent);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (entry);
		TEST_ALLOC_PARENT (property, interface);

		TEST_LIST_NOT_EMPTY (&interface->properties);
		TEST_EQ_P (interface->properties.next, &property->entry);

		TEST_EQ_STR (property->symbol, "foo");
		TEST_ALLOC_PARENT (property->symbol, property);

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
	nih_discard (interface);

	other = property_new (interface, "Test", "s", NIH_DBUS_READ);
	other->symbol = nih_strdup (other, "test_property");
	nih_list_add (&interface->properties, &other->entry);

	property = property_new (NULL, "TestProperty", "s", NIH_DBUS_READ);
	entry = parse_stack_push (NULL, &context.stack,
				  PARSE_PROPERTY, property);
	nih_discard (property);

	ret = property_end_tag (xmlp, "property");

	TEST_LT (ret, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_DUPLICATE_SYMBOL);
	nih_free (err);

	nih_free (entry);
	nih_free (parent);


	XML_ParserFree (xmlp);
}


void
test_annotation (void)
{
	Property *property = NULL;
	char *    symbol;
	int       ret;
	NihError *err;

	TEST_FUNCTION ("property_annotation");


	/* Check that the annotation to mark a property as deprecated is
	 * handled, and the Property is marked deprecated.
	 */
	TEST_FEATURE ("with deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			property = property_new (NULL, "TestProperty", "s",
						 NIH_DBUS_READ);
		}

		ret = property_annotation (property,
					   "org.freedesktop.DBus.Deprecated",
					   "true");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_FALSE (property->deprecated);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (property);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_TRUE (property->deprecated);

		nih_free (property);
	}


	/* Check that the annotation to mark a property as deprecated can be
	 * given a false value to explicitly mark the Property non-deprecated.
	 */
	TEST_FEATURE ("with explicitly non-deprecated annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			property = property_new (NULL, "TestProperty", "s",
						 NIH_DBUS_READ);
			property->deprecated = TRUE;
		}

		ret = property_annotation (property,
					   "org.freedesktop.DBus.Deprecated",
					   "false");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			TEST_TRUE (property->deprecated);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (property);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FALSE (property->deprecated);

		nih_free (property);
	}


	/* Check that an annotation to add a symbol to the property is
	 * handled, and the new symbol is stored in the property.
	 */
	TEST_FEATURE ("with symbol annotation");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			property = property_new (NULL, "TestProperty", "s",
						 NIH_DBUS_READ);
		}

		ret = property_annotation (property,
					   "com.netsplit.Nih.Symbol",
					   "foo");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (property);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (property->symbol, "foo");
		TEST_ALLOC_PARENT (property->symbol, property);

		nih_free (property);
	}


	/* Check that an annotation to add a symbol to the property
	 * replaces any previous symbol applied (e.g. by a previous
	 * annotation).
	 */
	TEST_FEATURE ("with symbol annotation and existing symbol");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			property = property_new (NULL, "TestProperty", "s",
						 NIH_DBUS_READ);
			property->symbol = nih_strdup (property, "test_arg");
		}

		symbol = property->symbol;
		TEST_FREE_TAG (symbol);

		ret = property_annotation (property,
					   "com.netsplit.Nih.Symbol",
					   "foo");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (property);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_FREE (symbol);

		TEST_EQ_STR (property->symbol, "foo");
		TEST_ALLOC_PARENT (property->symbol, property);

		nih_free (property);
	}


	/* Check that an invalid value for the deprecated annotation results
	 * in an error being raised.
	 */
	TEST_FEATURE ("with invalid value for deprecated annotation");
	property = property_new (NULL, "TestProperty", "s", NIH_DBUS_READ);

	ret = property_annotation (property,
				   "org.freedesktop.DBus.Deprecated",
				   "foo");

	TEST_LT (ret, 0);

	TEST_EQ_P (property->symbol, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_ILLEGAL_DEPRECATED);
	nih_free (err);

	nih_free (property);


	/* Check that an invalid symbol in an annotation results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid symbol in annotation");
	property = property_new (NULL, "TestProperty", "s", NIH_DBUS_READ);

	ret = property_annotation (property,
				   "com.netsplit.Nih.Symbol",
				   "foo bar");

	TEST_LT (ret, 0);

	TEST_EQ_P (property->symbol, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_INVALID_SYMBOL);
	nih_free (err);

	nih_free (property);


	/* Check that an unknown annotation results in an error being
	 * raised.
	 */
	TEST_FEATURE ("with unknown annotation");
	property = property_new (NULL, "TestProperty", "s", NIH_DBUS_READ);

	ret = property_annotation (property,
				   "com.netsplit.Nih.Unknown",
				   "true");

	TEST_LT (ret, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_UNKNOWN_ANNOTATION);
	nih_free (err);

	nih_free (property);
}


void
test_lookup (void)
{
	Interface *interface = NULL;
	Property * property1 = NULL;
	Property * property2 = NULL;
	Property * property3 = NULL;
	Property * ret;

	TEST_FUNCTION ("property_lookup");


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

		ret = property_lookup (interface, "bar");

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

		ret = property_lookup (interface, "baz");

		TEST_EQ_P (ret, NULL);

		nih_free (interface);
	}
}


static int my_get_property_called = 0;
static const char *property_value;

int
my_get_property (void *          data,
		 NihDBusMessage *message,
		 char **         str)
{
	my_get_property_called++;

	TEST_EQ_P (data, NULL);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (str, NULL);

	if (! strcmp (property_value, "felch and firkin")) {
		nih_dbus_error_raise ("com.netsplit.Nih.MyProperty.Fail",
				      "Bad value for property");
		return -1;

	} else if (! strcmp (property_value, "fruitbat and ball")) {
		nih_error_raise (EBADF, strerror (EBADF));
		return -1;

	} else {
		*str = nih_strdup (message, property_value);
		if (! *str)
			nih_return_no_memory_error (-1);
	}

	return 0;
}

void
test_object_get_function (void)
{
	pid_t             dbus_pid;
	DBusConnection *  server_conn;
	DBusConnection *  client_conn;
	NihList           prototypes;
	NihList           handlers;
	NihList           structs;
	Interface *       interface = NULL;
	Property *        property = NULL;
	char *            iface;
	char *            name;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
	TypeStruct *      structure;
	TypeVar *         var;
	NihListEntry *    attrib;
	DBusMessage *     method_call;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	DBusMessage *     reply;
	NihDBusMessage *  message = NULL;
	NihDBusObject *   object = NULL;
	dbus_uint32_t     serial;
	int               ret;
	NihError *        err;
	NihDBusError *    dbus_err;

	TEST_FUNCTION ("property_object_get_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that marshals a value
	 * obtained by calling a property handler function into a variant
	 * appended to the message iterator passed.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_object_get_function (NULL, "my", interface, property,
						    &prototypes, &handlers,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_object_get_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_get");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_property");
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

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can generate a function that marshals a structure
	 * value obtained by calling a property handler function, with
	 * the structure type passed back in the structs array.
	 */
	TEST_FEATURE ("with structure property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "(su)", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_object_get_function (NULL, "my", interface, property,
						    &prototypes, &handlers,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_object_get_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_get");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_property");
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
		TEST_EQ_STR (arg->type, "MyProperty **");
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

		TEST_LIST_EMPTY (&handlers);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyProperty");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to get the value of a
	 * property for a reply we're generating.  The handler function
	 * should be called and the value appended to our message inside
	 * a variant.
	 */
	TEST_FEATURE ("with property (generated code)");
	TEST_ALLOC_FAIL {
		property_value = "dog and doughnut";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

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

		reply = dbus_message_new_method_return (method_call);

		dbus_message_iter_init_append (reply, &iter);

		my_get_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_get (object, message, &iter);

		if (test_alloc_failed
		    && (ret < 0)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			dbus_message_unref (reply);
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_get_property_called);
		TEST_EQ (ret, 0);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str);
		TEST_EQ_STR (str, "dog and doughnut");

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


	/* Check that our function can return a D-Bus error and the
	 * generated code simply returns that upwards.
	 */
	TEST_FEATURE ("with D-Bus error from handler (generated code)");
	TEST_ALLOC_FAIL {
		property_value = "felch and firkin";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

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

		reply = dbus_message_new_method_return (method_call);

		dbus_message_iter_init_append (reply, &iter);

		my_get_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_get (object, message, &iter);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			dbus_message_unref (reply);
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_get_property_called);

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.MyProperty.Fail");
		TEST_EQ_STR (err->message, "Bad value for property");
		nih_free (err);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that our function can return a generic error and the
	 * generated code simply returns that upwards.
	 */
	TEST_FEATURE ("with generic error from handler (generated code)");
	TEST_ALLOC_FAIL {
		property_value = "fruitbat and ball";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

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

		reply = dbus_message_new_method_return (method_call);

		dbus_message_iter_init_append (reply, &iter);

		my_get_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_get (object, message, &iter);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			dbus_message_unref (reply);
			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_get_property_called);

		TEST_EQ (err->number, EBADF);
		TEST_EQ_STR (err->message, strerror (EBADF));
		nih_free (err);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (reply);
		dbus_message_unref (method_call);
	}


	/* Check that when we generate a function for a deprecated
	 * property, we don't include the attribute since we don't
	 * want gcc warnings when implementing an object.
	 */
	TEST_FEATURE ("with deprecated property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
			property->deprecated = TRUE;
		}

		str = property_object_get_function (NULL, "my", interface, property,
						    &prototypes, &handlers,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_object_get_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_get");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_property");
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

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int my_set_property_called = 0;

int
my_set_property (void *          data,
		 NihDBusMessage *message,
		 const char *    str)
{
	nih_local char *dup = NULL;

	my_set_property_called++;

	TEST_EQ_P (data, NULL);

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_ALLOC_PARENT (str, message);

	if (! strcmp (str, "dog and doughnut")) {
		dup = nih_strdup (NULL, str);
		if (! dup)
			nih_return_no_memory_error (-1);

		return 0;

	} else if (! strcmp (str, "felch and firkin")) {
		nih_dbus_error_raise ("com.netsplit.Nih.MyProperty.Fail",
				      "Bad value for property");
		return -1;

	} else if (! strcmp (str, "fruitbat and ball")) {
		nih_error_raise (EBADF, strerror (EBADF));
		return -1;
	}

	return 0;
}

void
test_object_set_function (void)
{
	pid_t             dbus_pid;
	DBusConnection *  server_conn;
	DBusConnection *  client_conn;
	NihList           prototypes;
	NihList           handlers;
	NihList           structs;
	Interface *       interface = NULL;
	Property *        property = NULL;
	char *            iface;
	char *            name;
	char *            str;
	TypeFunc *        func;
	TypeVar *         arg;
	TypeStruct *      structure;
	TypeVar *         var;
	NihListEntry *    attrib;
	double            double_arg;
	DBusMessage *     method_call;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	NihDBusMessage *  message = NULL;
	NihDBusObject *   object = NULL;
	dbus_uint32_t     serial;
	int               ret;
	NihError *        err;
	NihDBusError *    dbus_err;

	TEST_FUNCTION ("property_object_set_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that demarshals a value
	 * from a variant in the passed message iterator, calls a handler
	 * function to set that property and returns to indicate success
	 * or error.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_object_set_function (NULL, "my", interface, property,
						    &prototypes, &handlers,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_object_set_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_set");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_property");
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

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to demarshal the
	 * property value from inside the variant in the method call,
	 * passing it to the handler function.
	 */
	TEST_FEATURE ("with property (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str = "dog and doughnut";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str);

		dbus_message_iter_close_container (&iter, &subiter);

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

		dbus_message_iter_init (method_call, &iter);

		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_VARIANT);

		my_set_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_set (object, message, &iter);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_set_property_called);
		TEST_EQ (ret, 0);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (method_call);
	}


	/* Check that we can generate a function that demarshals a
	 * structure value from a variant in the passed message iterator,
	 * calling a handler function to set that property, with the
	 * structure type passed back in the structs array.
	 */
	TEST_FEATURE ("with structure property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "(su)", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_object_set_function (NULL, "my", interface, property,
						    &prototypes, &handlers,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_object_set_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_set");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_property");
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
		TEST_EQ_STR (arg->type, "const MyProperty *");
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

		TEST_LIST_EMPTY (&handlers);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyProperty");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that if the handler raises a D-Bus error, it is returned
	 * to the caller.
	 */
	TEST_FEATURE ("with D-Bus error from handler (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str = "felch and firkin";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str);

		dbus_message_iter_close_container (&iter, &subiter);

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

		dbus_message_iter_init (method_call, &iter);

		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_VARIANT);

		my_set_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_set (object, message, &iter);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_set_property_called);

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.MyProperty.Fail");
		nih_free (err);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (method_call);
	}


	/* Check that if the handler raises a generic error, it is returned
	 * to the caller.
	 */
	TEST_FEATURE ("with generic error from handler (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str = "fruitbat and ball";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str);

		dbus_message_iter_close_container (&iter, &subiter);

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

		dbus_message_iter_init (method_call, &iter);

		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_VARIANT);

		my_set_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_set (object, message, &iter);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_TRUE (my_set_property_called);

		TEST_EQ (err->number, EBADF);
		nih_free (err);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (method_call);
	}


	/* Check that a missing argument to the property method call
	 * results in an invalid args error message being returned
	 * without the handler being called.
	 */
	TEST_FEATURE ("with missing argument to method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

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

		dbus_message_iter_init (method_call, &iter);

		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);

		my_set_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_set (object, message, &iter);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_set_property_called);

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_INVALID_ARGS);
		nih_free (err);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (method_call);
	}


	/* Check that a non-variant type in the property method call
	 * results in an invalid args error message being returned
	 * without the handler being called.
	 */
	TEST_FEATURE ("with invalid argument in method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

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

		dbus_message_iter_init (method_call, &iter);

		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);

		my_set_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_set (object, message, &iter);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_set_property_called);

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_INVALID_ARGS);
		nih_free (err);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (method_call);
	}


	/* Check that the wrong type in the variant in the property method call
	 * results in an invalid args error message being returned without
	 * the handler being called.
	 */
	TEST_FEATURE ("with invalid variant item in method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_DOUBLE_AS_STRING,
						  &subiter);

		double_arg = 3.14;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
						&double_arg);

		dbus_message_iter_close_container (&iter, &subiter);

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

		dbus_message_iter_init (method_call, &iter);

		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);

		my_set_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_set (object, message, &iter);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_set_property_called);

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_INVALID_ARGS);
		nih_free (err);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (method_call);
	}


	/* Check that an extra argument to the property method call
	 * results in an invalid args error message being returned
	 * without the handler being called.
	 */
	TEST_FEATURE ("with extra argument to method (generated code)");
	TEST_ALLOC_FAIL {
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (client_conn),
			"/com/netsplit/Nih",
			"org.freedesktop.DBus.Properties",
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		iface = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&iface);

		name = "property";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&name);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str = "dog and doughnut";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str);

		dbus_message_iter_close_container (&iter, &subiter);

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

		dbus_message_iter_init (method_call, &iter);

		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);
		assert (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING);
		dbus_message_iter_next (&iter);

		my_set_property_called = 0;

		ret = my_com_netsplit_Nih_Test_property_set (object, message, &iter);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			nih_free (object);
			nih_free (message);
			dbus_message_unref (method_call);
			continue;
		}

		TEST_FALSE (my_set_property_called);

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_INVALID_ARGS);
		nih_free (err);

		nih_free (object);
		nih_free (message);
		dbus_message_unref (method_call);
	}


	/* Check that a deprecated property does not have the attribute
	 * added, since we don't want gcc warnings when implementing
	 * objects.
	 */
	TEST_FEATURE ("with deprecated property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&handlers);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
			property->deprecated = TRUE;
		}

		str = property_object_set_function (NULL, "my", interface, property,
						    &prototypes, &handlers,
						    &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&handlers);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_object_set_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_set");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_NOT_EMPTY (&handlers);

		func = (TypeFunc *)handlers.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_property");
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

		TEST_LIST_EMPTY (&handlers);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


int my_com_netsplit_Nih_Test_test_property_get_notify_called = FALSE;
static DBusPendingCall *   last_pending_call = NULL;
static NihDBusPendingData *last_pending_data = NULL;

void
my_com_netsplit_Nih_Test_test_property_get_notify (DBusPendingCall *   pending_call,
			     NihDBusPendingData *pending_data)
{
	my_com_netsplit_Nih_Test_test_property_get_notify_called = TRUE;
	last_pending_call = pending_call;
	last_pending_data = pending_data;
}

static void
my_blank_get_handler (void *          data,
		      NihDBusMessage *message,
		      const char *    value)
{
}

static void
my_blank_error_handler (void *          data,
			NihDBusMessage *message)
{
}

void
test_proxy_get_function (void)
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
	DBusMessageIter   subiter;
	char *            str_value;
	NihError *        err;
	NihDBusError *    dbus_err;

	TEST_FUNCTION ("property_proxy_get_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that makes a method call
	 * to obtain the value of a D-Bus property and returns the pending
	 * call structure.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "test_property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "test_property");
		}

		str = property_proxy_get_function (NULL, "my", interface, property,
						   &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_get_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_test_property");
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
		TEST_EQ_STR (arg->type, "MyGetTestPropertyReply");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to make a method call
	 * to obtain the value of a property.  The function should return
	 * a DBusPendingCall object and we should receive the method call
	 * on the other side.  Returning the reply and blocking the call
	 * should result in our notify function being called with the
	 * pending call that was returned and the pending data with the
	 * expected information.
	 */
	TEST_FEATURE ("with property (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_com_netsplit_Nih_Test_test_property_get_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_test_property (proxy,
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
							"Get"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

		dbus_message_iter_init_append (reply, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "wibble";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

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
		TEST_TRUE (my_com_netsplit_Nih_Test_test_property_get_notify_called);
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

		my_com_netsplit_Nih_Test_test_property_get_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_test_property (proxy,
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
							"Get"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_error (method_call,
						"com.netsplit.Nih.MyProperty.Fail",
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
						  "com.netsplit.Nih.MyProperty.Fail"));
		dbus_message_unref (reply);

		/* Check the notify function was called with all the right
		 * things.
		 */
		TEST_TRUE (my_com_netsplit_Nih_Test_test_property_get_notify_called);
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

		my_com_netsplit_Nih_Test_test_property_get_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_test_property (proxy,
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
							"Get"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

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
		TEST_TRUE (my_com_netsplit_Nih_Test_test_property_get_notify_called);
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

		my_com_netsplit_Nih_Test_test_property_get_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_test_property (proxy,
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
							"Get"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

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
		TEST_TRUE (my_com_netsplit_Nih_Test_test_property_get_notify_called);
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

		my_com_netsplit_Nih_Test_test_property_get_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_test_property (proxy,
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
							"Get"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

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
		TEST_FALSE (my_com_netsplit_Nih_Test_test_property_get_notify_called);

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

		my_com_netsplit_Nih_Test_test_property_get_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_get_test_property (proxy,
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

		TEST_FALSE (my_com_netsplit_Nih_Test_test_property_get_notify_called);

		nih_free (proxy);
	}


	/* Check that a deprecated property has its get function annotated
	 * with the deprecated attribute so that the client gets a gcc
	 * warning if they use it.
	 */
	TEST_FEATURE ("with deprecated property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "test_property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "test_property");
			property->deprecated = TRUE;
		}

		str = property_proxy_get_function (NULL, "my", interface, property,
						   &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_get_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_test_property");
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
		TEST_EQ_STR (arg->type, "MyGetTestPropertyReply");
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

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

static void my_error_handler (void *data, NihDBusMessage *message);

static int my_get_property_handler_called = FALSE;
static int my_error_handler_called = FALSE;
static NihDBusMessage *last_message = NULL;
static DBusConnection *last_conn = NULL;
static DBusMessage *last_msg = NULL;
static NihError *last_error = NULL;

static void
my_get_property_handler (void *          data,
			 NihDBusMessage *message,
			 const char *    value)
{
	my_get_property_handler_called++;

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

	TEST_NE_P (value, NULL);
	TEST_ALLOC_PARENT (value, message);
	TEST_EQ_STR (value, "wibble");
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
test_proxy_get_notify_function (void)
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
	DBusMessageIter     subiter;
	char *              str_value;
	double              double_value;
	NihDBusError *      dbus_err;

	TEST_FUNCTION ("property_proxy_get_notify_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that takes a pending call
	 * and pending data structure, stealing the D-Bus message and
	 * demarshalling the property value from the variant argument
	 * before making a call to either the handler for a valid reply
	 * or error handler for an invalid reply.  The typedef for the
	 * handler function is returned in addition to the prototype.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_proxy_get_notify_function (NULL, "my", interface,
							  property,
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

		TEST_EXPECTED_STR (str, "test_property_proxy_get_notify_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_get_notify");
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
		TEST_EQ_STR (func->name, "(*MyGetPropertyReply)");
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

		TEST_LIST_EMPTY (&typedefs);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can generate a notify function for a structure
	 * property, with the structure type passed back in the structs
	 * array.
	 */
	TEST_FEATURE ("with structure property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "(su)", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_proxy_get_notify_function (NULL, "my", interface,
							  property,
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

		TEST_EXPECTED_STR (str, "test_property_proxy_get_notify_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_get_notify");
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
		TEST_EQ_STR (func->name, "(*MyGetPropertyReply)");
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
		TEST_EQ_STR (arg->type, "const MyProperty *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
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
		TEST_EQ_STR (structure->name, "MyProperty");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to handle a completed
	 * pending call, demarshalling the property value from the variant in
	 * the reply and passing it to our handler.
	 */
	TEST_FEATURE ("with reply (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

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

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "wibble";
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
				(NihDBusReplyHandler)my_get_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_get_notify (pending_call, pending_data);

		TEST_TRUE (my_get_property_handler_called);
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
			"Get");

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
						"com.netsplit.Nih.MyProperty.Fail",
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
				(NihDBusReplyHandler)my_get_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_get_notify (pending_call, pending_data);

		TEST_FALSE (my_get_property_handler_called);
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
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.MyProperty.Fail");
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
			"Get");

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
				(NihDBusReplyHandler)my_get_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_get_notify (pending_call, pending_data);

		TEST_FALSE (my_get_property_handler_called);
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
			"Get");

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
				(NihDBusReplyHandler)my_get_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_get_notify (pending_call, pending_data);

		TEST_FALSE (my_get_property_handler_called);
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
			"Get");

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

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_DOUBLE_AS_STRING,
						  &subiter);

		double_value = 3.14;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
						&double_value);

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
				(NihDBusReplyHandler)my_get_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_get_notify (pending_call, pending_data);

		TEST_FALSE (my_get_property_handler_called);
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
	 * the reply and calls the error handler with the invalid arguments
	 * error raised.
	 */
	TEST_FEATURE ("with incorrect argument type (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

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

		str_value = "wibble";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

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
				(NihDBusReplyHandler)my_get_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_get_notify (pending_call, pending_data);

		TEST_FALSE (my_get_property_handler_called);
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
			"Get");

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
				(NihDBusReplyHandler)my_get_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_get_notify (pending_call, pending_data);

		TEST_FALSE (my_get_property_handler_called);
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
			"Get");

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

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "wibble";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

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
				(NihDBusReplyHandler)my_get_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_get_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_get_notify (pending_call, pending_data);

		TEST_FALSE (my_get_property_handler_called);
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


	/* Check that the generated function for a deprecated property is
	 * not marked deprecated, since it's implementation.
	 */
	TEST_FEATURE ("with deprecated property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
			property->deprecated = TRUE;
		}

		str = property_proxy_get_notify_function (NULL, "my", interface,
							  property,
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

		TEST_EXPECTED_STR (str, "test_property_proxy_get_notify_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_get_notify");
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
		TEST_EQ_STR (func->name, "(*MyGetPropertyReply)");
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

		TEST_LIST_EMPTY (&typedefs);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


int my_com_netsplit_Nih_Test_test_property_set_notify_called = FALSE;

void
my_com_netsplit_Nih_Test_test_property_set_notify (DBusPendingCall *   pending_call,
			     NihDBusPendingData *pending_data)
{
	my_com_netsplit_Nih_Test_test_property_set_notify_called = TRUE;
	last_pending_call = pending_call;
	last_pending_data = pending_data;
}

static void
my_blank_set_handler (void *          data,
		      NihDBusMessage *message)
{
}

void
test_proxy_set_function (void)
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
	TypeStruct *      structure;
	TypeVar *         var;
	NihListEntry *    attrib;
	DBusConnection *  flakey_conn;
	NihDBusProxy *    proxy = NULL;
	DBusPendingCall * pending_call;
	DBusMessage *     method_call;
	DBusMessage *     reply;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	char *            str_value;
	NihError *        err;
	NihDBusError *    dbus_err;

	TEST_FUNCTION ("property_proxy_set_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that makes a method call
	 * to set the value of a D-Bus property and returns the pending
	 * call structure.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "test_property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "test_property");
		}

		str = property_proxy_set_function (NULL, "my", interface, property,
						   &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_set_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_test_property");
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
		TEST_EQ_STR (arg->type, "MySetTestPropertyReply");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can generate a function for a structure property,
	 * with the structure type passed back in the structs array.
	 */
	TEST_FEATURE ("with structure property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "test_property",
						 "(su)", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "test_property");
		}

		str = property_proxy_set_function (NULL, "my", interface, property,
						   &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_set_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_test_property");
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
		TEST_EQ_STR (arg->type, "const MyTestProperty *");
		TEST_ALLOC_PARENT (arg->type, arg);
		TEST_EQ_STR (arg->name, "value");
		TEST_ALLOC_PARENT (arg->name, arg);
		nih_free (arg);

		TEST_LIST_NOT_EMPTY (&func->args);

		arg = (TypeVar *)func->args.next;
		TEST_ALLOC_SIZE (arg, sizeof (TypeVar));
		TEST_ALLOC_PARENT (arg, func);
		TEST_EQ_STR (arg->type, "MySetTestPropertyReply");
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


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyTestProperty");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that an array argument may be NULL if the length argument
	 * is zero.
	 */
	TEST_FEATURE ("with array property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "test_property",
						 "ai", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "test_property");
		}

		str = property_proxy_set_function (NULL, "my", interface, property,
						   &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_set_function_array.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_test_property");
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
		TEST_EQ_STR (arg->type, "MySetTestPropertyReply");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to make a method call
	 * to set the value of a property.  The function should return
	 * a DBusPendingCall object and we should receive the method call
	 * on the other side.  Returning the reply and blocking the call
	 * should result in our notify function being called with the
	 * pending call that was returned and the pending data with the
	 * expected information.
	 */
	TEST_FEATURE ("with property (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_com_netsplit_Nih_Test_test_property_set_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_set_test_property (proxy, "wibble",
						     my_blank_set_handler,
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
							"Set"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "wibble");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

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
		TEST_TRUE (my_com_netsplit_Nih_Test_test_property_set_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_set_handler);
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


	/* Check that the handler argument to the generated function
	 * may be NULL, since there's no useful information in there that
	 * isn't conveyed by a separate error reply (other than the success).
	 * The pending call should still be generated, and the message still
	 * expecting a reply, just the handler missing from the data (which
	 * tells the notify function to do everything but call it).
	 */
	TEST_FEATURE ("with no handler (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_com_netsplit_Nih_Test_test_property_set_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_set_test_property (proxy, "wibble",
						     NULL,
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
							"Set"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "wibble");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_method_return (method_call);
		dbus_message_unref (method_call);

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
		TEST_TRUE (my_com_netsplit_Nih_Test_test_property_set_notify_called);
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


	/* Check that both the handler and error handler arguments may
	 * be set to NULL for a fire-and-forget method call in which
	 * we don't care about the success or failure of setting the
	 * property.  The method call is flagged to expect no reply,
	 * and we don't ever call the notify function.
	 */
	TEST_FEATURE ("with no reply expected (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		my_com_netsplit_Nih_Test_test_property_set_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_set_test_property (proxy, "wibble",
						     NULL, NULL, NULL, -1);

		if (test_alloc_failed
		    && (pending_call == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);
			continue;
		}

		TEST_EQ_P (pending_call, (void *)TRUE);


		TEST_DBUS_MESSAGE (server_conn, method_call);

		/* Check the incoming message */
		TEST_TRUE (dbus_message_is_method_call (method_call,
							DBUS_INTERFACE_PROPERTIES,
							"Set"));
		TEST_TRUE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "wibble");

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
		TEST_FALSE (my_com_netsplit_Nih_Test_test_property_set_notify_called);

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

		my_com_netsplit_Nih_Test_test_property_set_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_set_test_property (proxy, "wibble",
						     my_blank_set_handler,
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
							"Set"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "wibble");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		/* Construct and send the reply */
		reply = dbus_message_new_error (method_call,
						"com.netsplit.Nih.MyProperty.Fail",
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
						  "com.netsplit.Nih.MyProperty.Fail"));
		dbus_message_unref (reply);

		/* Check the notify function was called with all the right
		 * things.
		 */
		TEST_TRUE (my_com_netsplit_Nih_Test_test_property_set_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_set_handler);
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

		my_com_netsplit_Nih_Test_test_property_set_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_set_test_property (proxy, "wibble",
						     my_blank_set_handler,
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
							"Set"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "wibble");

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
		TEST_TRUE (my_com_netsplit_Nih_Test_test_property_set_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_set_handler);
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

		my_com_netsplit_Nih_Test_test_property_set_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_set_test_property (proxy, "wibble",
						     my_blank_set_handler,
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
							"Set"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "wibble");

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
		TEST_TRUE (my_com_netsplit_Nih_Test_test_property_set_notify_called);
		TEST_EQ_P (last_pending_call, pending_call);
		TEST_ALLOC_SIZE (last_pending_data, sizeof (NihDBusPendingData));

		TEST_EQ_P (last_pending_data->connection, client_conn);
		TEST_EQ_P (last_pending_data->handler,
			   (NihDBusReplyHandler)my_blank_set_handler);
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

		my_com_netsplit_Nih_Test_test_property_set_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_set_test_property (proxy, "wibble",
						     my_blank_set_handler,
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
							"Set"));
		TEST_FALSE (dbus_message_get_no_reply (method_call));

		dbus_message_iter_init (method_call, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "test_property");

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "wibble");

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
		TEST_FALSE (my_com_netsplit_Nih_Test_test_property_set_notify_called);

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

		my_com_netsplit_Nih_Test_test_property_set_notify_called = FALSE;
		last_pending_call = NULL;
		last_pending_data = NULL;

		pending_call = my_set_test_property (proxy, "wibble",
						     my_blank_set_handler,
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

		TEST_FALSE (my_com_netsplit_Nih_Test_test_property_set_notify_called);

		nih_free (proxy);
	}


	/* Check that a deprecated property has its get function annotated
	 * with the deprecated attribute so that the client gets a gcc
	 * warning if they use it.
	 */
	TEST_FEATURE ("with deprecated property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "test_property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "test_property");
			property->deprecated = TRUE;
		}

		str = property_proxy_set_function (NULL, "my", interface, property,
						   &prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_set_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "DBusPendingCall *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_test_property");
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
		TEST_EQ_STR (arg->type, "MySetTestPropertyReply");
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

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

static int my_set_property_handler_called = FALSE;

static void
my_set_property_handler (void *          data,
		 NihDBusMessage *message)
{
	my_set_property_handler_called++;

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
}

void
test_proxy_set_notify_function (void)
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
	DBusConnection *    server_conn;
	DBusConnection *    client_conn;
	DBusConnection *    flakey_conn;
	dbus_uint32_t       serial;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data = NULL;
	DBusMessage *       method_call;
	DBusMessage *       reply;
	DBusMessageIter     iter;
	double              double_value;
	NihDBusError *      dbus_err;

	TEST_FUNCTION ("property_proxy_set_notify_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that takes a pending call
	 * and pending data structure, stealing the D-Bus message and
	 * before making a call to either the handler for a valid reply
	 * or error handler for an invalid reply.  The typedef for the
	 * handler function is returned in addition to the prototype.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_proxy_set_notify_function (NULL, "my", interface,
							  property,
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

		TEST_EXPECTED_STR (str, "test_property_proxy_set_notify_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_set_notify");
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
		TEST_EQ_STR (func->name, "(*MySetPropertyReply)");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to handle a completed
	 * pending call, checking the reply has no arguments before passing
	 * it to our handler.
	 */
	TEST_FEATURE ("with reply (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

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
				(NihDBusReplyHandler)my_set_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_set_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_set_notify (pending_call, pending_data);

		TEST_TRUE (my_set_property_handler_called);
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


	/* Since there's no useful information in the reply, we allow
	 * it to be omitted (thus only requiring the error handler),
	 * check that in this case the function is not called.
	 */
	TEST_FEATURE ("with no handler (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

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
				my_error_handler, (void *)my_error_handler);
		}

		my_set_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_set_notify (pending_call, pending_data);

		TEST_FALSE (my_set_property_handler_called);
		TEST_FALSE (my_error_handler_called);

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
			"Set");

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
						"com.netsplit.Nih.MyProperty.Fail",
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
				(NihDBusReplyHandler)my_set_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_set_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_set_notify (pending_call, pending_data);

		TEST_FALSE (my_set_property_handler_called);
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
		TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.MyProperty.Fail");
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
			"Set");

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
				(NihDBusReplyHandler)my_set_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_set_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_set_notify (pending_call, pending_data);

		TEST_FALSE (my_set_property_handler_called);
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
			"Set");

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
				(NihDBusReplyHandler)my_set_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_set_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_set_notify (pending_call, pending_data);

		TEST_FALSE (my_set_property_handler_called);
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


	/* Check that the generated code catches an unexpected argument in
	 * the reply and calls the error handler with the invalid arguments
	 * error raised.
	 */
	TEST_FEATURE ("with unexpected argument (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

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
				(NihDBusReplyHandler)my_set_property_handler,
				my_error_handler, (void *)my_error_handler);
		}

		my_set_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_set_notify (pending_call, pending_data);

		TEST_FALSE (my_set_property_handler_called);
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


	/* Check that an unexpected argument is still caught even if no
	 * reply handler has been specified.
	 */
	TEST_FEATURE ("with unexpected argument and no handler (generated code)");
	TEST_ALLOC_FAIL {
		/* Make the method call */
		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

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
				NULL,
				my_error_handler, (void *)my_error_handler);
		}

		my_set_property_handler_called = FALSE;
		my_error_handler_called = FALSE;
		last_message = NULL;
		last_conn = NULL;
		last_msg = NULL;

		my_com_netsplit_Nih_Test_property_set_notify (pending_call, pending_data);

		TEST_FALSE (my_set_property_handler_called);
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


	/* Check that the generated function for a deprecated property is
	 * not marked deprecated, since it's implementation.
	 */
	TEST_FEATURE ("with deprecated property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&typedefs);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
			property->deprecated = TRUE;
		}

		str = property_proxy_set_notify_function (NULL, "my", interface,
							  property,
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

		TEST_EXPECTED_STR (str, "test_property_proxy_set_notify_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "void");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_com_netsplit_Nih_Test_property_set_notify");
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
		TEST_EQ_STR (func->name, "(*MySetPropertyReply)");
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
		nih_free (property);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_proxy_get_sync_function (void)
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
	TypeStruct *    structure;
	TypeVar *       var;
	NihListEntry *  attrib;
	NihDBusProxy *  proxy = NULL;
	void *          parent = NULL;
	pid_t           pid;
	int             status;
	DBusMessage *   method_call;
	DBusMessage *   reply;
	DBusMessageIter iter;
	DBusMessageIter subiter;
	char *          str_value;
	double          double_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;

	TEST_FUNCTION ("property_proxy_get_sync_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that will make a method
	 * call to obtain the value of a property and return it in the
	 * pointer argument supplied.  The function returns an integer
	 * to indicate success.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_proxy_get_sync_function (NULL, "my", interface,
							property,
							&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_get_sync_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_property_sync");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can generate a function that will make a method
	 * call to obtain the value of a property and return it in the
	 * pointer argument supplied.  The function returns an integer
	 * to indicate success.
	 */
	TEST_FEATURE ("with structure property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "(su)", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_proxy_get_sync_function (NULL, "my", interface,
							property,
							&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_get_sync_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_property_sync");
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
		TEST_EQ_STR (arg->type, "MyProperty **");
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


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyProperty");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to make a method call
	 * and obtain the value of the property.
	 */
	TEST_FEATURE ("with method call (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"Get"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "property");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter);

			str_value = "wibble";
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

		str_value = NULL;

		ret = my_get_property_sync (parent, proxy, &str_value);

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

			TEST_EQ_P (str_value, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (ret, 0);

		TEST_NE_P (str_value, NULL);
		TEST_ALLOC_PARENT (str_value, parent);
		TEST_EQ_STR (str_value, "wibble");

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
								"Get"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "property");

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

		str_value = NULL;

		ret = my_get_property_sync (parent, proxy, &str_value);

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

			TEST_EQ_P (str_value, NULL);

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

		TEST_EQ_P (str_value, NULL);

		nih_free (proxy);
	}


	/* Check that the generated code returns a raised disconnected
	 * error when called on a disconnected connection.
	 */
	TEST_FEATURE ("with disconnected connection (generated code)");
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

		str_value = NULL;

		ret = my_get_property_sync (parent, proxy, &str_value);

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);

			TEST_EQ_P (str_value, NULL);

			nih_free (proxy);
			continue;
		}

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_DISCONNECTED);
		nih_free (err);

		TEST_EQ_P (str_value, NULL);

		nih_free (proxy);
	}


	/* Check that an incorrect type in the variant results in the
	 * function returning a raised error.
	 */
	TEST_FEATURE ("with incorrect type in variant (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"Get"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "property");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
							&double_value);

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

		str_value = NULL;

		ret = my_get_property_sync (parent, proxy, &str_value);

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

			TEST_EQ_P (str_value, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (str_value, NULL);

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
								"Get"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "property");

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

		str_value = NULL;

		ret = my_get_property_sync (parent, proxy, &str_value);

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

			TEST_EQ_P (str_value, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (str_value, NULL);

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
								"Get"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "property");

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

		str_value = NULL;

		ret = my_get_property_sync (parent, proxy, &str_value);

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

			TEST_EQ_P (str_value, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (str_value, NULL);

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
								"Get"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "property");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_INVALID);

			/* Construct and send the reply */
			reply = dbus_message_new_method_return (method_call);
			dbus_message_unref (method_call);

			dbus_message_iter_init_append (reply, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter);

			str_value = "wibble";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&iter, &subiter);

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

		str_value = NULL;

		ret = my_get_property_sync (parent, proxy, &str_value);

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

			TEST_EQ_P (str_value, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
		nih_free (err);

		TEST_EQ_P (str_value, NULL);

		nih_free (proxy);
	}


	/* Check that a deprecated property has the deprecated attribute
	 * added to its function prototype, since we want to warn about
	 * client code using them.
	 */
	TEST_FEATURE ("with deprecated property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
			property->deprecated = TRUE;
		}

		str = property_proxy_get_sync_function (NULL, "my", interface,
							property,
							&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_get_sync_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_get_property_sync");
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


		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_proxy_set_sync_function (void)
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
	TypeStruct *    structure;
	TypeVar *       var;
	NihListEntry *  attrib;
	NihDBusProxy *  proxy = NULL;
	void *          parent = NULL;
	pid_t           pid = -1;
	int             status;
	DBusMessage *   method_call;
	DBusMessage *   reply;
	DBusMessageIter iter;
	DBusMessageIter subiter;
	char *          str_value;
	double          double_value;
	int             ret;
	NihError *      err;
	NihDBusError *  dbus_err;

	TEST_FUNCTION ("property_proxy_set_sync_function");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can generate a function that will make a method
	 * call to set the value of a property, returning an integer to
	 * indicate success.
	 */
	TEST_FEATURE ("with property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_proxy_set_sync_function (NULL, "my", interface,
							property,
							&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_set_sync_function_standard.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_property_sync");
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

		TEST_LIST_EMPTY (&prototypes);


		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can generate a function for a structure property,
	 * with the structure type passed back in the structs array.
	 */
	TEST_FEATURE ("with structure property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "(su)", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_proxy_set_sync_function (NULL, "my", interface,
							property,
							&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_set_sync_function_structure.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_property_sync");
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
		TEST_EQ_STR (arg->type, "const MyProperty *");
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


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyProperty");
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
		nih_free (property);
		nih_free (interface);
	}


	/* Check that an array property argument may be NULL if the length
	 * of the array is zero.
	 */
	TEST_FEATURE ("with array property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "ai", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
		}

		str = property_proxy_set_sync_function (NULL, "my", interface,
							property,
							&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_set_sync_function_array.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_property_sync");
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


		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
	}


	/* Check that we can use the generated code to make a method call
	 * and set the value of the property.
	 */
	TEST_FEATURE ("with method call (generated code)");
	TEST_ALLOC_FAIL {
		TEST_CHILD (pid) {
			TEST_DBUS_MESSAGE (server_conn, method_call);

			/* Check the incoming message */
			TEST_TRUE (dbus_message_is_method_call (method_call,
								DBUS_INTERFACE_PROPERTIES,
								"Set"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "property");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_VARIANT);

			dbus_message_iter_recurse (&iter, &subiter);

			TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&subiter, &str_value);
			TEST_EQ_STR (str_value, "wibble");

			dbus_message_iter_next (&subiter);

			TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
				 DBUS_TYPE_INVALID);

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

		ret = my_set_property_sync (NULL, proxy, "wibble");

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

			TEST_EQ_P (str_value, NULL);

			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (ret, 0);

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
								"Set"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "property");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_VARIANT);

			dbus_message_iter_recurse (&iter, &subiter);

			TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&subiter, &str_value);
			TEST_EQ_STR (str_value, "wibble");

			dbus_message_iter_next (&subiter);

			TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
				 DBUS_TYPE_INVALID);

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

		ret = my_set_property_sync (NULL, proxy, "wibble");

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

			TEST_EQ_P (str_value, NULL);

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

		nih_free (proxy);
	}


	/* Check that the generated code returns a raised disconnected
	 * error when called on a disconnected connection.
	 */
	TEST_FEATURE ("with disconnected connection (generated code)");
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

		ret = my_set_property_sync (NULL, proxy, "wibble");

		TEST_LT (ret, 0);

		err = nih_error_get ();

		if (test_alloc_failed
		    && (err->number == ENOMEM)) {
			nih_free (err);
			nih_free (proxy);
			continue;
		}

		waitpid (pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		dbus_err = (NihDBusError *)err;
		TEST_EQ_STR (dbus_err->name, DBUS_ERROR_DISCONNECTED);
		nih_free (err);

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
								"Set"));

			dbus_message_iter_init (method_call, &iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "com.netsplit.Nih.Test");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&iter, &str_value);
			TEST_EQ_STR (str_value, "property");

			dbus_message_iter_next (&iter);

			TEST_EQ (dbus_message_iter_get_arg_type (&iter),
				 DBUS_TYPE_VARIANT);

			dbus_message_iter_recurse (&iter, &subiter);

			TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
				 DBUS_TYPE_STRING);

			dbus_message_iter_get_basic (&subiter, &str_value);
			TEST_EQ_STR (str_value, "wibble");

			dbus_message_iter_next (&subiter);

			TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
				 DBUS_TYPE_INVALID);

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

		ret = my_set_property_sync (NULL, proxy, "wibble");

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


	/* Check that a deprecated property has the deprecated attribute
	 * added to its function prototype, since we want to warn against
	 * client code using this.
	 */
	TEST_FEATURE ("with deprecated property");
	TEST_ALLOC_FAIL {
		nih_list_init (&prototypes);
		nih_list_init (&structs);

		TEST_ALLOC_SAFE {
			interface = interface_new (NULL, "com.netsplit.Nih.Test");
			interface->symbol = NULL;

			property = property_new (NULL, "property",
						 "s", NIH_DBUS_READWRITE);
			property->symbol = nih_strdup (property, "property");
			property->deprecated = TRUE;
		}

		str = property_proxy_set_sync_function (NULL, "my", interface,
							property,
							&prototypes, &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_LIST_EMPTY (&prototypes);
			TEST_LIST_EMPTY (&structs);

			nih_free (property);
			nih_free (interface);
			continue;
		}

		TEST_EXPECTED_STR (str, "test_property_proxy_set_sync_function_deprecated.c");

		TEST_LIST_NOT_EMPTY (&prototypes);

		func = (TypeFunc *)prototypes.next;
		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_ALLOC_PARENT (func, str);
		TEST_EQ_STR (func->type, "int");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "my_set_property_sync");
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


		TEST_LIST_EMPTY (&structs);

		nih_free (str);
		nih_free (property);
		nih_free (interface);
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
	test_lookup ();

	test_object_get_function ();
	test_object_set_function ();

	test_proxy_get_function ();
	test_proxy_get_notify_function ();

	test_proxy_set_function ();
	test_proxy_set_notify_function ();

	test_proxy_get_sync_function ();
	test_proxy_set_sync_function ();

	return 0;
}
