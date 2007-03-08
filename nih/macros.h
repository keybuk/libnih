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

#ifndef NIH_MACROS_H
#define NIH_MACROS_H

/* Get standard definitions from C library */
#include <stddef.h>
#include <stdint.h>


/* Allow headers to be imported from C++, marking as C. */
#ifdef __cplusplus
# define NIH_BEGIN_EXTERN  extern "C" {
# define NIH_END_EXTERN    }
#else
# define NIH_BEGIN_EXTERN
# define NIH_END_EXTERN
#endif /* __cplusplus */


/* Define NULL if we haven't got it */
#ifndef NULL
# define NULL ((void *) 0)
#endif /* NULL */

/* Define TRUE and FALSE if we haven't got them */
#ifndef FALSE
# define FALSE 0
#endif /* FALSE */
#ifndef TRUE
# define TRUE (!FALSE)
#endif /* TRUE */

/* Define MIN and MAX if we haven't got them */
#ifndef MIN
# define MIN(_a, _b) ((_a) > (_b) ? (_b) : (_a))
#endif /* MIN */
#ifndef MAX
# define MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#endif /* MAX */


/* Hack to turn numeric macros into a string */
#define _STRINGIFY_AGAIN(_s) #_s
#define NIH_STRINGIFY(_s)    _STRINGIFY_AGAIN(_s)

/* Branch prediction */
#define NIH_LIKELY(_e)   __builtin_expect ((_e) ? TRUE : FALSE, TRUE)
#define NIH_UNLIKELY(_e) __builtin_expect ((_e) ? TRUE : FALSE, FALSE)

/* Force a true or false value, _e must be an assignment expression */
#define NIH_MUST(_e) while (! (_e))
#define NIH_ZERO(_e) while ((_e))


/* Make gettext friendlier */
#if ENABLE_NLS
# include <libintl.h>
# include <locale.h>

# define _(_str)  gettext (_str)
# define _n(_str1, _str2, _num) ngettext (_str1, _str2, _num)
# define N_(_str) (_str)
#else /* ENABLE_NLS */
# define _(_str)  (_str)
# define _n(_str1, _str2, _num) ((_num) == 1 ? (_str1) : (_str2))
# define N_(_str) (_str)
#endif /* ENABLE_NLS */

#endif /* NIH_MACROS_H */
