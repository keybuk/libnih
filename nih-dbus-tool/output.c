/* nih-dbus-tool
 *
 * output.c - source and header file output
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

#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>
#include <nih/main.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "type.h"
#include "node.h"
#include "interface.h"
#include "method.h"
#include "signal.h"
#include "property.h"
#include "output.h"


/* Prototypes for static functions */
static int   output_write    (int fd, const char *str)
	__attribute__ ((warn_unused_result));


/**
 * output_package:
 *
 * Package name to use when generating header and source file comments
 * and header file sentinel macro.  Defaults to libnih when not set.
 **/
char *output_package = NULL;


/**
 * output:
 * @source_path: path of source file to write,
 * @source_fd: file descriptor open to write to @source_path,
 * @header_path: path of header file to write,
 * @header_fd: file descriptor open to write to @header_path,
 * @prefix: prefix to prepend to symbols,
 * @node: node to output,
 * @object: whether to output for an object or proxy.
 *
 * Writes a valid C source file to @source_fd and its accompanying header
 * file to @header_fd; which should file descriptors open to writing to
 * @source_path and @header_path respectively.
 *
 * If @object is TRUE, the output code provides D-Bus bindings that wrap
 * externally defined C functions providing an implementation of @node.
 * When @object is FALSE, the output code instead provides API functions
 * that access a remote D-Bus object @node.
 *
 * Externally available symbols will all be prefixed with @prefix.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
output (const char *source_path,
	int         source_fd,
	const char *header_path,
	int         header_fd,
	const char *prefix,
	Node *      node,
	int         object)
{
	NihList         prototypes;
	NihList         handlers;
	NihList         structs;
	NihList         typedefs;
	NihList         vars;
	NihList         externs;
	nih_local char *array = NULL;
	nih_local char *code = NULL;
	nih_local char *source = NULL;
	nih_local char *header = NULL;
	nih_local char *sentinel = NULL;

	nih_assert (source_path != NULL);
	nih_assert (source_fd >= 0);
	nih_assert (header_path != NULL);
	nih_assert (header_fd >= 0);
	nih_assert (prefix != NULL);
	nih_assert (node != NULL);

	nih_list_init (&prototypes);
	nih_list_init (&handlers);
	nih_list_init (&structs);
	nih_list_init (&typedefs);
	nih_list_init (&vars);
	nih_list_init (&externs);

	/* Start off the text of the source file with the copyright preamble
	 * and the list of includes.
	 */
	source = output_preamble (NULL, source_path);
	if (! source) {
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! nih_strcat (&source, NULL,
			  "#ifdef HAVE_CONFIG_H\n"
			  "# include <config.h>\n"
			  "#endif /* HAVE_CONFIG_H */\n"
			  "\n"
			  "\n"
			  "#include <dbus/dbus.h>\n"
			  "\n"
			  "#include <stdint.h>\n"
			  "#include <string.h>\n"
			  "\n"
			  "#include <nih/macros.h>\n"
			  "#include <nih/alloc.h>\n"
			  "#include <nih/string.h>\n"
			  "#include <nih/logging.h>\n"
			  "#include <nih/error.h>\n"
			  "\n"
			  "#include <nih-dbus/dbus_error.h>\n"
			  "#include <nih-dbus/dbus_message.h>\n")) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Start off the text of the header file with the copyright preamble,
	 * sentinel and list of includes.
	 */
	header = output_preamble (NULL, NULL);
	if (! header) {
		nih_error_raise_no_memory ();
		return -1;
	}

	sentinel = output_sentinel (NULL, header_path);
	if (! sentinel) {
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! nih_strcat_sprintf (&header, NULL,
				  "#ifndef %s\n"
				  "#define %s\n"
				  "\n",
				  sentinel,
				  sentinel)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! nih_strcat (&header, NULL,
			  "#include <dbus/dbus.h>\n"
			  "\n"
			  "#include <stdint.h>\n"
			  "\n"
			  "#include <nih/macros.h>\n"
			  "\n"
			  "#include <nih-dbus/dbus_interface.h>\n"
			  "#include <nih-dbus/dbus_message.h>\n")) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Obtain the interfaces array for the source file */
	array = node_interfaces_array (NULL, prefix, node, object, &vars);
	if (! array) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Add any object/proxy-specific headers, and obtain the code
	 * for the functions, as well as the prototypes, typedefs, handler
	 * prototypes, extern prototypes, etc.
	 */
	if (object) {
		if (! nih_strcat (&source, NULL,
				  "#include <nih-dbus/dbus_object.h>\n")) {
			nih_error_raise_no_memory ();
			return -1;
		}

		code = node_object_functions (NULL, prefix, node,
					      &prototypes, &handlers,
					      &structs, &externs);
		if (! code) {
			nih_error_raise_no_memory ();
			return -1;
		}
	} else {
		if (! nih_strcat (&source, NULL,
				  "#include <nih-dbus/dbus_pending_data.h>\n"
				  "#include <nih-dbus/dbus_proxy.h>\n")) {
			nih_error_raise_no_memory ();
			return -1;
		}

		if (! nih_strcat (&header, NULL,
				  "#include <nih-dbus/dbus_pending_data.h>\n"
				  "#include <nih-dbus/dbus_proxy.h>\n")) {
			nih_error_raise_no_memory ();
			return -1;
		}

		code = node_proxy_functions (NULL, prefix, node,
					     &prototypes,
					     &structs, &typedefs, &externs);
		if (! code) {
			nih_error_raise_no_memory ();
			return -1;
		}
	}

	/* errors.h is always the last header by style, followed by the
	 * header itself.
	 */
	if (! nih_strcat_sprintf (&source, NULL,
				  "#include <nih-dbus/errors.h>\n"
				  "\n"
				  "#include \"%s\"\n"
				  "\n"
				  "\n",
				  header_path)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! nih_strcat (&header, NULL,
			  "\n"
			  "\n")) {
		nih_error_raise_no_memory ();
		return -1;
	}


	/* Declare the prototypes of static functions defined here in the
	 * source file.  These are the handler and getter/setter functions
	 * referred to in the array structures.
	 */
	if (! NIH_LIST_EMPTY (&prototypes)) {
		nih_local char *block = NULL;

		block = type_func_layout (NULL, &prototypes);
		if (! block) {
			nih_error_raise_no_memory ();
			return -1;
		}

		if (! nih_strcat_sprintf (&source, NULL,
					  "/* Prototypes for static functions */\n"
					  "%s"
					  "\n"
					  "\n",
					  block)) {
			nih_error_raise_no_memory ();
			return -1;
		}
	}

	/* Declare the prototypes of external handler functions that we
	 * expect other source files to implement.
	 */
	if (! NIH_LIST_EMPTY (&handlers)) {
		nih_local char *block = NULL;

		block = type_func_layout (NULL, &handlers);
		if (! block) {
			nih_error_raise_no_memory ();
			return -1;
		}

		if (! nih_strcat_sprintf (&source, NULL,
					  "/* Prototypes for externally implemented handler functions */\n"
					  "%s"
					  "\n"
					  "\n",
					  block)) {
			nih_error_raise_no_memory ();
			return -1;
		}
	}

	/* Define the arrays of methods and signals and their arguments,
	 * prototypes, interfaces, etc. for the node.  These refer to the
	 * above prototypes.
	 */
	if (! nih_strcat_sprintf (&source, NULL,
				  "%s"
				  "\n"
				  "\n",
				  array)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Finally append all of the function code.
	 */
	if (! nih_strcat (&source, NULL, code)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Write it */
	if (output_write (source_fd, source) < 0)
		return -1;


	/* Define each of the structures in the header file, each is
	 * a typdef so gets its own line.
	 */
	if (! NIH_LIST_EMPTY (&structs)) {
		NIH_LIST_FOREACH (&structs, iter) {
			TypeStruct *    structure = (TypeStruct *)iter;
			nih_local char *block = NULL;

			block = type_struct_to_string (NULL, structure);
			if (! block) {
				nih_error_raise_no_memory ();
				return -1;
			}

			if (! nih_strcat_sprintf (&header, NULL,
						  "%s"
						  "\n",
						  block)) {
				nih_error_raise_no_memory ();
				return -1;
			}
		}

		if (! nih_strcat (&header, NULL, "\n")) {
			nih_error_raise_no_memory ();
			return -1;
		}
	}

	/* Define each of the typedefs in the header file, some of these
	 * are actually required in the prototypes while others serve as
	 * documentation for what to pass to nih_dbus_proxy_connect()
	 */
	if (! NIH_LIST_EMPTY (&typedefs)) {
		NIH_LIST_FOREACH (&typedefs, iter) {
			TypeFunc *      func = (TypeFunc *)iter;
			nih_local char *block = NULL;

			block = type_func_to_typedef (NULL, func);
			if (! block) {
				nih_error_raise_no_memory ();
				return -1;
			}

			if (! nih_strcat_sprintf (&header, NULL,
						  "%s"
						  "\n",
						  block)) {
				nih_error_raise_no_memory ();
				return -1;
			}
		}

		if (! nih_strcat (&header, NULL, "\n")) {
			nih_error_raise_no_memory ();
			return -1;
		}
	}

	if (! nih_strcat (&header, NULL,
			  "NIH_BEGIN_EXTERN\n")) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Declare global variables defined in the source file, these are
	 * the interface structures and the array of them for the node.
	 */
	if (! NIH_LIST_EMPTY (&vars)) {
		nih_local char *block = NULL;

		block = type_var_layout (NULL, &vars);
		if (! block) {
			nih_error_raise_no_memory ();
			return -1;
		}

		if (! nih_strcat_sprintf (&header, NULL,
					  "\n"
					  "%s"
					  "\n",
					  block)) {
			nih_error_raise_no_memory ();
			return -1;
		}
	}

	/* Declare the prototypes of the functions defined in the source
	 * file.
	 */
	if (! NIH_LIST_EMPTY (&externs)) {
		nih_local char *block = NULL;

		block = type_func_layout (NULL, &externs);
		if (! block) {
			nih_error_raise_no_memory ();
			return -1;
		}

		if (! nih_strcat_sprintf (&header, NULL,
					  "\n"
					  "%s"
					  "\n",
					  block)) {
			nih_error_raise_no_memory ();
			return -1;
		}
	}

	if (! nih_strcat_sprintf (&header, NULL,
				  "NIH_END_EXTERN\n"
				  "\n"
				  "#endif /* %s */\n",
				  sentinel)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Write it */
	if (output_write (header_fd, header) < 0)
		return -1;

	return 0;
}

/**
 * output_preamble:
 * @parent: parent object for new string,
 * @path: path of source file.
 *
 * Generates the preamble header of a source or header file, containing the
 * package name of the software being built, @path if specified, the author's
 * copyright and a statement to see the source for copying conditions.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if allocation failed.
 **/
char *
output_preamble (const void *parent,
		 const char *path)
{
	char *code;

	code = nih_sprintf (parent, "/* %s\n *\n",
			    output_package ?: package_name);
	if (! code)
		return NULL;

	if (path) {
		if (! nih_strcat_sprintf (&code, parent,
					  " * %s - auto-generated D-Bus bindings\n"
					  " *\n",
					  path)) {
			nih_free (code);
			return NULL;
		}
	}

	if (! nih_strcat_sprintf (&code, parent,
				  " * %s\n"
				  " *\n"
				  " * This file was automatically generated; see the source for copying\n"
				  " * conditions.\n"
				  " */\n"
				  "\n",
				  package_copyright)) {
		nih_free (code);
		return NULL;
	}

	return code;
}

/**
 * output_sentinel:
 * @parent: parent object for new string,
 * @path: path of header file.
 *
 * Generates the name of header sentinel macro, used to ensure that a header
 * is not accidentally included twice (thus making out-of-order includes
 * possible).
 *
 * The name is the path, prefixed with the package name of the software being
 * built, uppercased and unrecognised characters replaced by underscores.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if allocation failed.
 **/
char *
output_sentinel (const void *parent,
		 const char *path)
{
	char *sentinel, *s;

	nih_assert (path != NULL);

	sentinel = nih_sprintf (parent, "%s_%s",
				output_package ?: package_name, path);
	if (! sentinel)
		return NULL;

	for (s = sentinel; *s; s++) {
		if (((*s < 'A') || (*s > 'Z'))
		    && ((*s < 'a') || (*s > 'z'))
		    && ((*s < '0') || (*s > '9'))) {
			*s = '_';
		} else {
			*s = toupper (*s);
		}
	}

	return sentinel;
}

/**
 * output_write:
 * @fd: file descriptor to write to,
 * @str: string to write.
 *
 * Wraps the write() syscall to ensure that the entire string @str is written
 * to @fd, since write() may perform short writes.
 *
 * Returns: zero on success, negative value on raised error.
 **/
static int
output_write (int         fd,
	      const char *str)
{
	ssize_t len;
	size_t  count;

	nih_assert (fd >= 0);
	nih_assert (str != NULL);

	count = strlen (str);

	while (count) {
		len = write (fd, str, count);
		if (len < 0) {
			nih_error_raise_system ();
			return -1;
		}

		count -= len;
		str += len;
	}

	return 0;
}
