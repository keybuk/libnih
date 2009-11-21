/* nih-dbus-tool
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

#ifndef NIH_DBUS_TOOL_OUTPUT_H
#define NIH_DBUS_TOOL_OUTPUT_H

#include <nih/macros.h>

#include "node.h"


NIH_BEGIN_EXTERN

char *output_package;

int   output          (const char *source_path, int source_fd,
		       const char *header_path, int header_fd,
		       const char *prefix, Node *node, int object)
	__attribute__ ((warn_unused_result));

char *output_preamble (const void *parent, const char *path)
	__attribute__ ((warn_unused_result, malloc));
char *output_sentinel (const void *parent, const char *path)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_OUTPUT_H */
