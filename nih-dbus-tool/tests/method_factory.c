/* nih-dbus-tool
 *
 * tests/method_factory.c - generate tests/method_code.c
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
#include "method.h"
#include "argument.h"


int
main (int   argc,
      char *argv[])
{
	NihList              prototypes;
	NihList              handlers;
	NihList              typedefs;
	NihList              structs;
	nih_local Interface *interface = NULL;
	nih_local Method *   method = NULL;
	Argument *           arg;
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
		"#include \"tests/method_code.h\"\n"
		"\n"
		"\n");

	interface = interface_new (NULL, "com.netsplit.Nih.Test");
	interface->symbol = NULL;

	method = method_new (NULL, "Method");
	method->symbol = nih_strdup (method, "method");

	arg = argument_new (method, "Str", "s", NIH_DBUS_ARG_IN);
	arg->symbol = nih_strdup (arg, "str");
	nih_list_add (&method->arguments, &arg->entry);

	arg = argument_new (method, "Flags", "i", NIH_DBUS_ARG_IN);
	arg->symbol = nih_strdup (arg, "flags");
	nih_list_add (&method->arguments, &arg->entry);

	arg = argument_new (method, "Output", "as", NIH_DBUS_ARG_OUT);
	arg->symbol = nih_strdup (arg, "output");
	nih_list_add (&method->arguments, &arg->entry);


	nih_list_init (&prototypes);
	nih_list_init (&handlers);
	nih_list_init (&structs);

	code = method_object_function (NULL, "my", interface, method,
				       &prototypes, &handlers,
				       &structs);

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
	nih_list_init (&structs);

	method->name = "AsyncMethod";
	method->symbol = "async_method";
	method->async = TRUE;

	code = method_object_function (NULL, "my", interface, method,
				       &prototypes, &handlers,
				       &structs);

	NIH_LIST_FOREACH (&handlers, iter) {
		TypeFunc *func = (TypeFunc *)iter;

		NIH_MUST (type_to_extern (&func->type, func));
	}

	block = type_func_layout (NULL, &handlers);

	printf ("%s\n", block);
	printf ("%s", code);
	printf ("\n");


	nih_list_init (&prototypes);
	nih_list_init (&structs);

	code = method_reply_function (NULL, "my", interface, method,
				      &prototypes, &structs);

	printf ("%s", code);
	printf ("\n"
		"\n");


	nih_list_init (&prototypes);
	nih_list_init (&structs);

	method->name = "TestMethod";
	method->symbol = "test_method";
	method->async = FALSE;

	arg = argument_new (method, "Length", "i", NIH_DBUS_ARG_OUT);
	arg->symbol = nih_strdup (arg, "length");
	nih_list_add (&method->arguments, &arg->entry);

	code = method_proxy_function (NULL, "my", interface, method,
				      &prototypes, &structs);

	printf ("extern void my_com_netsplit_Nih_Test_TestMethod_notify (DBusPendingCall *pending_call, "
		"NihDBusPendingData *pending_data);\n");
	printf ("\n");

	printf ("%s", code);
	printf ("\n");


	nih_list_init (&prototypes);
	nih_list_init (&typedefs);
	nih_list_init (&structs);

	method->name = "Method";
	method->symbol = "method";

	code = method_proxy_notify_function (NULL, "my", interface, method,
					     &prototypes, &typedefs,
					     &structs);

	printf ("%s", code);
	printf ("\n"
		"\n");


	nih_list_init (&prototypes);
	nih_list_init (&structs);

	code = method_proxy_sync_function (NULL, "my", interface, method,
					   &prototypes, &structs);

	printf ("%s", code);
	printf ("\n");

	return 0;
}
