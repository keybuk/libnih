/* libnih
 *
 * string.c - useful string utility functions
 *
 * Copyright © 2007 Scott James Remnant <scott@netsplit.com>.
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


#include <sys/ioctl.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/logging.h>

#include "string.h"


/**
 * nih_sprintf:
 * @parent: parent block of allocation,
 * @format: format string.
 *
 * Writes a new string according to @format as sprintf(), except that the
 * string is allocated using nih_alloc().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
nih_sprintf (const void *parent,
	     const char *format,
	     ...)
{
	char    *str;
	va_list  args;

	nih_assert (format != NULL);

	va_start (args, format);
	str = nih_vsprintf (parent, format, args);
	va_end (args);

	return str;
}

/**
 * nih_vsprintf:
 * @parent: parent block of allocation,
 * @format: format string,
 * @args: arguments to format string.
 *
 * Writes a new string according to @format as vsprintf(), except that the
 * string is allocated using nih_alloc().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
nih_vsprintf (const void *parent,
	      const char *format,
	      va_list     args)
{
	ssize_t   len;
	va_list   args_copy;
	char     *str;

	nih_assert (format != NULL);

	va_copy (args_copy, args);
	len = vsnprintf (NULL, 0, format, args_copy);

	nih_assert (len >= 0);

	str = nih_alloc (parent, len + 1);
	if (! str)
		return NULL;

	va_copy (args_copy, args);
	vsnprintf (str, len + 1, format, args_copy);

	return str;
}


/**
 * nih_strdup:
 * @parent: parent block of allocation,
 * @str: string to duplicate.
 *
 * Allocates enough memory to store a duplicate of @str and writes a
 * copy of the string to it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: duplicated string or NULL if insufficient memory.
 **/
char *
nih_strdup (const void *parent,
	    const char *str)
{
	size_t len;

	nih_assert (str != NULL);

	len = strlen (str);
	return nih_strndup (parent, str, len);
}

/**
 * nih_strndup:
 * @parent: parent block of allocation,
 * @str: string to duplicate,
 * @len: length of string to duplicate.
 *
 * Allocates enough memory to store up to @len bytes of @str, or if @str
 * is shorter, @len bytes.  A copy of the string is written to this
 * block with a NUL byte appended.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: duplicated string or NULL if insufficient memory.
 **/
char *
nih_strndup (const void *parent,
	     const char *str,
	     size_t      len)
{
	char *dup;

	nih_assert (str != NULL);

	dup = nih_alloc (parent, len + 1);
	if (! dup)
		return NULL;

	memset (dup, '\0', len + 1);
	strncpy (dup, str, len);

	return dup;
}


/**
 * nih_str_split:
 * @parent: parent of returned array,
 * @str: string to split,
 * @delim: characters to split on,
 * @repeat: allow repeated characters.
 *
 * Splits @str into an array of strings by separating on any character in
 * @delim; if @repeat is true then sequences of @delim are ignored, otherwise
 * they result in empty strings in the returned array.
 *
 * The last element in the array is always NULL.
 *
 * The individual strings are allocated using nih_alloc() so you may just use
 * nih_free() on the returned array and must NOT use nih_strv_free().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: allocated array or NULL if insufficient memory.
 **/
char **
nih_str_split (const void *parent,
	       const char *str,
	       const char *delim,
	       int         repeat)
{
	char   **array;
	size_t   len;

	nih_assert (str != NULL);
	nih_assert (delim != NULL);

	len = 0;
	array = nih_str_array_new (parent);
	if (! array)
		return NULL;

	while (*str) {
		const char  *ptr;

		/* Skip initial delimiters */
		while (repeat && strchr (delim, *str))
			str++;

		/* Find the end of the token */
		ptr = str;
		while (*str && (! strchr (delim, *str)))
			str++;

		if (! nih_str_array_addn (&array, parent, &len,
					  ptr, str - ptr)) {
			nih_free (array);
			return NULL;
		}

		/* Skip over the delimiter */
		if (*str)
			str++;
	}

	return array;
}

/**
 * nih_str_array_new:
 * @parent: parent of array.
 *
 * Allocates a new NULL-terminated array of strings with zero elements;
 * use nih_str_array_add() to append new strings to the array.  Because
 * each array element will be allocated using nih_alloc() as a child of
 * the array itself, the entire array can be freed with nih_free().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: newly allocated array or NULL if insufficient memory.
 **/
char **
nih_str_array_new (const void *parent)
{
	char **array;

	array = nih_alloc (parent, sizeof (char *));
	if (! array)
		return NULL;

	array[0] = NULL;

	return array;
}

