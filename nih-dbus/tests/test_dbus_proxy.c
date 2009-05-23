/* libnih
 *
 * test_dbus_proxy.c - test suite for nih-dbus/dbus_proxy.c
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
	TEST_EQ_P (data, proxy->conn);
}

static void
my_freeing_lost_handler (void *        data,
			 NihDBusProxy *proxy)
{
	my_lost_handler_called++;

	TEST_NE_P (proxy, NULL);
	TEST_EQ_P (data, proxy->conn);

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
	TEST_FEATURE ("without name or handler");
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

		TEST_EQ_P (proxy->conn, conn);

		TEST_EQ_P (proxy->name, NULL);

		TEST_EQ_P (proxy->owner, NULL);

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_EQ_P (proxy->lost_handler, NULL);
		TEST_EQ_P (proxy->data, NULL);

		nih_free (proxy);
	}


	/* Check that we can create a simple proxy for a remote object on
	 * a managed bus connection, and have a proxy object returned with
	 * the right detailed filled in.
	 */
	TEST_FEATURE ("without handler");
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

		TEST_EQ_P (proxy->conn, conn);

		TEST_ALLOC_PARENT (proxy->name, proxy);
		TEST_EQ_STR (proxy->name, "com.netsplit.Nih");

		TEST_EQ_P (proxy->owner, NULL);

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_EQ_P (proxy->lost_handler, NULL);
		TEST_EQ_P (proxy->data, NULL);

		nih_free (proxy);
	}


	/* Check that we can pass a lost handler function which looks up
	 * whether the name is on the bus, and sets up a match for it.
	 * If the name does not exist on the bus, NULL should be set for
	 * the owner.
	 */
	TEST_FEATURE ("with lost handler and unconnected name");
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

		TEST_EQ_P (proxy->conn, conn);

		TEST_ALLOC_PARENT (proxy->name, proxy);
		TEST_EQ_STR (proxy->name, "com.netsplit.Nih");

		TEST_EQ_P (proxy->owner, NULL);

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_EQ_P (proxy->lost_handler, my_lost_handler);
		TEST_EQ_P (proxy->data, conn);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}
	}


	/* Check that we can pass a lost handler function when the name
	 * does exist on the bus, and that the unique name of the owner
	 * is stored in the owner member.
	 */
	TEST_FEATURE ("with lost handler and connected name");
	TEST_ALLOC_FAIL {
		TEST_DBUS_OPEN (other_conn);

		assert (dbus_bus_request_name (other_conn, "com.netsplit.Nih",
					       0, NULL)
			== DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

		proxy = nih_dbus_proxy_new (NULL, conn, "com.netsplit.Nih",
					    "/com/netsplit/Nih",
					    my_lost_handler, conn);

		if (test_alloc_failed) {
			TEST_EQ_P (proxy, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_DBUS_CLOSE (other_conn);
			continue;
		}

		TEST_ALLOC_SIZE (proxy, sizeof (NihDBusProxy));

		TEST_EQ_P (proxy->conn, conn);

		TEST_ALLOC_PARENT (proxy->name, proxy);
		TEST_EQ_STR (proxy->name, "com.netsplit.Nih");

		TEST_ALLOC_PARENT (proxy->owner, proxy);
		TEST_EQ_STR (proxy->owner, dbus_bus_get_unique_name (other_conn));

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_EQ_P (proxy->lost_handler, my_lost_handler);
		TEST_EQ_P (proxy->data, conn);

		/* Constructs the rule when we free */
		TEST_ALLOC_SAFE {
			nih_free (proxy);
		}

		TEST_DBUS_CLOSE (other_conn);
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
	NihDBusProxy *  proxy;
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


int
main (int   argc,
      char *argv[])
{
	nih_error_init ();

	test_new ();
	test_name_owner_changed ();

	return 0;
}
