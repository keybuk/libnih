/* nih-dbus-tool
 *
 * tests/signal_factory.c - generate tests/signal_code.c
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
#include "signal.h"
#include "argument.h"


int
main (int   argc,
      char *argv[])
{
	NihList              prototypes;
	NihList              typedefs;
	NihList              structs;
	nih_local Interface *interface = NULL;
	nih_local Signal *   signal = NULL;
	Argument *           arg;
	nih_local char *     code = NULL;

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
		"#include <nih-dbus/dbus_proxy.h>\n"
		"#include <nih-dbus/errors.h>\n"
		"\n"
		"#include \"tests/signal_code.h\"\n"
		"\n");

	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	interface->symbol = NULL;

	signal = signal_new (NULL, "Signal");
	signal->symbol = nih_strdup (signal, "signal");

	arg = argument_new (signal, "Msg", "s", NIH_DBUS_ARG_OUT);
	arg->symbol = nih_strdup (arg, "msg");
	nih_list_add (&signal->arguments, &arg->entry);


	nih_list_init (&prototypes);
	nih_list_init (&structs);

	code = signal_object_function (NULL, "my", interface, signal,
				       &prototypes, &structs);

	printf ("%s"
		"\n",
		code);


	nih_list_init (&prototypes);
	nih_list_init (&typedefs);
	nih_list_init (&structs);

	code = signal_proxy_function (NULL, "my", interface, signal,
				      &prototypes, &typedefs, &structs);

	printf ("%s", code);

	return 0;
}
