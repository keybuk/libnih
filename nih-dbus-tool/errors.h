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

#ifndef NIH_DBUS_TOOL_ERRORS_H
#define NIH_DBUS_TOOL_ERRORS_H

#include <nih/macros.h>
#include <nih/errors.h>


/* Allocated error numbers */
enum {
	NIH_DBUS_TOOL_ERROR_START = NIH_ERROR_APPLICATION_START,

	NODE_INVALID_PATH,

	INTERFACE_MISSING_NAME,
	INTERFACE_INVALID_NAME,
	INTERFACE_ILLEGAL_DEPRECATED,
	INTERFACE_UNKNOWN_ANNOTATION,
	INTERFACE_INVALID_SYMBOL,
	INTERFACE_DUPLICATE_SYMBOL,

	METHOD_MISSING_NAME,
	METHOD_INVALID_NAME,
	METHOD_ILLEGAL_DEPRECATED,
	METHOD_INVALID_SYMBOL,
	METHOD_ILLEGAL_NO_REPLY,
	METHOD_ILLEGAL_ASYNC,
	METHOD_UNKNOWN_ANNOTATION,
	METHOD_DUPLICATE_SYMBOL,

	SIGNAL_MISSING_NAME,
	SIGNAL_INVALID_NAME,
	SIGNAL_ILLEGAL_DEPRECATED,
	SIGNAL_INVALID_SYMBOL,
	SIGNAL_UNKNOWN_ANNOTATION,
	SIGNAL_DUPLICATE_SYMBOL,

	PROPERTY_MISSING_NAME,
	PROPERTY_INVALID_NAME,
	PROPERTY_MISSING_TYPE,
	PROPERTY_INVALID_TYPE,
	PROPERTY_MISSING_ACCESS,
	PROPERTY_ILLEGAL_ACCESS,
	PROPERTY_ILLEGAL_DEPRECATED,
	PROPERTY_INVALID_SYMBOL,
	PROPERTY_UNKNOWN_ANNOTATION,
	PROPERTY_DUPLICATE_SYMBOL,

	ARGUMENT_INVALID_NAME,
	ARGUMENT_MISSING_TYPE,
	ARGUMENT_INVALID_TYPE,
	ARGUMENT_ILLEGAL_METHOD_DIRECTION,
	ARGUMENT_ILLEGAL_SIGNAL_DIRECTION,
	ARGUMENT_INVALID_SYMBOL,
	ARGUMENT_UNKNOWN_ANNOTATION,
	ARGUMENT_DUPLICATE_SYMBOL,

	ANNOTATION_MISSING_NAME,
	ANNOTATION_MISSING_VALUE,
};

/* Error strings for defined messages */
#define SYMBOLS_UNIQUE_OVERFLOW_STR           N_("Unable to generate unique name for symbol")

#define NODE_INVALID_PATH_STR                 N_("Invalid object path in <node> name attribute")

#define INTERFACE_MISSING_NAME_STR            N_("<interface> missing required name attribute")
#define INTERFACE_INVALID_NAME_STR            N_("Invalid interface name in <interface> name attribute")
#define INTERFACE_ILLEGAL_DEPRECATED_STR      N_("Illegal value for org.freedesktop.DBus.Deprecated interface annotation, expected 'true' or 'false'")
#define INTERFACE_INVALID_SYMBOL_STR          N_("Invalid C symbol for interface")
#define INTERFACE_UNKNOWN_ANNOTATION_STR      N_("Unknown annotation for interface")
#define INTERFACE_DUPLICATE_SYMBOL_STR        N_("Symbol '%s' already assigned to %s interface")

