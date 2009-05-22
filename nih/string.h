/* libnih
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
