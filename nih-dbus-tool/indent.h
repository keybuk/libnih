/* nih-dbus-tool
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
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

#ifndef NIH_DBUS_TOOL_INDENT_H
#define NIH_DBUS_TOOL_INDENT_H

#include <nih/macros.h>


NIH_BEGIN_EXTERN

char *indent  (char **str, const void *parent, int level)
	__attribute__ ((warn_unused_result, malloc));
char *comment (char **str, const void *parent)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_INDENT_H */
