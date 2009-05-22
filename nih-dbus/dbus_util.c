/* libnih
 *
 * dbus_util.c - D-Bus utility functions
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
#include <stdarg.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/logging.h>

#include "dbus_util.h"


/**
 * nih_dbus_path:
 * @parent: parent object for new string,
 * @root: root of path.
 *
 * Generates a D-Bus path suitable for object registration rooted at
 * @root with each of the further elements joined with "/" separators and
 * appended after non-permissible characters are removed.
 *
 * The final argument to this function must be NULL to signify the end
 * of elements.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
nih_dbus_path (const void *parent,
	       const char *root,
	       ...)
{
	const char *arg;
	const char *ptr;
	char *      path;
	va_list     args;
	size_t      len;

	nih_assert (root != NULL);

	/* First work out how much space we'll need */
	len = strlen (root);

	va_start (args, root);
	for (arg = va_arg (args, const char *); arg != NULL;
	     arg = va_arg (args, const char *)) {
		len += 1;

		if (! *arg)
			len += 1;

		for (ptr = arg; *ptr != '\0'; ptr++) {
			if (   ((*ptr >= 'a') && (*ptr <= 'z'))
			    || ((*ptr >= 'A') && (*ptr <= 'Z'))
			    || ((*ptr >= '0') && (*ptr <= '9'))) {
				len += 1;
			} else {
				len += 3;
			}
		}
	}
	va_end (args);

	/* Now we can allocate it */
	path = nih_alloc (parent, len + 1);
	if (! path)
		return NULL;

	/* And copy the elements in */
	strcpy (path, root);
	len = strlen (root);

	va_start (args, root);
	for (arg = va_arg (args, const char *); arg != NULL;
	     arg = va_arg (args, const char *)) {
		path[len++] = '/';

		if (! *arg)
			path[len++] = '_';

		for (ptr = arg; *ptr != '\0'; ptr++) {
			if (   ((*ptr >= 'a') && (*ptr <= 'z'))
			    || ((*ptr >= 'A') && (*ptr <= 'Z'))
			    || ((*ptr >= '0') && (*ptr <= '9'))) {
				path[len++] = *ptr;
			} else {
				path[len++] = '_';

				sprintf (path + len, "%02x", *ptr);
				len += 2;
			}
		}
	}
	va_end (args);

	path[len] = '\0';

	return path;
}
