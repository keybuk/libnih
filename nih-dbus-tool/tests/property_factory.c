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

#include "property.h"


int
main (int   argc,
      char *argv[])
{
	nih_local Property *property = NULL;
	nih_local char *    code = NULL;

	printf ("#include <dbus/dbus.h>\n"
		"\n"
		"#include <nih/macros.h>\n"
		"#include <nih/alloc.h>\n"
		"#include <nih/string.h>\n"
		"#include <nih/logging.h>\n"
		"#include <nih/error.h>\n"
		"#include <nih/errors.h>\n"
		"\n"
		"#include <nih-dbus/dbus_error.h>\n"
		"#include <nih-dbus/dbus_message.h>\n"
		"#include <nih-dbus/dbus_object.h>\n"
		"\n"
		"#include \"property_code.h\"\n"
		"\n"
		"\n");

	printf ("extern int my_property_get (void *data, "
		"NihDBusMessage *message, char **str);\n"
		"\n");

	property = property_new (NULL, "my_property", "s", NIH_DBUS_READWRITE);
	property->symbol = nih_strdup (property, "my_property");

	code = property_object_get_function (NULL, property,
					     "MyProperty_get",
					     "my_property_get");

	printf ("%s", code);
	printf ("\n"
		"\n");

	printf ("extern int my_property_set (void *data, "
		"NihDBusMessage *message, const char *str);\n"
		"\n");

	code = property_object_set_function (NULL, property,
					     "MyProperty_set",
					     "my_property_set");

	printf ("%s", code);

	return 0;
}
