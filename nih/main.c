/* libnih
 *
 * main.c - main loop handling and functions often called from main()
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
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
#include <stdlib.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/logging.h>

#include "main.h"


/**
 * program_name:
 *
 * The name of the program, taken from the argument array with the directory
 * name portion stripped.
 **/
const char *program_name = NULL;

/**
 * package_name:
 *
 * The name of the overall package, usually set to the autoconf PACKAGE_NAME
 * macro.  This should be used in preference.
 **/
const char *package_name = NULL;

/**
 * package_version:
 *
 * The version of the overall package, thus also the version of the program.
 * Usually set to the autoconf PACKAGE_VERSION macro.  This should be used
 * in preference.
 **/
const char *package_version = NULL;

/**
 * package_copyright:
 *
 * The copyright message for the package, taken from the autoconf
 * AC_COPYRIGHT macro.
 **/
const char *package_copyright = NULL;

/**
 * package_bugreport:
 *
 * The e-mail address to report bugs on the package to, taken from the
 * autoconf PACKAGE_BUGREPORT macro.
 **/
const char *package_bugreport = NULL;


/**
 * nih_main_init_full:
 * @argv0: program name from arguments,
 * @package: package name from configure,
 * @version: package version from configure,
 * @bugreport: bug report address from configure,
 * @copyright: package copyright message from configure.
 *
 * Should be called at the beginning of #main to initialise the various
 * global variables exported from this module.  For autoconf-using packages
 * call the #nih_main_init macro instead.
 **/
void
nih_main_init_full (const char *argv0,
		    const char *package,
		    const char *version,
		    const char *bugreport,
		    const char *copyright)
{
	nih_assert (argv0 != NULL);
	nih_assert (package != NULL);
	nih_assert (version != NULL);

	/* Only take the basename of argv0 */
	program_name = strrchr (argv0, '/');
	if (program_name) {
		program_name++;
	} else {
		program_name = argv0;
	}

	package_name = package;
	package_version = version;

	/* bugreport and copyright may be NULL/empty */
	if (bugreport && *bugreport)
		package_bugreport = bugreport;
	if (copyright && *copyright)
		package_copyright = copyright;
}


/**
 * nih_main_package_string:
 *
 * Compares the invoked program name against the package name, producing
 * a string in the form "program (package version)" if they differ or
 * "program version" if they match.
 *
 * Returns: internal copy of the string.
 **/
const char *
nih_main_package_string (void)
{
	static char *package_string = NULL;
	size_t       len;

	nih_assert (program_name != NULL);

	if (strcmp (program_name, package_name)) {
		len = snprintf (NULL, 0, "%s (%s %s)",
				program_name, package_name, package_version);
		if (len < 0)
			return program_name;

		package_string = realloc (package_string, len + 1);
		if (! package_string)
			return program_name;

		snprintf (package_string, len + 1, "%s (%s %s)",
			  program_name, package_name, package_version);
	} else {
		len = snprintf (NULL, 0, "%s %s",
				package_name, package_version);
		if (len < 0)
			return package_name;

		package_string = realloc (package_string, len + 1);
		if (! package_string)
			return package_name;

		snprintf (package_string, len + 1, "%s %s",
			  package_name, package_version);
	}

	return package_string;
}

/**
 * nih_main_suggest_help:
 *
 * Print a message suggesting --help to stderr.
 **/
void
nih_main_suggest_help (void)
{
	nih_assert (program_name != NULL);

	fprintf (stderr, _("Try `%s --help' for more information.\n"),
		 program_name);
}

/**
 * nih_main_version:
 *
 * Print the program version to stdout.
 **/
void
nih_main_version (void)
{
	nih_assert (program_name != NULL);

	printf ("%s\n", nih_main_package_string ());
	if (package_copyright)
		printf ("%s\n", package_copyright);
	printf ("\n");
	printf (_("This is free software; see the source for copying conditions.  There is NO\n"
		  "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"));
}
