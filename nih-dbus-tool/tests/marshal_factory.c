/* nih-dbus-tool
 *
 * tests/marshal_factory.c - generate tests/marshal_code.c
 *
 * Copyright © 2010 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2010 Canonical Ltd.
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


#include <dbus/dbus.h>

#include <stdio.h>
#include <assert.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>

#include "indent.h"
#include "type.h"
#include "marshal.h"


static void
marshal_function (const char *name,
		  const char *signature)
{
	DBusSignatureIter iter;
	nih_local char *  code = NULL;
	NihList           locals;
	NihList           inputs;
	NihList           structs;

	nih_list_init (&inputs);
	nih_list_init (&locals);
	nih_list_init (&structs);

	dbus_signature_iter_init (&iter, signature);

	code = marshal (NULL, &iter,
			"iter", "value",
			"return -1;\n",
			&inputs, &locals,
			"my", NULL, name, "value",
			&structs);

	printf ("int\n"
		"my_%s_marshal (DBusMessage *message",
		name);

	NIH_LIST_FOREACH (&inputs, iter) {
		TypeVar *var = (TypeVar *)iter;

		assert (type_to_const (&var->type, var));

		printf (", %s %s", var->type, var->name);
	}

	printf (")\n"
		"{\n"
		"\tDBusMessageIter iter;\n");

	NIH_LIST_FOREACH (&locals, iter) {
		TypeVar *local_var = (TypeVar *)iter;

		printf ("\t%s %s;\n", local_var->type, local_var->name);
	}

	printf ("\n" /* FIXME */
		"\tnih_assert (message != NULL);\n"
		"\n"
		"\tdbus_message_iter_init_append (message, &iter);\n"
		"\n");

	assert (indent (&code, NULL, 1));
	printf ("%s", code);

	printf ("\n"
		"\treturn 0;\n"
		"}\n"
		"\n");
}


int
main (int   argc,
      char *argv[])
{
	printf ("#include <dbus/dbus.h>\n"
		"\n"
		"#include <nih/macros.h>\n"
		"#include <nih/alloc.h>\n"
		"#include <nih/string.h>\n"
		"#include <nih/logging.h>\n"
		"#include <nih/error.h>\n"
		"\n"
		"#include \"tests/marshal_code.h\"\n"
		"\n");

	marshal_function ("byte", DBUS_TYPE_BYTE_AS_STRING);
	marshal_function ("boolean", DBUS_TYPE_BOOLEAN_AS_STRING);
	marshal_function ("int16", DBUS_TYPE_INT16_AS_STRING);
	marshal_function ("uint16", DBUS_TYPE_UINT16_AS_STRING);
	marshal_function ("int32", DBUS_TYPE_INT32_AS_STRING);
	marshal_function ("uint32", DBUS_TYPE_UINT32_AS_STRING);
	marshal_function ("int64", DBUS_TYPE_INT64_AS_STRING);
	marshal_function ("uint64", DBUS_TYPE_UINT64_AS_STRING);
	marshal_function ("double", DBUS_TYPE_DOUBLE_AS_STRING);
	marshal_function ("string", DBUS_TYPE_STRING_AS_STRING);
	marshal_function ("object_path", DBUS_TYPE_OBJECT_PATH_AS_STRING);
	marshal_function ("signature", DBUS_TYPE_SIGNATURE_AS_STRING);
	marshal_function ("int16_array", (DBUS_TYPE_ARRAY_AS_STRING
					  DBUS_TYPE_INT16_AS_STRING));
	marshal_function ("int16_array_array", (DBUS_TYPE_ARRAY_AS_STRING
						DBUS_TYPE_ARRAY_AS_STRING
						DBUS_TYPE_INT16_AS_STRING));
	marshal_function ("string_array", (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING));
	marshal_function ("string_array_array", (DBUS_TYPE_ARRAY_AS_STRING
						 DBUS_TYPE_ARRAY_AS_STRING
						 DBUS_TYPE_STRING_AS_STRING));
	marshal_function ("struct", (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				     DBUS_TYPE_STRING_AS_STRING
				     DBUS_TYPE_UINT32_AS_STRING
				     DBUS_TYPE_ARRAY_AS_STRING
				     DBUS_TYPE_STRING_AS_STRING
				     DBUS_TYPE_ARRAY_AS_STRING
				     DBUS_TYPE_INT16_AS_STRING
				     DBUS_STRUCT_END_CHAR_AS_STRING));
	marshal_function ("struct_array", (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_STRUCT_BEGIN_CHAR_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_UINT32_AS_STRING
					   DBUS_STRUCT_END_CHAR_AS_STRING));
	marshal_function ("dict_entry_array", (DBUS_TYPE_ARRAY_AS_STRING
					       DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					       DBUS_TYPE_STRING_AS_STRING
					       DBUS_TYPE_UINT32_AS_STRING
					       DBUS_DICT_ENTRY_END_CHAR_AS_STRING));
	marshal_function ("unix_fd", DBUS_TYPE_UNIX_FD_AS_STRING);

	return 0;
}
