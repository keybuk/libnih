/* nih-dbus-tool
 *
 * tests/property_factory.c - generate tests/property_code.c
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <stdio.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>

#include "type.h"
#include "property.h"


int
main (int   argc,
      char *argv[])
{
	NihList             prototypes;
	NihList             handlers;
	NihList             typedefs;
	nih_local Property *property = NULL;
	nih_local char *    code = NULL;
	nih_local char *    block = NULL;

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

	property = property_new (NULL, "my_property", "s", NIH_DBUS_READWRITE);
	property->symbol = nih_strdup (property, "my_property");


	nih_list_init (&prototypes);
	nih_list_init (&handlers);

	code = property_object_get_function (NULL, property,
					     "MyProperty_get",
					     "my_property_get_handler",
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

	code = property_object_set_function (NULL, property,
					     "MyProperty_set",
					     "my_property_set_handler",
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

	code = property_proxy_get_function (NULL,
					    "com.netsplit.Nih.Test",
					    property,
					    "my_property_get",
					    "my_test_property_get_notify",
					    "MyPropertyGetHandler",
					    &prototypes);

	printf ("extern void my_test_property_get_notify (DBusPendingCall *pending_call, "
		"NihDBusPendingData *pending_data);\n");
	printf ("\n");

	printf ("%s"
		"\n", code);


	nih_list_init (&prototypes);
	nih_list_init (&typedefs);

	code = property_proxy_get_notify_function (NULL, property,
						   "my_property_get_notify",
						   "MyPropertyGetHandler",
						   &prototypes, &typedefs);

	printf ("%s", code);
	printf ("\n"
		"\n");


	nih_list_init (&prototypes);

	code = property_proxy_get_sync_function (NULL,
						 "com.netsplit.Nih.Test",
						 property,
						 "my_property_get_sync",
						 &prototypes);

	printf ("%s"
		"\n", code);


	nih_list_init (&prototypes);

	code = property_proxy_set_sync_function (NULL,
						 "com.netsplit.Nih.Test",
						 property,
						 "my_property_set_sync",
						 &prototypes);

	printf ("%s", code);

	return 0;
}
