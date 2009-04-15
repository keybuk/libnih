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

#ifndef NIH_DBUS_TOOL_DEMARSHAL_H
#define NIH_DBUS_TOOL_DEMARSHAL_H

#include <nih/macros.h>
#include <nih/list.h>

#include <dbus/dbus.h>


NIH_BEGIN_EXTERN

char *demarshal (const void *parent, DBusSignatureIter *iter,
		 const char *parent_name, const char *iter_name,
		 const char *name,
		 const char *oom_error_code,
		 const char *type_error_code,
		 NihList *outputs, NihList *locals)
	__attribute__ ((malloc, warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_DEMARSHAL_H */
