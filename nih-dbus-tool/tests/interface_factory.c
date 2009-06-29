/* nih-dbus-tool
 *
 * tests/interface_factory.c - generate tests/interface_code.c
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
	NihList              typedefs;
	NihList              structs;
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
		"#include \"tests/interface_code.h\"\n"
		"\n"
		"\n");

	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	interface->symbol = NULL;

	property = property_new (interface, "name", "s", NIH_DBUS_READWRITE);
	property->symbol = nih_strdup (property, "name");
	nih_list_add (&interface->properties, &property->entry);

	property = property_new (interface, "size", "u", NIH_DBUS_READWRITE);
	property->symbol = nih_strdup (property, "size");
	nih_list_add (&interface->properties, &property->entry);


	nih_list_init (&prototypes);
	nih_list_init (&structs);

	interface->name = "com.netsplit.Nih.TestA";

	code = interface_proxy_get_all_function (NULL, "my", interface,
						 &prototypes, &structs);

	printf ("extern void my_com_netsplit_Nih_TestA_get_all_notify (DBusPendingCall *pending_call, "
		"NihDBusPendingData *pending_data);\n");
	printf ("\n");

	printf ("%s"
		"\n", code);


	nih_list_init (&prototypes);
	nih_list_init (&typedefs);
	nih_list_init (&structs);

	interface->name = "com.netsplit.Nih.Test";

	code = interface_proxy_get_all_notify_function (NULL, "my", interface,
							&prototypes, &typedefs,
							&structs);

	printf ("%s", code);
	printf ("\n"
		"\n");


	nih_list_init (&prototypes);
	nih_list_init (&structs);

	code = interface_proxy_get_all_sync_function (NULL, "my", interface,
						      &prototypes, &structs);

	printf ("%s"
		"\n", code);

	return 0;
}
