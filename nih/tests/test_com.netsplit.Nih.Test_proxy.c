/* libnih
 *
 * test_com.netsplit.Nih.Test_proxy.c - test suite for auto-generated
 * proxy bindings.
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

#include <nih/test.h>

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/error.h>
#include <nih/errors.h>

#include <nih/dbus.h>

#include "com.netsplit.Nih.Test_proxy.h"
#include "com.netsplit.Nih.Test_impl.h"


void
test_method_dispatch (void)
{
	DBusConnection *conn;
	NihDBusProxy   *proxy;
	NihError       *err;
	NihDBusError   *dbus_err;
	char           *output;
	int             ret;

	TEST_GROUP ("method dispatching");


	/* Check that we can make a D-Bus method call, passing in the
	 * expected arguments and receiving the expected arguments in the
	 * reply.
	 */
	TEST_FEATURE ("with valid argument");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_method (proxy, "test data", 0, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "test data");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that if the method call returns a D-Bus error, the proxy
	 * call returns a negative number and raises the same D-Bus error.
	 */
	TEST_FEATURE ("with returned D-Bus error");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_method (proxy, "test data", 1, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

	dbus_err = (NihDBusError *)err;
	TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.IllegalValue");

	nih_free (dbus_err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that in out of memory conditions, D-Bus automatically
	 * repeats the method call so we don't notice on the client side.
	 */
	TEST_FEATURE ("with out of memory error");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_method (proxy, "test data", 2, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "test data");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an error unknown to D-Bus is turned into a generic
	 * failed error.
	 */
	TEST_FEATURE ("with unknown error");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_method (proxy, "test data", 3, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

	dbus_err = (NihDBusError *)err;
	TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

	nih_free (dbus_err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that the fact the server implementation is asynchronous
	 * is hidden and the call blocks until the reply comes back anyway.
	 */
	TEST_FEATURE ("with valid argument to async call");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 0, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "test data");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an error returned from an asynchronous server-side
	 * call still comes back as an error.
	 */
	TEST_FEATURE ("with returned D-Bus error from async call");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 1, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

	dbus_err = (NihDBusError *)err;
	TEST_EQ_STR (dbus_err->name, "com.netsplit.Nih.IllegalValue");

	nih_free (dbus_err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that in out of memory conditions, D-Bus automatically
	 * repeats the method call so we don't notice on the client side
	 * even for async server-side calls.
	 */
	TEST_FEATURE ("with out of memory error from async call");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 2, &output);

	TEST_EQ (ret, 0);

	TEST_NE_P (output, NULL);
	TEST_ALLOC_PARENT (output, proxy);
	TEST_EQ_STR (output, "test data");

	nih_free (proxy);

	my_teardown (conn);


	/* Check that an error unknown to D-Bus is turned into a generic
	 * failed error.
	 */
	TEST_FEATURE ("with unknown error from async call");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 3, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));

	dbus_err = (NihDBusError *)err;
	TEST_EQ_STR (dbus_err->name, DBUS_ERROR_FAILED);

	nih_free (dbus_err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that a condition whereby the wrong arguments are returned
	 * from a method call results in a special illegal arguments error
	 * being returned.
	 */
	TEST_FEATURE ("with wrong argument type in reply");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 4, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
	nih_free (err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that a condition whereby too many arguments are returned
	 * from a method call results in a special illegal arguments error
	 * being returned.
	 */
	TEST_FEATURE ("with too many arguments in reply");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 5, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
	nih_free (err);

	nih_free (proxy);

	my_teardown (conn);


	/* Check that a condition whereby arguments are missing from the
	 * method call return results in a special illegal arguments error
	 * being returned.
	 */
	TEST_FEATURE ("with missing arguments in reply");
	conn = my_setup ();
	proxy = nih_dbus_proxy_new (NULL, conn, NULL, "/com/netsplit/Nih");

	output = NULL;

	ret = proxy_test_async_method (proxy, "test data", 6, &output);

	TEST_LT (ret, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_INVALID_ARGS);
	nih_free (err);

	nih_free (proxy);

	my_teardown (conn);
}


int
main (int   argc,
      char *argv[])
{
	test_method_dispatch ();

	return 0;
}