#define METHOD_MISSING_NAME_STR               N_("<method> missing required name attribute")
#define METHOD_INVALID_NAME_STR               N_("Invalid method name in <method> name attribute")
#define METHOD_ILLEGAL_DEPRECATED_STR         N_("Illegal value for org.freedesktop.DBus.Deprecated method annotation, expected 'true' or 'false'")
#define METHOD_ILLEGAL_NO_REPLY_STR           N_("Illegal value for org.freedesktop.DBus.Method.NoReply method annotation, expected 'true' or 'false'")
#define METHOD_INVALID_SYMBOL_STR             N_("Invalid C symbol for method")
#define METHOD_ILLEGAL_ASYNC_STR              N_("Illegal value for com.netsplit.Nih.Method.Async method annotation, expected 'true' or 'false'")
#define METHOD_UNKNOWN_ANNOTATION_STR         N_("Unknown annotation for method")
#define METHOD_DUPLICATE_SYMBOL_STR           N_("Symbol '%s' already assigned to %s method")

#define SIGNAL_MISSING_NAME_STR               N_("<signal> missing required name attribute")
#define SIGNAL_INVALID_NAME_STR               N_("Invalid signal name in <signal> name attribute")
#define SIGNAL_ILLEGAL_DEPRECATED_STR         N_("Illegal value for org.freedesktop.DBus.Deprecated signal annotation, expected 'true' or 'false'")
#define SIGNAL_INVALID_SYMBOL_STR             N_("Invalid C symbol for signal")
#define SIGNAL_UNKNOWN_ANNOTATION_STR         N_("Unknown annotation for signal")
#define SIGNAL_DUPLICATE_SYMBOL_STR           N_("Symbol '%s' already assigned to %s signal")

#define PROPERTY_MISSING_NAME_STR             N_("<property> missing required name attribute")
#define PROPERTY_INVALID_NAME_STR             N_("Invalid property name in <property> name attribute")
#define PROPERTY_MISSING_TYPE_STR             N_("<property> missing required type attribute")
#define PROPERTY_INVALID_TYPE_STR             N_("Invalid D-Bus type in <property> type attribute")
#define PROPERTY_MISSING_ACCESS_STR           N_("<property> missing required access attribute")
#define PROPERTY_ILLEGAL_ACCESS_STR           N_("Illegal value for <property> access attribute, expected 'read', 'write' or 'readwrite'")
#define PROPERTY_ILLEGAL_DEPRECATED_STR       N_("Illegal value for org.freedesktop.DBus.Deprecated property annotation, expected 'true' or 'false'")
#define PROPERTY_INVALID_SYMBOL_STR           N_("Invalid C symbol for property")
#define PROPERTY_UNKNOWN_ANNOTATION_STR       N_("Unknown annotation for property")
#define PROPERTY_DUPLICATE_SYMBOL_STR         N_("Symbol '%s' already assigned to %s property")

#define ARGUMENT_INVALID_SYMBOL_STR           N_("Invalid C symbol for argument")
#define ARGUMENT_UNKNOWN_ANNOTATION_STR       N_("Unknown annotation for argument")
#define ARGUMENT_INVALID_NAME_STR             N_("Invalid argument name in <arg> name attribute")
#define ARGUMENT_MISSING_TYPE_STR             N_("<arg> missing required type attribute")
#define ARGUMENT_INVALID_TYPE_STR             N_("Invalid D-Bus type in <arg> type attribute")
#define ARGUMENT_ILLEGAL_METHOD_DIRECTION_STR N_("Illegal value for <arg> direction attribute, expected 'in' or 'out'")
#define ARGUMENT_ILLEGAL_SIGNAL_DIRECTION_STR N_("Illegal value for <arg> direction attribute, expected 'out'")
#define ARGUMENT_DUPLICATE_SYMBOL_STR         N_("Symbol '%s' already assigned to %s argument")

#define ANNOTATION_MISSING_NAME_STR           N_("<annotation> missing required name attribute")
#define ANNOTATION_MISSING_VALUE_STR          N_("<annotation> missing required value attribute")

#endif /* NIH_DBUS_TOOL_ERRORS_H */
