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

#ifndef NIH_ERRORS_H
#define NIH_ERRORS_H

#include <nih/error.h>

#include <errno.h>


/* Allocated error numbers */
enum {
	/* 0x0000 thru 0xFFFF reserved for errno */
	NIH_ERROR_ERRNO_START = 0x0000L,

	/* 0x10000 thru 0x1FFFF reserved for libnih */
	NIH_ERROR_LIBNIH_START = 0x10000L,

	NIH_CONFIG_EXPECTED_TOKEN,
	NIH_CONFIG_UNEXPECTED_TOKEN,
	NIH_CONFIG_TRAILING_SLASH,
	NIH_CONFIG_UNTERMINATED_QUOTE,
	NIH_CONFIG_UNTERMINATED_BLOCK,
	NIH_CONFIG_UNKNOWN_STANZA,

	NIH_DIR_LOOP_DETECTED,

	/* 0x20000 thru 0x2FFFF reserved for applications */
	NIH_ERROR_APPLICATION_START = 0x20000L,

	/* 0x30000 thru 0x3FFFF reserved for libnih-dbus */
	NIH_ERROR_LIBNIH_DBUS_START = 0x30000L,

	/* 0x80000 upwards for other libraries */
	NIH_ERROR_LIBRARY_START = 0x80000L
};

/* Error strings for defined messages */
#define NIH_CONFIG_EXPECTED_TOKEN_STR      N_("Expected token")
#define NIH_CONFIG_UNEXPECTED_TOKEN_STR    N_("Unexpected token")
#define NIH_CONFIG_TRAILING_SLASH_STR      N_("Trailing slash in file")
#define NIH_CONFIG_UNTERMINATED_QUOTE_STR  N_("Unterminated quoted string")
#define NIH_CONFIG_UNTERMINATED_BLOCK_STR  N_("Unterminated block")
#define NIH_CONFIG_UNKNOWN_STANZA_STR      N_("Unknown stanza")

#define NIH_DIR_LOOP_DETECTED_STR          N_("Directory loop detected")

#endif /* NIH_ERRORS_H */
