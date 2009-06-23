/* nih-dbus-tool
 *
 * indent.c - indentation and other code-style string changes
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

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/logging.h>

#include "indent.h"


/**
 * indent:
 * @str: pointer to string to be indented,
 * @parent: parent object of returned string,
 * @level: number of indents to add.
 *
 * Indents the given string @str with @level tab characters preceding
 * each non-empty line, including the last even if that has no terminating
 * newline.
 *
 * @str is modified directly, the returned string is simply a pointer to
 * it, thus @parent is actually ignored though it usual to pass the parent
 * of @str for style reasons.
 *
 * Returns: modified @str or NULL if insufficient memory.
 **/
char *
indent (char **     str,
	const void *parent,
	int         level)
{
	char * s;
	char * ret;
	size_t len;

	nih_assert (str != NULL);
	nih_assert (*str != NULL);
	nih_assert (level > 0);

	/* First figure out how many tab characters we have to insert */
	len = strlen (*str);
	if (**str != '\n')
		len += level;
	for (s = *str; *s; s++)
		if ((*s == '\n') && (s[1] != '\0') && (s[1] != '\n'))
			len += level;

	/* Increase the length of the string to fit */
	ret = nih_realloc (*str, parent, len + 1);
	if (! ret)
		return NULL;

	/* Now put the tab characters in */
	*str = ret;
	if (**str != '\n') {
		memmove (*str + level, *str, len + 1 - level);
		memset (*str, '\t', level);
	}
	for (s = *str; *s; s++) {
		if ((*s == '\n') && (s[1] != '\0') && (s[1] != '\n')) {
			memmove (s + level + 1, s + 1,
				 *str + len + 1 - level - s - 1);
			memset (s + 1, '\t', level);
		}
	}

	return *str;
}


/**
 * comment:
 * @str: pointer to string to be commented,
 * @parent: parent object of returned string.
 *
 * Applies commenting to the given @str, prefixing " * " onto each line
 * including the first and last.
 *
 * @str is modified directly, the returned string is simply a pointer to
 * it, thus @parent is actually ignored though it usual to pass the parent
 * of @str for style reasons.
 *
 * Returns: modified @str or NULL if insufficient memory.
 **/
char *
comment (char **     str,
	 const void *parent)
{
	char * s;
	char * ret;
	size_t len;

	nih_assert (str != NULL);
	nih_assert (*str != NULL);

	/* First figure out how many characters we have to insert */
	len = strlen (*str);
	if (**str == '\n') {
		len += 2;
	} else {
		len += 3;
	}
	for (s = *str; *s; s++) {
		if ((*s == '\n') && (s[1] != '\0')) {
			if (s[1] == '\n') {
				len += 2;
			} else {
				len += 3;
			}
		}
	}

	/* Increase the length of the string to fit */
	ret = nih_realloc (*str, parent, len + 1);
	if (! ret)
		return NULL;

	/* Now put the comment characters in */
	*str = ret;
	if (**str == '\n') {
		memmove (*str + 2, *str, len + 1 - 2);
		memcpy (*str, " *", 2);
	} else {
		memmove (*str + 3, *str, len + 1 - 3);
		memcpy (*str, " * ", 3);
	}
	for (s = *str; *s; s++) {
		if ((*s == '\n') && (s[1] != '\0')) {
			if (s[1] == '\n') {
				memmove (s + 2 + 1, s + 1,
					 *str + len + 1 - 2 - s - 1);
				memcpy (s + 1, " *", 2);
			} else {
				memmove (s + 3 + 1, s + 1,
					 *str + len + 1 - 3 - s - 1);
				memcpy (s + 1, " * ", 3);
			}
		}
	}

	return *str;
}
