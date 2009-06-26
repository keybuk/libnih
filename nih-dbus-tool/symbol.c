/* nih-dbus-tool
 *
 * symbol.c - C symbol generation and validation
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
#include <stdarg.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "symbol.h"


/* Prototypes for static functions */
static char *symbol_strcat_interface (char **str, const void *parent,
				      const char *format, ...)
	__attribute__ ((format (printf, 3, 4), warn_unused_result, malloc));
static char *symbol_strcat_title     (char **str, const void *parent,
				      const char *format, ...)
	__attribute__ ((format (printf, 3, 4), warn_unused_result, malloc));


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


/**
 * symbol_strcat_interface:
 * @str: pointer to string to modify,
 * @parent: parent object of new string,
 * @format: format to append to @str.
 *
 * Replaces periods in the string expanded from @format with underscores
 * and concatenates it onto @str.  The new string is allocated using
 * nih_alloc() and @str will be updated to point to the new string; use
 * the return value simply to check for success.
 *
 * @parent is ignored; though it is usual to pass a parent of @str for
 * style reasons.
 *
 * Returns: newly allocated string or NULL if allocation fails.
 **/
static char *
symbol_strcat_interface (char **     str,
			 const void *parent,
			 const char *format,
			 ...)
{
	nih_local char *name = NULL;
	va_list         args;
	size_t          len;
	char *          ret;

	nih_assert (str != NULL);
	nih_assert (*str != NULL);
	nih_assert (format != NULL);

	va_start (args, format);
	name = nih_vsprintf (NULL, format, args);
	if (! name)
		return NULL;
	va_end (args);

	len = strlen (*str);

	ret = nih_realloc (*str, parent, len + strlen (name) + 1);
	if (! ret)
		return NULL;

	*str = ret;

	/* Copy the characters across, replacing the periods between
	 * interface components with underscores.
	 */
	for (char *s = name; *s; s++)
		(*str)[len++] = (*s == '.' ? '_' : *s);

	(*str)[len] = '\0';

	return *str;
}

/**
 * symbol_strcat_title:
 * @str: pointer to string to modify,
 * @parent: parent object of new string,
 * @format: format to append to @str.
 *
 * Expands the @format string and modifies it so that each underscore-
 * separated word has the underscores removed and the initial character
 * uppercased.  The new string is allocated using nih_alloc() and @str
 * will be updated to point to the new string; use the return value
 * simply to check for success.
 *
 * If the string pointed to by @str is NULL, a new string will be
 * allocated and if @parent is not NULL, it should be a pointer to another
 * object which will be used as a parent for the returned string.  When all
 * parents of the returned string are freed, the returned string will also
 * be freed.
 *
 * When the string pointed to by @str is not NULL, @parent is ignored;
 * though it is usual to pass a parent of @str for style reasons.
 *
 * Returns: newly allocated string or NULL if allocation fails.
 **/
static char *
symbol_strcat_title (char **     str,
		     const void *parent,
		     const char *format,
		     ...)
{
	va_list         args;
	nih_local char *symbol = NULL;
	size_t          str_len;
	size_t          len;
	char *          ret;
	int             first = TRUE;

	nih_assert (str != NULL);
	nih_assert (format != NULL);

	va_start (args, format);
	symbol = nih_vsprintf (NULL, format, args);
	if (! symbol)
		return NULL;
	va_end (args);

	str_len = *str ? strlen (*str) : 0;
	len = str_len;

	/* Figure out how long the new string will be by counting how many
	 * characters that aren't underscores we'll end up adding.
	 */
	for (char *s = symbol; *s; s++)
		if (*s != '_')
			len++;

	ret = nih_realloc (*str, parent, len + 1);
	if (! ret)
		return NULL;

	*str = ret;

	len = str_len;

	/* Copy the characters across, uppercasing the first character of
	 * each word and stripping underscores.
	 */
	for (char *s = symbol; *s; s++) {
		if (*s == '_') {
			first = TRUE;
		} else {
			(*str)[len++] = (first ? toupper (*s) : *s);
			first = FALSE;
		}
	}

	(*str)[len] = '\0';

	return *str;
}

/**
 * symbol_impl:
 * @parent: parent object of new string,
 * @prefix: prefix for returned symbol,
 * @interface_name: name of D-Bus interface,
 * @name: name of interface member,
 * @postfix: postfix for returned symbol.
 *
 * Generates a C symbol for an implementation function, one that is hidden
 * from the API and thus uniqueness and verboseness is more desirable than
 * readability.  The @prefix is prepended to the @interface_name and
 * member @name, with the @postfix appended.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if allocation fails.
 **/
