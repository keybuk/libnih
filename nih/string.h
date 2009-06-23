/* libnih
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

#ifndef NIH_STRING_H
#define NIH_STRING_H

/**
 * These functions provide string utilities that use the nih_alloc()
 * functions for memory management.  This allows you to create and modify
 * strings, and arrays of strings, which may be referenced by other objects
 * and cleaned up automatically.
 **/

#include <stdarg.h>

#include <nih/macros.h>


NIH_BEGIN_EXTERN

char * nih_sprintf          (const void *parent, const char *format, ...)
	__attribute__ ((format (printf, 2, 3), warn_unused_result, malloc));

char * nih_vsprintf         (const void *parent, const char *format,
			     va_list args)
	__attribute__ ((format (printf, 2, 0), warn_unused_result, malloc));

char * nih_strdup           (const void *parent, const char *str)
	__attribute__ ((warn_unused_result, malloc));

char * nih_strndup          (const void *parent, const char *str, size_t len)
	__attribute__ ((warn_unused_result, malloc));

char * nih_strcat           (char **str, const void *parent, const char *src)
	__attribute__ ((warn_unused_result, malloc));
char * nih_strncat          (char **str, const void *parent, const char *src,
			     size_t len)
	__attribute__ ((warn_unused_result, malloc));

char * nih_strcat_sprintf   (char **str, const void *parent,
			     const char *format, ...)
	__attribute__ ((format (printf, 3, 4), warn_unused_result, malloc));
char * nih_strcat_vsprintf  (char **str, const void *parent,
			     const char *format, va_list args)
	__attribute__ ((format (printf, 3, 0), warn_unused_result, malloc));

char **nih_str_split        (const void *parent, const char *str,
			     const char *delim, int repeat)
	__attribute__ ((warn_unused_result, malloc));

char **nih_str_array_new    (const void *parent)
	__attribute__ ((warn_unused_result, malloc));
char **nih_str_array_add    (char ***array, const void *parent, size_t *len,
			     const char *str)
	__attribute__ ((warn_unused_result, malloc));
char **nih_str_array_addn   (char ***array, const void *parent, size_t *len,
			     const char *str, size_t strlen)
	__attribute__ ((warn_unused_result, malloc));
char **nih_str_array_addp   (char ***array, const void *parent, size_t *len,
			     void *ptr)
	__attribute__ ((warn_unused_result, malloc));
char **nih_str_array_copy   (const void *parent, size_t *len,
			     char * const *array)
	__attribute__ ((warn_unused_result, malloc));
char **nih_str_array_append (char ***array, const void *parent, size_t *len,
			     char * const *args)
	__attribute__ ((warn_unused_result, malloc));

char * nih_str_wrap         (const void *parent, const char *str, size_t len,
		             size_t first_indent, size_t indent)
	__attribute__ ((warn_unused_result, malloc));
size_t nih_str_screen_width (void);
char * nih_str_screen_wrap  (const void *parent, const char *str,
			     size_t first_indent, size_t indent)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_STRING_H */
