/* nih-dbus-tool
 *
 * symbol.c - C symbol generation and validation
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


#include <ctype.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "symbol.h"


/**
 * symbol_valid:
 * @symbol: Symbol to verify.
 *
 * Verifies whether @symbol matches the rules for C symbol names.  To make
 * things easier for ourselves, we only support a subset of what C99 can
 * really support. ie. no universal character names.
 *
 * Returns: TRUE if valid, FALSE if not.
 **/
int
symbol_valid (const char *symbol)
{
	nih_assert (symbol != NULL);

	/* We can get away with just using strlen() here even through symbol
	 * is in UTF-8 because all the valid characters are ASCII.
	 */
	for (size_t i = 0; i < strlen (symbol); i++) {
		/* Symbols may contain digits, but not at the beginning. */
		if ((symbol[i] >= '0') && (symbol[i] <= '9')) {
			if (i == 0)
				return FALSE;

			continue;
		}

		/* Valid characters anywhere are [A-Za-z_] */
		if (   ((symbol[i] < 'A') || (symbol[i] > 'Z'))
		    && ((symbol[i] < 'a') || (symbol[i] > 'z'))
		    && (symbol[i] != '_'))
			return FALSE;
	}

	/* Symbol must be at least 1 character */
	if (strlen (symbol) < 1)
		return FALSE;

	return TRUE;
}


/**
 * symbol_from_name:
 * @parent: parent object of new string,
 * @name: name to convert.
 *
 * Converts the D-Bus style name @name to C style; basically the name
 * is lower-cased, and underscores inserted between CamelCase words.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if allocation fails.
 **/
char *
symbol_from_name (const void *parent,
		  const char *name)
{
	char * symbol;
	char * ptr;
	size_t len = 0;

	nih_assert (name != NULL);

	/* Figure out how long the symbol will need to be by counting
	 * how many places we need to add underscores and adding them
	 * to the length.
	 */
	len = strlen (name);
	for (size_t i = 0; i < strlen (name); i++)
		if ((i > 0) && (name[i] >= 'A') && (name[i] <= 'Z')
		    && (name[i-1] != '_')
		    && ((name[i-1] < 'A') || (name[i-1] > 'Z')))
			len++;

	/* Allocate the new string */
	symbol = nih_alloc (parent, len + 1);
	if (! symbol)
		return NULL;

	/* Copy the characters across, lowercasing as we go and inserting
	 * underscores before any capital unless it follows an underscore.
	 */
	ptr = symbol;
	for (size_t i = 0; i < strlen (name); i++) {
		if ((i > 0) && (name[i] >= 'A') && (name[i] <= 'Z')
		    && (name[i-1] != '_')
		    && ((name[i-1] < 'A') || (name[i-1] > 'Z')))
			*(ptr++) = '_';

		*(ptr++) = tolower (name[i]);
	}

	*ptr = '\0';

	return symbol;
}
