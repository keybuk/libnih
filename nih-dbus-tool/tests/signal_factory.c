/* nih-dbus-tool
 *
 * tests/signal_factory.c - generate tests/signal_code.c
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
#include "signal.h"
#include "argument.h"


int
main (int   argc,
      char *argv[])
{
	NihList           prototypes;
	NihList           typedefs;
	nih_local Signal *signal = NULL;
	Argument *        arg;
	nih_local char *  code = NULL;

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
		"#include \"signal_code.h\"\n"
		"\n");

	signal = signal_new (NULL, "MySignal");
	signal->symbol = nih_strdup (signal, "my_signal");

	arg = argument_new (signal, "Msg", "s", NIH_DBUS_ARG_OUT);
	arg->symbol = nih_strdup (arg, "msg");
	nih_list_add (&signal->arguments, &arg->entry);


	nih_list_init (&prototypes);

	code = signal_object_function (NULL, "com.netsplit.Nih.Test", signal,
				       "my_emit_signal",
				       &prototypes);

	printf ("%s"
		"\n",
		code);


	nih_list_init (&prototypes);
	nih_list_init (&typedefs);

	code = signal_proxy_function (NULL, signal,
				      "my_signal_filter",
				      "MySignalHandler",
				      &prototypes,
				      &typedefs);

	printf ("%s", code);

	return 0;
}
