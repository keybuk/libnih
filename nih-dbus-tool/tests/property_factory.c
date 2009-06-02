/* nih-dbus-tool
 *
 * tests/property_factory.c - generate tests/property_code.c
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <stdio.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>

#include "type.h"
#include "interface.h"
#include "property.h"


int
main (int   argc,
      char *argv[])
{
	NihList              prototypes;
	NihList              handlers;
	NihList              typedefs;
	nih_local Interface *interface = NULL;
	nih_local Property * property = NULL;
	nih_local char *     code = NULL;
	nih_local char *     block = NULL;

	printf ("#include <dbus/dbus.h>\n"
		"\n"
		"#include <nih/macros.h>\n"
		"#include <nih/alloc.h>\n"
		"#include <nih/string.h>\n"
		"#include <nih/logging.h>\n"
		"#include <nih/error.h>\n"
		"\n"
		"#include <nih-dbus/dbus_error.h>\n"
		"#include <nih-dbus/dbus_message.h>\n"
		"#include <nih-dbus/dbus_object.h>\n"
		"#include <nih-dbus/dbus_pending_data.h>\n"
		"#include <nih-dbus/dbus_proxy.h>\n"
		"#include <nih-dbus/errors.h>\n"
		"\n"
		"#include \"property_code.h\"\n"
		"\n"
		"\n");

	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	interface->symbol = NULL;

	property = property_new (NULL, "property", "s", NIH_DBUS_READWRITE);
	property->symbol = nih_strdup (property, "property");


	nih_list_init (&prototypes);
	nih_list_init (&handlers);

	code = property_object_get_function (NULL, "my", interface, property,
					     &prototypes, &handlers);

	NIH_LIST_FOREACH (&handlers, iter) {
		TypeFunc *func = (TypeFunc *)iter;

		NIH_MUST (type_to_extern (&func->type, func));
	}

	block = type_func_layout (NULL, &handlers);

	printf ("%s\n", block);
	printf ("%s", code);
	printf ("\n"
		"\n");


	nih_list_init (&prototypes);
	nih_list_init (&handlers);

	code = property_object_set_function (NULL, "my", interface, property,
					     &prototypes, &handlers);

	NIH_LIST_FOREACH (&handlers, iter) {
		TypeFunc *func = (TypeFunc *)iter;

		NIH_MUST (type_to_extern (&func->type, func));
	}

	block = type_func_layout (NULL, &handlers);

	printf ("%s\n", block);
	printf ("%s", code);
	printf ("\n"
		"\n");


	nih_list_init (&prototypes);

	property->name = "test_property";
	property->symbol = "test_property";

	code = property_proxy_get_function (NULL, "my", interface, property,
					    &prototypes);

	printf ("extern void my_com_netsplit_Nih_Test_test_property_get_notify (DBusPendingCall *pending_call, "
		"NihDBusPendingData *pending_data);\n");
	printf ("\n");

	printf ("%s"
		"\n", code);


	nih_list_init (&prototypes);
	nih_list_init (&typedefs);

	property->name = "property";
	property->symbol = "property";

	code = property_proxy_get_notify_function (NULL, "my", interface,
						   property,
						   &prototypes, &typedefs);

	printf ("%s", code);
	printf ("\n"
		"\n");


	nih_list_init (&prototypes);

	property->name = "test_property";
	property->symbol = "test_property";

	code = property_proxy_set_function (NULL, "my", interface, property,
					    &prototypes);

	printf ("extern void my_com_netsplit_Nih_Test_test_property_set_notify (DBusPendingCall *pending_call, "
		"NihDBusPendingData *pending_data);\n");
	printf ("\n");

	printf ("%s"
		"\n", code);


	nih_list_init (&prototypes);
	nih_list_init (&typedefs);

	property->name = "property";
	property->symbol = "property";

	code = property_proxy_set_notify_function (NULL, "my", interface,
						   property,
						   &prototypes, &typedefs);

	printf ("%s", code);
	printf ("\n"
		"\n");


	nih_list_init (&prototypes);

	code = property_proxy_get_sync_function (NULL, "my", interface,
						 property,
						 &prototypes);

	printf ("%s"
		"\n", code);


	nih_list_init (&prototypes);

	code = property_proxy_set_sync_function (NULL, "my", interface,
						 property,
						 &prototypes);

	printf ("%s", code);

	return 0;
}
