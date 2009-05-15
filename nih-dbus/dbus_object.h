/* libnih
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

#ifndef NIH_DBUS_OBJECT_H
#define NIH_DBUS_OBJECT_H

#include <nih/macros.h>

#include <nih-dbus/dbus_message.h>

#include <dbus/dbus.h>


/**
 * NihDBusObject:
 * @path: path of object,
 * @conn: associated connection,
 * @data: pointer to object data,
 * @interfaces: NULL-terminated array of interfaces the object supports,
 * @registered: TRUE while the object is registered.
 *
 * This structure represents an object visible on the given @conn at @path
 * and being handled by libnih-dbus.  It connects the @data pointer to the
 * individual method and property handler functions defined by the
 * @interfaces.
 *
 * Automatic introspection is provided based on @interfaces.
 *
 * No reference is held to @conn, therefore you may not assume that it is
 * valid.  In general, the object will be automatically freed should @conn
 * be cleaned up.
 **/
typedef struct nih_dbus_interface NihDBusInterface;
typedef struct nih_dbus_object {
	char *                   path;
	DBusConnection *         conn;
	void *                   data;
	const NihDBusInterface **interfaces;
	int                      registered;
} NihDBusObject;


/**
 * NihDBusMethodHandler:
 * @object: D-Bus object being handled,
 * @message: information about method call message to handle.
 *
 * A method handler function is called for a specific D-Bus method call
 * on @object, available as the @message parameter.  The handler should
 * examine the arguments and send the method reply or an error.
 *
 * While method handlers can be written by hand, it's far more efficient
 * to have them generated automatically.  nih-dbus-tool will generate
 * method handlers that call C functions of appropriate names with
 * ordinary arguments matching the expected types based on the message,
 * and that generate the reply message based on the return arguments.
 *
 * Returns: DBUS_HANDLER_RESULT_HANDLED when the message has been handled
 * and a reply or error sent, DBUS_HANDLER_RESULT_NOT_YET_HANDLED if the
 * handler has declined to handle the message or
 * DBUS_HANDLER_RESULT_NEED_MEMORY if insufficient memory to handle the
 * message.
 **/
typedef DBusHandlerResult (*NihDBusMethodHandler) (NihDBusObject *object,
						   NihDBusMessage *message);

/**
 * NihDBusPropertyGetter:
 * @object: D-Bus object being handled,
 * @message: information about property get message,
 * @iter: iterator to append value to.
 *
 * A property getter function is called when generating a reply to a
 * D-Bus properties Get or GetAll method, available as the @message
 * parameter.  The getter should append a variant onto @iter containing
 * the property value.
 *
 * Unlike method handlers, the Get and GetAll methods are implemented
 * internally to libnih-dbus, with a reply being generated and sent as
 * part of that handling.  It's only necessary to provide the actual
 * property value wrapped up in a variant.
 *
 * While property getters can be written by hand, it's far more efficient
 * to have them generated automatically.  nih-dbus-tool will generate
 * property getters that call C functions of appropriate names with an
 * ordinary pointer argument matching the expected type that the function
 * can place the property value into.
 *
 * Returns: zero on success, negative value if insufficient memory.
 **/
typedef int (*NihDBusPropertyGetter) (NihDBusObject *object,
				      NihDBusMessage *message,
				      DBusMessageIter *iter);

/**
 * NihDBusPropertySetter:
 * @object: D-Bus object being handled,
 * @message: information about property get message,
 * @iter: iterator to obtain value from.
 *
 * A property setter function is called when a handling the D-Bus
 * properties Set method, available as the @message parameter.  The
 * setter should obtain the new value from the variant pointed to by
 * @iter and return either an empty reply message or an error message.
 *
 * Unlike method handlers, the Set method is implemented internally to
 * libnih-dbus, with a reply being generated and sent as part of that
 * handling.  It's only necessary to take the property value from the
 * variant and set it.
 *
 * While property setters can be written by hand, it's far more efficient
 * to have them generated automatically.  nih-dbus-tool will generate
 * property getters that call C functions of appropriate names with an
 * ordinary argument matching the expected type that the function
 * can obtain the property value from.
 *
 * Returns: zero on success, negative value on raised error.
 **/
typedef int (*NihDBusPropertySetter) (NihDBusObject *object,
				      NihDBusMessage *message,
				      DBusMessageIter *iter);


/**
 * NihDBusArgDir:
 *
 * Whether an argument is for the method call (in) or method reply (out).
 **/
