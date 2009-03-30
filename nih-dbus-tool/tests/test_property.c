/* nih-dbus-tool
 *
 * test_property.c - test suite for nih-dbus-tool/property.c
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
#include "property.h"
#include "parse.h"
#include "errors.h"


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
	ParseContext  context;
	ParseStack   *parent = NULL, *entry;
	XML_Parser    xmlp;
	Interface    *interface = NULL;
	Property     *property;
	char         *attr[9];
	int           ret;
	NihError     *err;
	FILE         *output;

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
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

	attr[0] = "type";
	attr[1] = "s";
	attr[2] = "access";
	attr[3] = "read";
	attr[4] = NULL;

	ret = property_start_tag (xmlp, "property", attr);

	TEST_LT (ret, 0);

	TEST_EQ_P (parse_stack_top (&context.stack), parent);

	TEST_LIST_EMPTY (&interface->properties);

	err = nih_error_get ();
	TEST_EQ (err->number, PROPERTY_MISSING_NAME);
	nih_free (err);

	nih_free (parent);


	/* Check that a property with an invalid name results in an
	 * error being raised.
	 */
	TEST_FEATURE ("with invalid name");
	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_INTERFACE, interface);

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
	parent = parse_stack_push (NULL, &context.stack,
				   PARSE_NODE, node_new (NULL, NULL));

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
	ParseContext  context;
	ParseStack   *parent = NULL, *entry = NULL;
	XML_Parser    xmlp;
	Interface    *interface = NULL;
	Property     *property = NULL, *other = NULL;
	int           ret;
	NihError     *err;

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

			property = property_new (NULL, "TestProperty", "s",
						 NIH_DBUS_READ);
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_PROPERTY, property);
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

			property = property_new (NULL, "TestProperty",
						 "s", NIH_DBUS_READ);
			property->symbol = nih_strdup (property, "foo");
			entry = parse_stack_push (NULL, &context.stack,
						  PARSE_PROPERTY, property);
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

	other = property_new (interface, "Test", "s", NIH_DBUS_READ);
	other->symbol = nih_strdup (other, "test_property");
	nih_list_add (&interface->properties, &other->entry);

	property = property_new (NULL, "TestProperty", "s", NIH_DBUS_READ);
	entry = parse_stack_push (NULL, &context.stack,
				  PARSE_PROPERTY, property);

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
	char     *symbol;
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

	return 0;
}
