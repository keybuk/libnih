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

#ifndef NIH_MACROS_H
#define NIH_MACROS_H

/**
 * This header tends to be included by every file that uses libnih, it makes
 * sure various sensible macros and types (including standard ones from the
 * C library) are defined.
 **/


/* Get standard definitions from C library */
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
/**
 * NIH_BEGIN_EXTERN:
 *
 * Use before beginning external definitions in header files, so that they
 * may be safely included from C++ code. Must be paired with NIH_END_EXTERN.
 **/
# define NIH_BEGIN_EXTERN  extern "C" {

/**
 * NIH_END_EXTERN:
 *
 * Use after external definitions in header files, so that they may be safely
 * included from C++ code.  Must be paired with NIH_BEGIN_EXTERN.
 **/
# define NIH_END_EXTERN    }
#else
# define NIH_BEGIN_EXTERN
# define NIH_END_EXTERN
#endif /* __cplusplus */


/**
 * FALSE:
 *
 * Defined to be zero.
 **/
#ifndef FALSE
# define FALSE 0
#endif /* FALSE */

/**
 * TRUE:
 *
 * Defined to be the opposite of zero, you don't need to know what that is ;)
 **/
#ifndef TRUE
# define TRUE (!FALSE)
#endif /* TRUE */


/**
 * nih_min:
 * @_a: first value,
 * @_b: second value.
 *
 * Compares the two values @_a and @_b, which must be compatible C types.
 *
 * Returns: the smaller of the two values.
 **/
#define nih_min(_a, _b)					   \
	({ typeof (_a) __a = (_a); typeof (_b) __b = (_b); \
		__a < __b ? __a : __b; })

/**
 * nih_max:
 * @_a: first value,
 * @_b: second value.
 *
 * Compares the two values @_a and @_b, which must be compatible C types.
 *
 * Returns: the larger of the two values.
 **/
#define nih_max(_a, _b)					   \
	({ typeof (_a) __a = (_a); typeof (_b) __b = (_b); \
		__a > __b ? __a : __b; })

/**
 * NIH_ALIGN_SIZE:
 *
 * In general, pointer alignment is something that the compiler takes care
 * of for us; but in some situations (e.g. nih_alloc) we need to return a
 * pointer that is generically aligned for any data type without actually
 * knowing the data type.
 *
 * This is a good guess as to the largest alignment of the platform, based
 * on recommendations in the C standard and comments in GNU libc.
 **/
#define NIH_ALIGN_SIZE nih_max(2 * sizeof (size_t),	\
			       __alignof__ (long long))


/**
 * NIH_STRINGIFY:
 * @_s: macro.
 *
 * Turns the macro @_s into a string.
 **/
#define _STRINGIFY_AGAIN(_s) #_s
#define NIH_STRINGIFY(_s)    _STRINGIFY_AGAIN(_s)


/**
 * NIH_LIKELY:
 * @_e: C expression.
 *
 * Indicates to the compiler that the expression @_e is likely to be true,
 * can aid optimisation when used properly.
 **/
#define NIH_LIKELY(_e)   __builtin_expect ((_e) ? TRUE : FALSE, TRUE)

/**
 * NIH_UNLIKELY:
 * @_e: C expression.
 *
 * Indicates to the compiler that the expression @_e is likely to be false,
 * can aid optimisation when used properly.
 **/
#define NIH_UNLIKELY(_e) __builtin_expect ((_e) ? TRUE : FALSE, FALSE)


/**
 * NIH_MUST:
 * @_e: C expression.
 *
 * Repeats the expression @_e until it yields a true value, normally used
 * around functions that perform memory allocation and return a pointer to
 * spin in out-of-memory situations.
 *
 * For situations where the the expression can raise an NihError and returns
 * false, where an error can include out-of-memory, you may want to use
 * NIH_SHOULD() to spin on OOM but break on other conditions.
 *
 * Returns: value of expression @_e which will be evaluated as many times
 * as necessary to become true.
 **/
#define NIH_MUST(_e)				\
	({ typeof (_e) __ret; while (! (__ret = (_e))); __ret; })

/**
 * NIH_ZERO:
 * @_e: C expression.
 *
 * Repeats the expression @_e until it yields a zero value, normally used
 * around functions that return zero to indicate success and non-zero to
 * indicate a temporary failure.
 *
 * Returns: value of expression @_e which will be evaluated as many times
 * as necessary to become zero.
 **/
#define NIH_ZERO(_e)						\
	({ typeof (_e) __ret; while ((__ret = (_e))); __ret; })


/* Make gettext friendlier */
#if ENABLE_NLS
# include <libintl.h>
# include <locale.h>

/**
 * _:
 * @_str:
 *
 * Marks the string @str for translation, if gettext is available.
 *
 * Returns: @_str or translated string.
 **/
# define _(_str) gettext (_str)

/**
 * _n:
 * @_str1: singular form,
 * @_str2: plural form,
 * @_num: number.
 *
 * Selects the appropriate plural form from @_str1 and @_str2 based on @_num,
 * translating if gettext is available.
 *
 * Returns: @_str1, @_str2 or translated string.
 **/
# define _n(_str1, _str2, _num) ngettext (_str1, _str2, _num)

/**
 * N_:
 * @_str:
 *
 * Marks the static string @_str for translation by gettext, but does not
 * return the translation.  You must call gettext() on the string before
 * presenting it to the user.
 *
 * Returns: @_str
 **/
# define N_(_str) (_str)

#else /* ENABLE_NLS */
# define _(_str)  (_str)
# define _n(_str1, _str2, _num) ((_num) == 1 ? (_str1) : (_str2))
# define N_(_str) (_str)
#endif /* ENABLE_NLS */

#endif /* NIH_MACROS_H */