typedef enum nih_dbus_arg_dir {
	NIH_DBUS_ARG_IN,
	NIH_DBUS_ARG_OUT
} NihDBusArgDir;

/**
 * NihDBusArg:
 * @type: D-Bus type signature,
 * @name: name of argument,
 * @dir: whether argument is for method call or reply.
 *
 * This structure defines an argument to a D-Bus method or signal and is used
 * to provide introspection of that method.
 *
 * It's unusual to use this directly, instead methods are pre-defined as
 * members of const arrays by nih-dbus-tool and referenced by the interfaces
 * it also defines.
 **/
typedef struct nih_dbus_arg {
	const char *  name;
	const char *  type;
	NihDBusArgDir dir;
} NihDBusArg;


/**
 * NihDBusMethod:
 * @name: name of the method,
 * @args: NULL-terminated array of arguments,
 * @handler: handler function.
 *
 * This structure defines a method associated with a D-Bus interface.
 *
 * It's unusual to use this directly, instead methods are pre-defined as
 * members of const arrays by nih-dbus-tool and referenced by the interfaces
 * it also defines.
 *
 * When the method is invoked, the @handler function will be called and
 * is expected to reply with a method return or error message.  Method
 * handler functions are also normally generated by nih-dbus-tool.
 *
 * @args is used to provide introspection of the method.
 **/
typedef struct nih_dbus_method {
	const char *         name;
	const NihDBusArg *   args;
	NihDBusMethodHandler handler;
} NihDBusMethod;


/**
 * NihDBusSignal:
 * @name: name of the signal,
 * @args: NULL-terminated array of arguments.
 *
 * This structure defines a signal that can be emitted by a D-Bus interface
 * and is used to provide introspection of that signal.
 *
 * It's unusual to use this directly, instead signals are pre-defined as
 * const arrays by nih-dbus-tool and referenced by the interfaces it also
 * defines.
 *
 * The signal itself is normally emitted by a function generated by
 * nih-dbus-tool that accepts C arguments matching @args.
 **/
typedef struct nih_dbus_signal {
	const char *      name;
	const NihDBusArg *args;
} NihDBusSignal;


/**
 * NihDBusAccess:
 *
 * Access restrictions for a property.
 **/
typedef enum nih_dbus_access {
	NIH_DBUS_READ,
	NIH_DBUS_WRITE,
	NIH_DBUS_READWRITE
} NihDBusAccess;

/**
 * NihDBusProperty:
 * @name: name of the property,
 * @type: type signature of value,
 * @access: access restrictions,
 * @getter: getter function,
 * @setter: setter function.
 *
 * This structure defines a property associated with a D-Bus interface.
 *
 * It's unusual to use this directly, instead properties are pre-defined as
 * members of const arrays by nih-dbus-tool and referenced by the interfaces
 * it also defines.
 *
 * When the D-Bus properties Get or GetAll methods are invoked, the
 * @getter function will be called and is expected to add a variant to a
 * message being generated.  When the D-Bus properties Set method is invoked,
 * the @setter function will be called and is expected to return an empty
 * reply or an error.  Both getter and setter functions are also normally
 * generated by nih-dbus-tool.
 *
 * @access is used to provide introspection of the property.
 **/
typedef struct nih_dbus_property {
	const char *          name;
	const char *          type;
	NihDBusAccess         access;
	NihDBusPropertyGetter getter;
	NihDBusPropertySetter setter;
} NihDBusProperty;


/**
 * NihDBusInterface:
 * @name: name of the interface,
 * @methods: NULL-terminated array of methods,
 * @signals: NULL-terminated array of signals,
 * @properties: NULL-terminated array of properties.
 *
 * This structure defines an interface that may be implemented by a D-Bus
 * object.  It's unusual to use this in any form other than a const array
 * for each type of object, and even then the individual members of that
 * array are normally taken from macros defined by nih-dbus-tool that expand
 * to interfaces it defines.
 **/
struct nih_dbus_interface {
	const char *           name;
	const NihDBusMethod *  methods;
	const NihDBusSignal *  signals;
	const NihDBusProperty *properties;
};


NIH_BEGIN_EXTERN

NihDBusObject *nih_dbus_object_new (const void *parent, DBusConnection *conn,
				    const char *path,
				    const NihDBusInterface **interfaces,
				    void *data)
	__attribute__ ((malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_OBJECT_H */
