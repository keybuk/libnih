/* nih-dbus-tool
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


#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/main.h>
#include <nih/option.h>
#include <nih/logging.h>

#include "node.h"
#include "parse.h"


/**
 * OutputMode:
 *
 * The tool can either generate code for a local object implementation
 * wrapping existing C functions (OUTPUT_OBJECT) or code for a proxy for
 * a remote object providing C access methods (OUTPUT_PROXY);
 **/
typedef enum output_mode {
	OUTPUT_OBJECT,
	OUTPUT_PROXY
} OutputMode;


/* Prototypes for option functions */
int mode_option (NihOption *option, const char *arg);

/* Prototypes for local functions */
char *source_file_path (const void *parent, const char *output_path,
			const char *filename)
	__attribute__ ((warn_unused_result, malloc));
char *header_file_path (const void *parent, const char *output_path,
			const char *filename)
	__attribute__ ((warn_unused_result, malloc));


/**
 * mode_option:
 * @option: NihOption invoked,
 * @arg: argument to parse.
 *
 * This option setter parses the output mode argument @arg and sets
 * the value member of @option, which must be a pointer to an OutputMode
 * enum.
 *
 * The arg_name member of @option must not be NULL.
 **/
int
mode_option (NihOption * option,
	     const char *arg)
{
	OutputMode *value;

	nih_assert (option != NULL);
	nih_assert (option->value != NULL);
	nih_assert (arg != NULL);

	value = (OutputMode *)option->value;

	if (! strcmp (arg, "object")) {
		*value = OUTPUT_OBJECT;
	} else if (! strcmp (arg, "proxy")) {
		*value = OUTPUT_PROXY;
	} else {
		fprintf (stderr, _("%s: illegal output mode: %s\n"),
			 program_name, arg);
		nih_main_suggest_help ();
		return -1;
	}

	return 0;
}


/**
 * source_file_path:
 * @parent: parent object for new string,
 * @output_path: output path,
 * @filename: input filename.
 *
 * Generates a path to the output source (.c) file from either the output
 * path given in @output_path or the input filename given in @filename,
 * depending on which one is not NULL.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
source_file_path (const void *parent,
		  const char *output_path,
		  const char *filename)
{
	char *path;

	nih_assert ((output_path != NULL) || (filename != NULL));

	if (output_path) {
		char *ptr;

		/* When the output path is given, return it; but allow for
		 * the output path being the header to make Makefile rules
		 * easier, and replace extension with .c when given one.
		 */
		ptr = strrchr (output_path, '.');
		if (ptr && (! strcmp (ptr, ".h"))) {
			path = nih_strndup (parent, output_path,
					    ptr - output_path);
			if (! path)
				return NULL;

			if (! nih_strcat (&path, parent, ".c")) {
				nih_free (path);
				return NULL;
			}
		} else {
			path = nih_strdup (parent, output_path);
		}

	} else if (filename) {
		char *ptr;

		/* Always output to the current directory */
		ptr = strrchr (filename, '/');
		if (ptr)
			filename = ptr + 1;

		/* When the input filename is given, strip the extension off
		 * and replace with .c unless the extension is .c or .h in
		 * which case we append the .c extension instead
		 */
		ptr = strrchr (filename, '.');
		if (ptr && strcmp (ptr, ".c") && strcmp (ptr, ".h")) {
			path = nih_strndup (parent, filename, ptr - filename);
			if (! path)
				return NULL;
		} else {
			path = nih_strdup (parent, filename);
			if (! path)
				return NULL;
		}

		if (! nih_strcat (&path, parent, ".c")) {
			nih_free (path);
			return NULL;
		}
	}

	return path;
}

