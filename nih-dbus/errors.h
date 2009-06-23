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

#ifndef NIH_DBUS_ERRORS_H
#define NIH_DBUS_ERRORS_H

#include <nih/error.h>
#include <nih/errors.h>

#include <errno.h>


/* Allocated error numbers */
enum {
	NIH_DBUS_ERROR_START = NIH_ERROR_LIBNIH_DBUS_START,

	NIH_DBUS_ERROR,
	NIH_DBUS_INVALID_ARGS,
};

/* Error strings for defined messages */
#define NIH_DBUS_INVALID_ARGS_STR	   N_("Invalid arguments received in reply")

#endif /* NIH_DBUS_ERRORS_H */
