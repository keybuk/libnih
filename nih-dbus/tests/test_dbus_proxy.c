/* libnih
 *
 * test_dbus_proxy.c - test suite for nih-dbus/dbus_proxy.c
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

#include <nih/test.h>
#include <nih-dbus/test_dbus.h>

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/error.h>

#include <nih-dbus/dbus_proxy.h>


static int my_lost_handler_called = FALSE;

static void
my_lost_handler (void *        data,
		 NihDBusProxy *proxy)
{
	my_lost_handler_called++;

	TEST_NE_P (proxy, NULL);
	TEST_EQ_P (data, proxy->connection);
}

static void
my_freeing_lost_handler (void *        data,
			 NihDBusProxy *proxy)
{
	my_lost_handler_called++;

	TEST_NE_P (proxy, NULL);
	TEST_EQ_P (data, proxy->connection);

	nih_free (proxy);
}

void
test_new (void)
{
	pid_t           dbus_pid;
	DBusConnection *conn;
	DBusConnection *other_conn;
	NihDBusProxy *  proxy;
	NihError *      err;

	TEST_FUNCTION ("nih_dbus_proxy_new");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (conn);


	/* Check that we can create a simple proxy for a remote object on
	 * a peer-to-peer connection, and have a proxy object returned with
	 * the right detailed filled in.
	 */
	TEST_FEATURE ("with peer-to-peer object");
	TEST_ALLOC_FAIL {
		proxy = nih_dbus_proxy_new (NULL, conn, NULL,
					    "/com/netsplit/Nih",
					    NULL, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (proxy, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			continue;
		}

		TEST_ALLOC_SIZE (proxy, sizeof (NihDBusProxy));

		TEST_EQ_P (proxy->connection, conn);

		TEST_EQ_P (proxy->name, NULL);

		TEST_EQ_P (proxy->owner, NULL);

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_TRUE (proxy->auto_start);

		TEST_EQ_P (proxy->lost_handler, NULL);
		TEST_EQ_P (proxy->data, NULL);

		nih_free (proxy);
	}


	/* Check that we can pass a well-known name which looks up
	 * whether the name is on the bus, and sets up a match for it.
	 * If the name does not exist on the bus, NULL should be set for
	 * the owner.
	 */
	TEST_FEATURE ("with unconnected well-known name");
	TEST_ALLOC_FAIL {
		proxy = nih_dbus_proxy_new (NULL, conn, "com.netsplit.Nih",
					    "/com/netsplit/Nih",
					    NULL, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (proxy, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			continue;
		}

		TEST_ALLOC_SIZE (proxy, sizeof (NihDBusProxy));

		TEST_EQ_P (proxy->connection, conn);

		TEST_ALLOC_PARENT (proxy->name, proxy);
		TEST_EQ_STR (proxy->name, "com.netsplit.Nih");

		TEST_EQ_P (proxy->owner, NULL);

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_TRUE (proxy->auto_start);

		TEST_EQ_P (proxy->lost_handler, NULL);
		TEST_EQ_P (proxy->data, NULL);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}
	}


	/* Check that we can pass a well-known name when the name
	 * does exist on the bus, and that the unique name of the owner
	 * is stored in the owner member.
	 */
	TEST_FEATURE ("with connected well-known name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (other_conn);

		assert (dbus_bus_request_name (other_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		proxy = nih_dbus_proxy_new (NULL, conn, "com.netsplit.Nih",
					    "/com/netsplit/Nih",
					    NULL, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (proxy, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_DBUS_CLOSE (other_conn);
			continue;
		}

		TEST_ALLOC_SIZE (proxy, sizeof (NihDBusProxy));

		TEST_EQ_P (proxy->connection, conn);

		TEST_ALLOC_PARENT (proxy->name, proxy);
		TEST_EQ_STR (proxy->name, "com.netsplit.Nih");

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (other_conn));

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_TRUE (proxy->auto_start);

		TEST_EQ_P (proxy->lost_handler, NULL);
		TEST_EQ_P (proxy->data, NULL);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (other_conn);
	}


	/* Check that we can pass a unique name, and that it is copied
	 * into the owner member.
	 */
	TEST_FEATURE ("with unique name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (other_conn);

		proxy = nih_dbus_proxy_new (NULL, conn,
					    dbus_bus_get_unique_name (other_conn),
					    "/com/netsplit/Nih",
					    NULL, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (proxy, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			TEST_DBUS_CLOSE (other_conn);
			continue;
		}

		TEST_ALLOC_SIZE (proxy, sizeof (NihDBusProxy));

		TEST_EQ_P (proxy->connection, conn);

		TEST_ALLOC_PARENT (proxy->name, proxy);
		TEST_EQ_STR (proxy->name, dbus_bus_get_unique_name (other_conn));

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (other_conn));

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_TRUE (proxy->auto_start);

		TEST_EQ_P (proxy->lost_handler, NULL);
		TEST_EQ_P (proxy->data, NULL);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (other_conn);
	}


	/* Check that we can pass a lost handler function and data pointer,
	 * which get stored in the structure for later use.
	 */
	TEST_FEATURE ("with lost_handler");
	TEST_ALLOC_FAIL {
		proxy = nih_dbus_proxy_new (NULL, conn, "com.netsplit.Nih",
					    "/com/netsplit/Nih",
					    my_lost_handler, conn);

		if (test_alloc_failed) {
			TEST_EQ_P (proxy, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			continue;
		}

		TEST_ALLOC_SIZE (proxy, sizeof (NihDBusProxy));

		TEST_EQ_P (proxy->connection, conn);

		TEST_ALLOC_PARENT (proxy->name, proxy);
		TEST_EQ_STR (proxy->name, "com.netsplit.Nih");

		TEST_EQ_P (proxy->owner, NULL);

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_TRUE (proxy->auto_start);

		TEST_EQ_P (proxy->lost_handler, my_lost_handler);
		TEST_EQ_P (proxy->data, conn);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}
	}


	TEST_DBUS_CLOSE (conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_name_owner_changed (void)
{
	pid_t           dbus_pid;
	DBusConnection *conn;
	DBusConnection *first_conn;
	DBusConnection *second_conn;
	NihDBusProxy *  proxy = NULL;
	char *          last_owner;

	TEST_FUNCTION ("nih_dbus_proxy_name_owner_changed");
	TEST_DBUS (dbus_pid);


	/* Check that when we start off with an unconnected name, and it
	 * joins the bus, the owner field is automatically updated based
	 * on the information in the NameOwnerChanged signal that it's
	 * asked to receive.
	 */
	TEST_FEATURE ("with initially unconnected name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (conn);

		my_lost_handler_called = FALSE;

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, conn,
						    "com.netsplit.Nih",
						    "/com/netsplit/Nih",
						    my_lost_handler, conn);
		}

		TEST_EQ_P (proxy->owner, NULL);


		TEST_DBUS_OPEN (first_conn);

		assert (dbus_bus_request_name (first_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		TEST_DBUS_DISPATCH (conn);

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (first_conn));

		TEST_FALSE (my_lost_handler_called);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (first_conn);
		TEST_DBUS_CLOSE (conn);
	}


	/* Check that when we start off with an unconnected name, and it
	 * changes its name after having joined the bus, the owner field
	 * is updated again.
	 */
	TEST_FEATURE ("with change of initially unconnected name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (conn);

		my_lost_handler_called = FALSE;

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, conn,
						    "com.netsplit.Nih",
						    "/com/netsplit/Nih",
						    my_lost_handler, conn);
		}

		TEST_EQ_P (proxy->owner, NULL);


		TEST_DBUS_OPEN (first_conn);

		assert (dbus_bus_request_name (first_conn, "com.netsplit.Nih",
					       DBUS_NAME_FLAG_ALLOW_REPLACEMENT, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		TEST_DBUS_DISPATCH (conn);

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (first_conn));

		last_owner = proxy->owner;
		TEST_FREE_TAG (last_owner);

		TEST_FALSE (my_lost_handler_called);


		TEST_DBUS_OPEN (second_conn);

		assert (dbus_bus_request_name (second_conn, "com.netsplit.Nih",
					       DBUS_NAME_FLAG_REPLACE_EXISTING, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		TEST_DBUS_DISPATCH (conn);

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (second_conn));

		TEST_FREE (last_owner);

		TEST_FALSE (my_lost_handler_called);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (first_conn);
		TEST_DBUS_CLOSE (second_conn);
		TEST_DBUS_CLOSE (conn);
	}


	/* Check that when we start off with an connected name, and it
	 * changes its name, the owner field is updated.
	 */
	TEST_FEATURE ("with change of initially connected name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (conn);

		my_lost_handler_called = FALSE;

		TEST_DBUS_OPEN (first_conn);

		assert (dbus_bus_request_name (first_conn, "com.netsplit.Nih",
					       DBUS_NAME_FLAG_ALLOW_REPLACEMENT, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);


		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, conn,
						    "com.netsplit.Nih",
						    "/com/netsplit/Nih",
						    my_lost_handler, conn);
		}

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (first_conn));

		last_owner = proxy->owner;
		TEST_FREE_TAG (last_owner);


		TEST_DBUS_OPEN (second_conn);

		assert (dbus_bus_request_name (second_conn, "com.netsplit.Nih",
					       DBUS_NAME_FLAG_REPLACE_EXISTING, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		TEST_DBUS_DISPATCH (conn);

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (second_conn));

		TEST_FREE (last_owner);

		TEST_FALSE (my_lost_handler_called);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (first_conn);
		TEST_DBUS_CLOSE (second_conn);
		TEST_DBUS_CLOSE (conn);
	}


	/* Check that when an initially connected name leaves the bus,
	 * the lost handler is called and the owner field reset to NULL.
	 */
	TEST_FEATURE ("with loss of initially unconnected name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (conn);

		my_lost_handler_called = FALSE;

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, conn,
						    "com.netsplit.Nih",
						    "/com/netsplit/Nih",
						    my_lost_handler, conn);
		}

		TEST_EQ_P (proxy->owner, NULL);


		TEST_DBUS_OPEN (first_conn);

		assert (dbus_bus_request_name (first_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		TEST_DBUS_DISPATCH (conn);

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (first_conn));

		last_owner = proxy->owner;
		TEST_FREE_TAG (last_owner);

		TEST_FALSE (my_lost_handler_called);

		TEST_DBUS_CLOSE (first_conn);

		TEST_DBUS_DISPATCH (conn);

		TEST_EQ_P (proxy->owner, NULL);
		TEST_FREE (last_owner);

		TEST_TRUE (my_lost_handler_called);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (conn);
	}


	/* Check that when an initially connected name leaves the bus,
	 * the lost handler is called and the owner field reset to NULL.
	 */
	TEST_FEATURE ("with loss of initially connected name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (conn);

		my_lost_handler_called = FALSE;

		TEST_DBUS_OPEN (first_conn);

		assert (dbus_bus_request_name (first_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);


		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, conn,
						    "com.netsplit.Nih",
						    "/com/netsplit/Nih",
						    my_lost_handler, conn);
		}

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (first_conn));

		last_owner = proxy->owner;
		TEST_FREE_TAG (last_owner);

		TEST_DBUS_CLOSE (first_conn);

		TEST_DBUS_DISPATCH (conn);

		TEST_EQ_P (proxy->owner, NULL);
		TEST_FREE (last_owner);

		TEST_TRUE (my_lost_handler_called);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (conn);
	}


	/* Check that when a unique name leaves the bus, the lost handler
	 * is still called and the owner field rest to NULL.
	 */
	TEST_FEATURE ("with loss of unique name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (conn);

		my_lost_handler_called = FALSE;

		TEST_DBUS_OPEN (first_conn);

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, conn,
						    dbus_bus_get_unique_name (first_conn),
						    "/com/netsplit/Nih",
						    my_lost_handler, conn);
		}

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (first_conn));

		last_owner = proxy->owner;
		TEST_FREE_TAG (last_owner);

		TEST_DBUS_CLOSE (first_conn);

		TEST_DBUS_DISPATCH (conn);

		TEST_EQ_P (proxy->owner, NULL);
		TEST_FREE (last_owner);

		TEST_TRUE (my_lost_handler_called);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (conn);
	}


	/* Check that if the lost handler doesn't free the structure and
	 * it comes back on the bus after having left, the owner field
	 * is updated with the new name.
	 */
	TEST_FEATURE ("with return of lost name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (conn);

		my_lost_handler_called = FALSE;

		TEST_DBUS_OPEN (first_conn);

		assert (dbus_bus_request_name (first_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);


		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, conn,
						    "com.netsplit.Nih",
						    "/com/netsplit/Nih",
						    my_lost_handler, conn);
		}

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (first_conn));

		last_owner = proxy->owner;
		TEST_FREE_TAG (last_owner);

		TEST_DBUS_CLOSE (first_conn);


		TEST_DBUS_DISPATCH (conn);

		TEST_EQ_P (proxy->owner, NULL);
		TEST_FREE (last_owner);

		TEST_TRUE (my_lost_handler_called);

		my_lost_handler_called = FALSE;


		TEST_DBUS_OPEN (second_conn);

		assert (dbus_bus_request_name (second_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		TEST_DBUS_DISPATCH (conn);

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (second_conn));

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (second_conn);
		TEST_DBUS_CLOSE (conn);
	}


	/* Check that the lost handler may free the proxy structure. */
	TEST_FEATURE ("with free of proxy structure by handler");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (conn);

		my_lost_handler_called = FALSE;

		TEST_DBUS_OPEN (first_conn);

		assert (dbus_bus_request_name (first_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);


		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, conn,
						    "com.netsplit.Nih",
						    "/com/netsplit/Nih",
						    my_freeing_lost_handler, conn);
		}

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (first_conn));

		TEST_FREE_TAG (proxy);

		TEST_DBUS_CLOSE (first_conn);

		TEST_DBUS_DISPATCH (conn);

		TEST_FREE (proxy);
		TEST_TRUE (my_lost_handler_called);

		TEST_DBUS_CLOSE (conn);
	}


	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