/**
 * nih_str_array_add:
 * @array: array of strings,
 * @parent: parent of @array,
 * @len: length of @array,
 * @str: string to add.
 *
 * Extend the NULL-terminated string @array (which has @len elements,
 * excluding the final NULL element), appending a copy of @str to it.
 * Both the array and the new string are allocated using nih_alloc(),
 * @parent must be that of @array.
 *
 * @len will be updated to contain the new array length and @array will
 * be updated to point to the new array pointer; use the return value
 * simply to check for success.
 *
 * If you don't know or care about the length, @len may be set to NULL;
 * this is less efficient as it necessates counting the length on each
 * invocation.
 *
 * Returns: new array pointer or NULL if insufficient memory.
 **/
char **
nih_str_array_add (char       ***array,
		   const void   *parent,
		   size_t       *len,
		   const char   *str)
{
	char  *new_str;
	char **new_array;

	nih_assert (array != NULL);
	nih_assert (str != NULL);

	new_str = nih_strdup (*array, str);
	if (! new_str)
		return NULL;

	new_array = nih_str_array_addp (array, parent, len, new_str);
	if (! new_array) {
		nih_free (new_str);
		return NULL;
	}

	return new_array;
}

/**
 * nih_str_array_addn:
 * @array: array of strings,
 * @parent: parent of @array,
 * @len: length of @array,
 * @str: string to add,
 * @strlen: length of @str.
 *
 * Extend the NULL-terminated string @array (which has @len elements,
 * excluding the final NULL element), appending a copy of the first
 * @strlen bytes of @str to it.
 *
 * Both the array and the new string are allocated using nih_alloc(),
 * @parent must be that of @array.
 *
 * @len will be updated to contain the new array length and @array will
 * be updated to point to the new array pointer; use the return value
 * simply to check for success.
 *
 * If you don't know or care about the length, @len may be set to NULL;
 * this is less efficient as it necessates counting the length on each
 * invocation.
 *
 * Returns: new array pointer or NULL if insufficient memory.
 **/
char **
nih_str_array_addn (char       ***array,
		    const void   *parent,
		    size_t       *len,
		    const char   *str,
		    size_t        strlen)
{
	char  *new_str;
	char **new_array;

	nih_assert (array != NULL);
	nih_assert (str != NULL);

	new_str = nih_strndup (*array, str, strlen);
	if (! new_str)
		return NULL;

	new_array = nih_str_array_addp (array, parent, len, new_str);
	if (! new_array) {
		nih_free (new_str);
		return NULL;
	}

	return new_array;
}

/**
 * nih_str_array_addp:
 * @array: array of strings,
 * @parent: parent of @array,
 * @len: length of @array,
 * @ptr: pointer to add.
 *
 * Extend the NULL-terminated string @array (which has @len elements,
 * excluding the final NULL element), appending the nih_alloc() allocated
 * block @ptr to it.
 *
 * The array is allocated using nih_alloc(), @parent must be that of @array;
 * @ptr will be reparented to be a child of the new array.
 *
 * @len will be updated to contain the new array length and @array will
 * be updated to point to the new array pointer; use the return value
 * simply to check for success.
 *
 * If you don't know or care about the length, @len may be set to NULL;
 * this is less efficient as it necessates counting the length on each
 * invocation.
 *
 * Returns: new array pointer or NULL if insufficient memory.
 **/
char **
nih_str_array_addp (char       ***array,
		    const void   *parent,
		    size_t       *len,
		    void         *ptr)
{
	char   **new_array;
	size_t   c_len;

	nih_assert (array != NULL);
	nih_assert (ptr != NULL);

	if (! len) {
		len = &c_len;
		c_len = 0;

		for (new_array = *array; new_array && *new_array; new_array++)
			c_len++;
	}

	new_array = nih_realloc (*array, parent, sizeof (char *) * (*len + 2));
	if (! new_array)
		return NULL;

	*array = new_array;

	nih_alloc_reparent (ptr, *array);

	(*array)[(*len)++] = ptr;
	(*array)[*len] = NULL;

	return *array;
}

/**
 * nih_strv_free:
 * @strv: array of strings:
 *
 * Free the given array of strings which should NOT have been allocated
 * using nih_alloc() (as you could just free the parent array if you used
 * that).
 *
 * The last member of the array should be NULL, and the array itself is
 * not freed.
 **/
void
nih_strv_free (char **strv)
{
	register char **s;

	for (s = strv; *s; s++)
		free (*s);
}


