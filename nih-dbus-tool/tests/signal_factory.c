/* nih-dbus-tool
 *
 * tests/signal_factory.c - generate tests/signal_code.c
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

#include "signal.h"
#include "argument.h"


int
main (int   argc,
      char *argv[])
{
	NihList           prototypes;
	NihList           externs;
	nih_local Signal *signal = NULL;
	Argument *        arg;
	nih_local char *  code = NULL;

	nih_list_init (&prototypes);
	nih_list_init (&externs);

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
		"#include \"signal_code.h\"\n"
		"\n"
		"\n");

	signal = signal_new (NULL, "MySignal");
	signal->symbol = nih_strdup (signal, "my_signal");

	arg = argument_new (signal, "Msg", "s", NIH_DBUS_ARG_OUT);
	arg->symbol = nih_strdup (arg, "msg");
	nih_list_add (&signal->arguments, &arg->entry);

	code = signal_emit_function (NULL, "com.netsplit.Nih.Test", signal,
				     "my_emit_signal",
				     &prototypes, &externs);

	printf ("%s", code);

	return 0;
}