char *
symbol_impl (const void *parent,
	     const char *prefix,
	     const char *interface_name,
	     const char *name,
	     const char *postfix)
{
	char *str;

	nih_assert (prefix != NULL);
	nih_assert (interface_name != NULL);
	nih_assert ((postfix != NULL) || (name == NULL));

	str = nih_sprintf (parent, "%s_", prefix);
	if (! str)
		return NULL;

	if (! symbol_strcat_interface (&str, parent, (name ? "%s_" : "%s"),
				       interface_name)) {
		nih_free (str);
		return NULL;
	}

	if (name) {
		if (! nih_strcat (&str, parent, name)) {
			nih_free (str);
			return NULL;
		}
	}

	if (postfix) {
		if (! nih_strcat_sprintf (&str, parent, "_%s", postfix)) {
			nih_free (str);
			return NULL;
		}
	}

	return str;
}

/**
 * symbol_extern:
 * @parent: parent object of new string,
 * @prefix: prefix for returned symbol,
 * @interface_symbol: symbol for D-Bus interface,
 * @midfix: midfix for returned symbol,
 * @symbol: symbol of interface member,
 * @postfix: postfix for returned symbol.
 *
 * Generates a C symbol for an external function, one that is either part of
 * the API or intended to be supplied externally, thus where readability
 * is more desirable than uniqueness or verboseness.  The @prefix is
 * prepended to the @interface_symbol (if supplied), @midfix (if supplied),
 * member @symbol, with the @postfix (if supplied) appended.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if allocation fails.
 **/
char *
symbol_extern (const void *parent,
	       const char *prefix,
	       const char *interface_symbol,
	       const char *midfix,
	       const char *symbol,
	       const char *postfix)
{
	char *str;

	nih_assert (prefix != NULL);
	nih_assert (symbol != NULL);

	str = nih_sprintf (parent, "%s_", prefix);
	if (! str)
		return NULL;

	if (interface_symbol) {
		if (! nih_strcat_sprintf (&str, parent, "%s_",
					  interface_symbol)) {
			nih_free (str);
			return NULL;
		}
	}

	if (midfix) {
		if (! nih_strcat_sprintf (&str, parent, "%s_", midfix)) {
			nih_free (str);
			return NULL;
		}
	}

	if (! nih_strcat (&str, parent, symbol)) {
		nih_free (str);
		return NULL;
	}

	if (postfix) {
		if (! nih_strcat_sprintf (&str, parent, "_%s", postfix)) {
			nih_free (str);
			return NULL;
		}
	}

	return str;
}

/**
 * symbol_typedef:
 * @parent: parent object of new string,
 * @prefix: prefix for returned symbol,
 * @interface_symbol: symbol for D-Bus interface,
 * @midfix: midfix for returned symbol,
 * @symbol: symbol of interface member,
 * @postfix: postfix for returned symbol.
 *
 * Generates a C typedef name for a function that is expected to be
 * supplied, this has the same basic form as an external symbol except that
 * underscores are removed and the first letter of each part is uppercased.
 * The @prefix is prepended to the @interface_symbol (if supplied), @midfix
 * (if supplied), member @symbol, with the @postfix appended.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if allocation fails.
 **/
char *
symbol_typedef (const void *parent,
		const char *prefix,
		const char *interface_symbol,
		const char *midfix,
		const char *symbol,
		const char *postfix)
{
	char *str;

	nih_assert (prefix != NULL);
	nih_assert (symbol != NULL);

	str = NULL;
	if (! symbol_strcat_title (&str, parent, "%s_", prefix))
		return NULL;

	if (interface_symbol) {
		if (! symbol_strcat_title (&str, parent, "%s_",
					   interface_symbol)) {
			nih_free (str);
			return NULL;
		}
	}

	if (midfix) {
		if (! symbol_strcat_title (&str, parent, "%s_", midfix)) {
			nih_free (str);
			return NULL;
		}
	}

	if (! symbol_strcat_title (&str, parent, "%s", symbol)) {
		nih_free (str);
		return NULL;
	}

	if (postfix) {
		if (! symbol_strcat_title (&str, parent, "_%s", postfix)) {
			nih_free (str);
			return NULL;
		}
	}

	return str;
}
