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
#include <string.h>
#include <assert.h>

#include "main.h"


/* Program name, taken from argv[0] */
const char *program_name = NULL;

/* Package name, taken from configure */
const char *package_name = NULL;

/* Package version, taken from configure */
const char *package_version = NULL;

/* Package copyright, taken from configure */
const char *package_copyright = NULL;

/* Package bug report address, taken from configure */
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
	assert (argv0 != NULL);
	assert (package != NULL);
	assert (version != NULL);

	program_name = argv0;
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
 * Returns: constant string.
 **/
const char *
nih_main_package_string (void)
{
	static char  package_string[81] = "\0";
	const char  *ptr;

	assert (program_name != NULL);

	ptr = strrchr (program_name, '/');
	ptr = ptr ? ptr + 1 : program_name;

	if (strcmp (ptr, package_name)) {
		snprintf (package_string, sizeof (package_string),
			  "%s (%s %s)", ptr, package_name, package_version);
	} else {
		snprintf (package_string, sizeof (package_string),
			  "%s %s", package_name, package_version);
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
	assert (program_name != NULL);

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
	assert (program_name != NULL);

	printf ("%s\n", nih_main_package_string ());
	if (package_copyright)
		printf ("%s\n", package_copyright);
	printf ("\n");
	printf (_("This is free software; see the source for copying conditions.  There is NO\n"
		  "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"));
}
