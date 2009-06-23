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