/**
 * nih_str_wrap:
 * @parent: parent of returned string,
 * @str: string to be wrapped,
 * @len: length of line to fit into,
 * @first_indent: indent for first line,
 * @indent: indent for subsequent lines.
 *
 * Returns a newly allocated copy of @str with newlines inserted so no
 * line is longer than @len characters (not including the newline).  Where
 * possible, newlines replace existing whitespace characters so that words
 * are not broken.
 *
 * The first line may be indented by an extra @first_indent characters, and
 * subsequent lines may be intended by an extra @indent characters.  These
 * are added to the string as whitespace characters.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
nih_str_wrap (const void *parent,
	      const char *str,
	      size_t      len,
	      size_t      first_indent,
	      size_t      indent)
{
	char   *txt;
	size_t  txtlen, col, ls, i;

	nih_assert (str != NULL);
	nih_assert (len > 0);

	txtlen = first_indent + strlen (str);
	txt = nih_alloc (parent, txtlen + 1);
	if (! txt)
		return NULL;

	memset (txt, ' ', first_indent);
	memcpy (txt + first_indent, str, strlen (str) + 1);

	col = ls = 0;
	for (i = 0; i < txtlen; i++) {
		int nl = 0;

		if (strchr (" \t\r", txt[i])) {
			/* Character is whitespace; convert to an ordinary
			 * space and remember the position for next time.
			 */
			txt[i] = ' ';
			ls = i;

			/* If this doesn't go over the line length,
			 * continue to the next character
			 */
			if (++col <= len)
				continue;
		} else if (txt[i] != '\n') {
			/* Character is part of a word.  If this doesn't go
			 * over the line length, continue to the next
			 * character
			 */
			if (++col <= len)
				continue;

			/* Filled a line; if we marked a whitespace character
			 * on this line, go back to that, otherwise we'll
			 * need to add a newline to the string after this
			 * character
			 */
			if (ls) {
				i = ls;
			} else {
				nl = 1;
			}
		}

		/* We need to insert a line break at this position, and
		 * any indent that goes along with it
		 */
		if (indent | nl) {
			char *new_txt;

			/* Need to increase the size of the string in memory */
			new_txt = nih_realloc (txt, parent,
					       txtlen + indent + nl + 1);
			if (! new_txt) {
				nih_free (txt);
				return NULL;
			}
			txt = new_txt;

			/* Move up the existing characters, then replace
			 * the gap with the indent
			 */
			memmove (txt + i + indent + 1, txt + i + 1 - nl,
				 txtlen - i + nl);
			memset (txt + i + 1, ' ', indent);
			txtlen += indent + nl;
		}

		/* Replace the current character with a newline */
		txt[i] = '\n';

		/* Reset the current column and last seen whitespace index;
		 * make sure we skip any indent as it's whitespace that doesn't
		 * count
		 */
		i += indent;
		col = indent;
		ls = 0;
	}

	return txt;
}

/**
 * nih_str_screen_width:
 *
 * Checks the COLUMNS environment variable, standard output if it is a
 * terminal or defaults to 80 characters.
 *
 * Returns: the width of the screen.
 **/
size_t
nih_str_screen_width (void)
{
	char   *columns;
	size_t  len = 0;

	/* Look at the columns environment variable */
	columns = getenv ("COLUMNS");
	if ((! len) && columns) {
		char *endptr;

		len = strtoul (columns, &endptr, 10);
		if (*endptr)
			len = 0;
	}

	/* Check whether standard output is a tty */
	if ((! len) && isatty (STDOUT_FILENO)) {
		struct winsize winsize;

		if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &winsize) == 0)
			len = winsize.ws_col;
	}

	/* Fallback to 80 columns */
	if (! len)
		len = 80;

	return len;
}

/**
 * nih_str_screen_wrap:
 * @parent: parent of returned string,
 * @str: string to be wrapped,
 * @first_indent: indent for first line,
 * @indent: indent for subsequent lines.
 *
 * Returns a newly allocated copy of @str with newlines inserted so no
 * line is wider than the screen (not including the newline).  Where
 * possible, newlines replace existing whitespace characters so that words
 * are not broken.
 *
 * If standard output is not a terminal, then 80 characters is assumed.
 * The width can be overriden with the COLUMNS environment variable.
 *
 * The first line may be indented by an extra @first_indent characters, and
 * subsequent lines may be intended by an extra @indent characters.  These
 * are added to the string as whitespace characters.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
nih_str_screen_wrap (const void *parent,
		     const char *str,
		     size_t      first_indent,
		     size_t      indent)
{
	size_t len;

	nih_assert (str != NULL);

	len = nih_str_screen_width () - 1;

	return nih_str_wrap (parent, str, len, first_indent, indent);
}
