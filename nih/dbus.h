/* libnih
 *
 * Copyright © 2008 Scott James Remnant <scott@netsplit.com>.
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

#ifndef NIH_DBUS_H
#define NIH_DBUS_H

#include <nih/macros.h>
#include <nih/error.h>

#include <dbus/dbus.h>


/**
 * NihDBusError:
 * @error: ordinary NihError,
 * @name: D-Bus name.
 *
 * This structure builds on NihError to include an additional @name field
 * required for transport across D-Bus.
 *
 * If you receive a NIH_DBUS_ERROR, the returned NihError structure is
 * actually this structure and can be cast to get the additional fields.
 **/
typedef struct nih_dbus_error {
	NihError  error;
	char     *name;
} NihDBusError;


/**
 * NihDBusDisconnectHandler:
 * @conn: Connection that was lost.
 *
 * A D-Bus disconnect handler is a function called when the D-Bus connection
 * @conn is disconnected from its server.  Once called, the connection is
 * automatically unreferenced.
 **/
typedef void (*NihDBusDisconnectHandler) (DBusConnection *conn);

/**
 * NihDBusConnectHandler:
 * @server: Server that received new connection,
 * @conn: New connection.
 *
 * A D-Bus connection handler is a function called when the D-Bus @server
 * receives a new connection @conn.  The function must return TRUE for the
 * connection to be accepted, otherwise it will be dropped.
 **/
typedef int (*NihDBusConnectHandler) (DBusServer *server,
				      DBusConnection *conn);


/**
 * NihDBusObject:
 * @path: path of object,
 * @conn: associated connection,
 * @data: pointer to object data,
 * @interfaces: NULL-terminated array of interfaces the object supports,
 * @registered: TRUE while the object is registered.
 *
 * An instance of this structure must be created for each object you want
 * to be visible on the bus and handled automatically by libnih-dbus.  It
 * connects the @data pointer to the individual method and property calls
 * defined by the @interfaces, providing automatic marshalling and
 * introspection.
 **/
typedef struct nih_dbus_interface NihDBusInterface;
typedef struct nih_dbus_object {
	char                    *path;
	DBusConnection          *conn;
	void                    *data;
	const NihDBusInterface **interfaces;
	int                      registered;
} NihDBusObject;

/**
 * NihDBusMessage:
 * @conn: D-Bus connection message was received on,
 * @message: message object received.
 *
 * This structure is used as a context for the processing of a message; the
 * primary reason for its existance is to be used as an nih_alloc() context
 * for any reply data.
 *
 * Instances are allocated automatically and passed to marshaller functions,
 * and freed on their return.
 **/
typedef struct nih_dbus_message {
	DBusConnection *conn;
	DBusMessage    *message;
} NihDBusMessage;


/**
 * NihDBusProxy:
 * @name: D-Bus name of object owner,
 * @path: path of object,
 * @conn: associated connection.
 *
 * Instances of this structure may be created for remote objects that you
 * wish to use.  Fundamentally they combine the three elements of data
 * necessary into one easy object that is bound to the lifetime of the
 * associated connection.
 **/
typedef struct nih_dbus_proxy {
	char           *name;
	char           *path;
	DBusConnection *conn;
} NihDBusProxy;


/**
 * NihDBusMarshaller:
 * @object: D-Bus object being handled,
 * @message: information about message to marshal.
 *
 * A marshaller function is called to turn a D-Bus @message into appropriate
 * function calls acting on @object, and to handle sending the method reply
 * where required.
 *
 * While marshallers can be written by hand, they are almost always
 * automatically generated.  nih-dbus-tool will generate marshallers that
 * call C functions of appropriate names with ordinary arguments that match
 * the expected type of the message.
 **/
typedef DBusHandlerResult (*NihDBusMarshaller) (NihDBusObject  *object,
						NihDBusMessage *message);


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
	const char       *name;
	const char       *type;
	NihDBusArgDir     dir;
} NihDBusArg;


/**
 * NihDBusMethod:
 * @name: name of the method,
 * @marshaller: marshaller function,
 * @args: NULL-terminated array of arguments.
 *
 * This structure defines a method associated with a D-Bus interface.
 *
 * It's unusual to use this directly, instead methods are pre-defined as
 * members of const arrays by nih-dbus-tool and referenced by the interfaces
 * it also defines.
 *
 * When the method is invoked, the @marshaller function will be called and
 * is expected to marshal the message into a C function call.  Marshaller
 * functions are also normally generated by nih-dbus-tool.
 *
 * @args is used to provide introspection of the method.
 **/
typedef struct nih_dbus_method {
	const char        *name;
	NihDBusMarshaller  marshaller;
	const NihDBusArg  *args;
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
	const char       *name;
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
 * @access: access restrictions.
 *
 * This structure defines a property associated with a D-Bus interface.
 *
 * It's unusual to use this directly, instead properties are pre-defined as
 * members of const arrays by nih-dbus-tool and referenced by the interfaces
 * it also defines.
 *
 * @access is used to provide introspection of the property.
 **/
typedef struct nih_dbus_property {
	const char        *name;
	const char        *type;
	NihDBusAccess      access;
} NihDBusProperty;


/**
 * NihAsyncNotifyData:
 * @handler: The user handler that our libnih handler should call,
 * @userdata: Data to pass to @handler,
 * @proxy: The proxy object to which the call was made.
 *
 * This structure contains information that is assembled during an asynchronous
 * method call and passed to the handler on the method's return. It should never
 * be used directly by the user.
 **/
typedef struct nih_async_notify_data {
    void *handler;
    void *userdata;
    NihDBusProxy *proxy;
} NihAsyncNotifyData;

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
	const char            *name;
	const NihDBusMethod   *methods;
	const NihDBusSignal   *signals;
	const NihDBusProperty *properties;
};


NIH_BEGIN_EXTERN

void            nih_dbus_error_raise        (const char *name,
					     const char *message);

void            nih_dbus_error_raise_printf (const char *name,
					     const char *format, ...)
	__attribute__ ((format (printf, 2, 3)));

DBusConnection *nih_dbus_connect            (const char *address,
				             NihDBusDisconnectHandler disconnect_handler)
	__attribute__ ((warn_unused_result));
DBusConnection *nih_dbus_bus                (DBusBusType bus,
					     NihDBusDisconnectHandler disconnect_handler)
	__attribute__ ((warn_unused_result));
int             nih_dbus_setup              (DBusConnection *conn,
				             NihDBusDisconnectHandler disconnect_handler)
	__attribute__ ((warn_unused_result));

DBusServer *    nih_dbus_server             (const char *address,
					     NihDBusConnectHandler connect_handler,
					     NihDBusDisconnectHandler disconnect_handler)
	__attribute__ ((warn_unused_result));

NihDBusObject * nih_dbus_object_new         (const void *parent,
					     DBusConnection *conn,
					     const char *path,
					     const NihDBusInterface **interfaces,
					     void *data)
	__attribute__ ((malloc));

int             nih_dbus_message_error      (NihDBusMessage *msg,
					     const char *name,
					     const char *format, ...)
	__attribute__ ((warn_unused_result));

NihDBusProxy *  nih_dbus_proxy_new          (const void *parent,
					     DBusConnection *conn,
					     const char *name,
					     const char *path)
	__attribute__ ((malloc));

char *          nih_dbus_path               (const void *parent,
					     const char *root, ...)
	__attribute__ ((sentinel, warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_H */