static int my_signal_filter_called = FALSE;
static DBusConnection *    last_conn = NULL;
static DBusMessage *       last_message = NULL;
static NihDBusProxySignal *last_proxied = NULL;

static DBusHandlerResult
my_signal_filter (DBusConnection *    conn,
		  DBusMessage *       message,
		  NihDBusProxySignal *proxied)
{
	my_signal_filter_called++;

	TEST_NE_P (conn, NULL);
	last_conn = conn;

	TEST_NE_P (message, NULL);
	last_message = message;
	dbus_message_ref (message);

	TEST_NE_P (proxied, NULL);
	last_proxied = proxied;

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
my_signal_handler (void *          data,
		   NihDBusMessage *message,
		   ...)
{
}

const NihDBusSignal my_interface_signals[] = {
	{ "MySignal", NULL, my_signal_filter },
	{ NULL }
};
const NihDBusInterface my_interface = {
	"com.netsplit.Nih",
	NULL,
	my_interface_signals,
	NULL
};

void
test_connect (void)
{
	pid_t               dbus_pid;
	DBusConnection *    client_conn;
	DBusConnection *    server_conn;
	NihDBusProxy *      proxy = NULL;
	NihDBusProxySignal *proxied;
	NihError *          err;
	DBusMessage *       signal;
	dbus_uint32_t       serial;
	DBusError           dbus_error;

	TEST_FUNCTION ("nih_dbus_proxy_connect");
	TEST_DBUS (dbus_pid);


	/* Check that we can connect a signal to a bus connection, with the
	 * remote end identified by a unique name and having a proxied signal
	 * structure returned to us.  If a matching signal is then emitted by
	 * the server-side, the filter function is called with the expected
	 * arguments.
	 */
	TEST_FEATURE ("with bus connection by unique name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (client_conn);
		TEST_DBUS_OPEN (server_conn);

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    dbus_bus_get_unique_name (server_conn),
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		proxied = nih_dbus_proxy_connect (proxy,
						  &my_interface, "MySignal",
						  my_signal_handler, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (proxied, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);

			TEST_DBUS_CLOSE (client_conn);
			TEST_DBUS_CLOSE (server_conn);
			continue;
		}

		TEST_ALLOC_SIZE (proxied, sizeof (NihDBusProxySignal));
		TEST_EQ_P (proxied->proxy, proxy);
		TEST_EQ_P (proxied->interface, &my_interface);
		TEST_EQ_P (proxied->signal, &my_interface_signals[0]);
		TEST_EQ_P (proxied->handler, my_signal_handler);
		TEST_EQ_P (proxied->data, NULL);

		TEST_ALLOC_PARENT (proxied, proxy);

		my_signal_filter_called = FALSE;
		last_conn = NULL;
		last_message = NULL;
		last_proxied = NULL;

		signal = dbus_message_new_signal ("/com/netsplit/Nih",
						  "com.netsplit.Nih",
						  "MySignal");

		dbus_connection_send (server_conn, signal, &serial);
		dbus_connection_flush (server_conn);

		dbus_message_unref (signal);

		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (my_signal_filter_called);
		TEST_EQ_P (last_conn, client_conn);
		TEST_NE_P (last_message, NULL);
		TEST_EQ (dbus_message_get_serial (last_message), serial);
		TEST_TRUE (dbus_message_is_signal (last_message,
						   "com.netsplit.Nih",
						   "MySignal"));
		TEST_EQ_STR (dbus_message_get_path (last_message),
			     "/com/netsplit/Nih");
		TEST_EQ_P (last_proxied, proxied);
		dbus_message_unref (last_message);

		TEST_ALLOC_SAFE {
			nih_free (proxied);

			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);
	}


	/* Check that we can connect a signal to a bus connection, with the
	 * remote end identified by a well-known name and having a proxied
	 * signal structure returned to us.  If a matching signal is then emitted
	 * by the server-side, the filter function is called with the expected
	 * arguments.
	 */
	TEST_FEATURE ("with bus connection by well known name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (client_conn);
		TEST_DBUS_OPEN (server_conn);

		assert (dbus_bus_request_name (server_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    "com.netsplit.Nih",
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		proxied = nih_dbus_proxy_connect (proxy,
						  &my_interface, "MySignal",
						  my_signal_handler, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (proxied, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);

			TEST_DBUS_CLOSE (client_conn);
			TEST_DBUS_CLOSE (server_conn);
			continue;
		}

		TEST_ALLOC_SIZE (proxied, sizeof (NihDBusProxySignal));
		TEST_EQ_P (proxied->proxy, proxy);
		TEST_EQ_P (proxied->interface, &my_interface);
		TEST_EQ_P (proxied->signal, &my_interface_signals[0]);
		TEST_EQ_P (proxied->handler, my_signal_handler);
		TEST_EQ_P (proxied->data, NULL);

		TEST_ALLOC_PARENT (proxied, proxy);

		my_signal_filter_called = FALSE;
		last_conn = NULL;
		last_message = NULL;
		last_proxied = NULL;

		signal = dbus_message_new_signal ("/com/netsplit/Nih",
						  "com.netsplit.Nih",
						  "MySignal");

		dbus_connection_send (server_conn, signal, &serial);
		dbus_connection_flush (server_conn);

		dbus_message_unref (signal);

		TEST_DBUS_DISPATCH (client_conn);

		TEST_TRUE (my_signal_filter_called);
		TEST_EQ_P (last_conn, client_conn);
		TEST_NE_P (last_message, NULL);
		TEST_EQ (dbus_message_get_serial (last_message), serial);
		TEST_TRUE (dbus_message_is_signal (last_message,
						   "com.netsplit.Nih",
						   "MySignal"));
		TEST_EQ_STR (dbus_message_get_path (last_message),
			     "/com/netsplit/Nih");
		TEST_EQ_P (last_proxied, proxied);
		dbus_message_unref (last_message);

		TEST_ALLOC_SAFE {
			nih_free (proxied);

			nih_free (proxy);
		}


		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);
	}


	/* Check that we can also connect a signal to a peer-to-peer
	 * connection that does not have a name.  If a matching signal
	 * is then emitted by the other side, the filter function is
	 * called with the expected arguments.  We cheat for this and
	 * just make the client accept all signals rather than using a
	 * real peer-to-peer.
	 */
	TEST_FEATURE ("with peer-to-peer connection");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (client_conn);
		TEST_DBUS_OPEN (server_conn);

		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    NULL,
						    "/com/netsplit/Nih",
						    NULL, NULL);
		}

		proxied = nih_dbus_proxy_connect (proxy,
						  &my_interface, "MySignal",
						  my_signal_handler, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (proxied, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			nih_free (proxy);

			TEST_DBUS_CLOSE (client_conn);
			TEST_DBUS_CLOSE (server_conn);
			continue;
		}

		TEST_ALLOC_SIZE (proxied, sizeof (NihDBusProxySignal));
		TEST_EQ_P (proxied->proxy, proxy);
		TEST_EQ_P (proxied->interface, &my_interface);
		TEST_EQ_P (proxied->signal, &my_interface_signals[0]);
		TEST_EQ_P (proxied->handler, my_signal_handler);
		TEST_EQ_P (proxied->data, NULL);

		TEST_ALLOC_PARENT (proxied, proxy);

		my_signal_filter_called = FALSE;
		last_conn = NULL;
		last_message = NULL;
		last_proxied = NULL;

		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		dbus_error_free (&dbus_error);

		signal = dbus_message_new_signal ("/com/netsplit/Nih",
						  "com.netsplit.Nih",
						  "MySignal");

		dbus_connection_send (server_conn, signal, &serial);
		dbus_connection_flush (server_conn);

		dbus_message_unref (signal);

		TEST_DBUS_DISPATCH (client_conn);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		dbus_error_free (&dbus_error);

		TEST_TRUE (my_signal_filter_called);
		TEST_EQ_P (last_conn, client_conn);
		TEST_NE_P (last_message, NULL);
		TEST_EQ (dbus_message_get_serial (last_message), serial);
		TEST_TRUE (dbus_message_is_signal (last_message,
						   "com.netsplit.Nih",
						   "MySignal"));
		TEST_EQ_STR (dbus_message_get_path (last_message),
			     "/com/netsplit/Nih");
		TEST_EQ_P (last_proxied, proxied);
		dbus_message_unref (last_message);

		TEST_ALLOC_SAFE {
			nih_free (proxied);

			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (client_conn);
		TEST_DBUS_CLOSE (server_conn);
	}


	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_signal_destroy (void)
{
	pid_t               dbus_pid;
	DBusConnection *    client_conn;
	DBusConnection *    server_conn;
	NihDBusProxy *      proxy = NULL;
	NihDBusProxySignal *proxied = NULL;
	DBusMessage *       signal;
	DBusError           dbus_error;

	/* Check that when an NihDBusProxySignal structure is destroyed,
	 * a signal no longer results in the filter function being called.
	 */
	TEST_FUNCTION ("nih_dbus_proxy_signal_destroy");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			proxy = nih_dbus_proxy_new (NULL, client_conn,
						    NULL,
						    "/com/netsplit/Nih",
						    NULL, NULL);
			proxied = nih_dbus_proxy_connect (proxy,
							  &my_interface,
							  "MySignal",
							  my_signal_handler,
							  NULL);
		}

		my_signal_filter_called = FALSE;
		last_conn = NULL;
		last_message = NULL;
		last_proxied = NULL;

		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		dbus_error_free (&dbus_error);

		signal = dbus_message_new_signal ("/com/netsplit/Nih",
						  "com.netsplit.Nih",
						  "MySignal");

		dbus_connection_send (server_conn, signal, NULL);
		dbus_connection_flush (server_conn);

		dbus_message_unref (signal);

		/* Free the proxied signal */
		nih_free (proxied);

		TEST_DBUS_DISPATCH (client_conn);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		dbus_error_free (&dbus_error);

		TEST_FALSE (my_signal_filter_called);

		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


int
main (int   argc,
      char *argv[])
{
	nih_error_init ();

	test_new ();
	test_name_owner_changed ();

	test_connect ();
	test_signal_destroy ();

	return 0;
}
