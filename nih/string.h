/* libnih
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

#ifndef NIH_STRING_H
#define NIH_STRING_H

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
void   nih_strv_free        (char **strv);

char * nih_str_wrap         (const void *parent, const char *str, size_t len,
		             size_t first_indent, size_t indent)
	__attribute__ ((warn_unused_result, malloc));
size_t nih_str_screen_width (void);
char * nih_str_screen_wrap  (const void *parent, const char *str,
			     size_t first_indent, size_t indent)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_STRING_H */