/**
 * header_file_path:
 * @parent: parent object for new string,
 * @output_path: output path,
 * @filename: input filename.
 *
 * Generates a path to the output header (.h) file from either the output
 * path given in @output_path or the input filename given in @filename,
 * depending on which one is not NULL.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
header_file_path (const void *parent,
		  const char *output_path,
		  const char *filename)
{
	char *path;

	nih_assert ((output_path != NULL) || (filename != NULL));

	if (output_path) {
		char *ptr;

		/* When the output path is given, and is the header file,
		 * return it; otherwise replace the extension with .h or
		 * append it if there was no extension
		 */
		ptr = strrchr (output_path, '.');
		if (ptr && (! strcmp (ptr, ".h"))) {
			path = nih_strdup (parent, output_path);
		} else if (ptr) {
			path = nih_strndup (parent, output_path,
					    ptr - output_path);
			if (! path)
				return NULL;

			if (! nih_strcat (&path, parent, ".h")) {
				nih_free (path);
				return NULL;
			}
		} else {
			path = nih_strdup (parent, output_path);
			if (! path)
				return NULL;

			if (! nih_strcat (&path, parent, ".h")) {
				nih_free (path);
				return NULL;
			}
		}


	} else if (filename) {
		char *ptr;

		/* Always output to the current directory */
		ptr = strrchr (filename, '/');
		if (ptr)
			filename = ptr + 1;

		/* When the input filename is given, strip the extension off
		 * and replace with .h unless the extension is .c or .h in
		 * which case we append the .h extension instead
		 */
		ptr = strrchr (filename, '.');
		if (ptr && strcmp (ptr, ".c") && strcmp (ptr, ".h")) {
			path = nih_strndup (parent, filename, ptr - filename);
			if (! path)
				return NULL;
		} else {
			path = nih_strdup (parent, filename);
			if (! path)
				return NULL;
		}

		if (! nih_strcat (&path, parent, ".h")) {
			nih_free (path);
			return NULL;
		}
	}

	return path;
}


#ifndef TEST
/**
 * output_mode:
 *
 * Output mode; set to OUTPUT_OBJECT to output code for a local object
 * implementation wrapping existing C functions or OUTPUT_PROXY to output
 * code for a remote object providing C access methods.
 **/
static OutputMode output_mode = OUTPUT_OBJECT;

/**
 * prefix:
 *
 * Prefix of expected and generated functions.
 **/
static const char *prefix = NULL;

/**
 * output_path:
 *
 * Path to output C code to, header is automatically placed alongside.
 **/
static const char *output_path = NULL;


/**
 * options:
 *
 * Command-line options accepted by this tool.
 **/
static NihOption options[] = {
	{ 0,   "mode", N_("output mode: object, or proxy [default: object]"),
	  NULL, "MODE", &output_mode, mode_option },
	{ 0,   "prefix", N_("prefix for C functions [default: dbus]"),
	  NULL, "PREFIX", &prefix, NULL },
	{ 'o', "output", N_("write C source to FILENAME, header alongside"),
	  NULL, "FILENAME", &output_path, NULL },

	NIH_OPTION_LAST
};


int
main (int   argc,
      char *argv[])
{
	char **args;
	char * filename;
	char * path = NULL;
	Node * node;

	nih_main_init (argv[0]);

	nih_option_set_usage (_("[XMLFILE]"));
	nih_option_set_synopsis (_("Generate C bindings for D-Bus objects"));
	nih_option_set_help (_("Fill this in later"));

	args = nih_option_parser (NULL, argc, argv, options, FALSE);
	if (! args)
		exit (1);

	/* Filename defaults to standard input when not specified, or when
	 * specified as "-".
	 */
	filename = args[0];
	if (filename && (! strcmp (filename, "-")))
		filename = NULL;

	/* Output path must be specified when we're using standard input */
	if ((! filename) && (! output_path)) {
		fprintf (stderr, _("%s: --output must be specified when using standard input\n"),
			 program_name);
		nih_main_suggest_help ();
		exit (1);
	}

	/* Set default prefix if not set */
	if (! prefix)
		prefix = "dbus";


	/* Parse the input file, which may be standard input */
	if (filename) {
		int fd;

		fd = open (filename, O_RDONLY);
		if (fd < 0) {
			nih_error ("%s: %s: %s", filename,
				   _("Unable to open"),
				   strerror (errno));
			exit (1);
		}

		node = parse_xml (NULL, fd, filename);
		if (! node)
			exit (1);
	} else {
		node = parse_xml (NULL, STDIN_FILENO, "(standard input)");
		if (! node)
			exit (1);
	}

	/* Output source file */
	path = source_file_path (NULL, output_path, filename);
	if (! path) {
		nih_error ("%s", strerror (ENOMEM));
		exit (1);
	}

	/* FIXME */

	nih_free (path);

	/* Output header file */
	path = header_file_path (NULL, output_path, filename);
	if (! path) {
		nih_error ("%s", strerror (ENOMEM));
		exit (1);
	}

	/* FIXME */

	nih_free (path);

	return 0;
}
#endif
