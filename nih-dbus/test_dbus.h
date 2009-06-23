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

#ifndef NIH_TEST_DBUS_H
#define NIH_TEST_DBUS_H

#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dbus/dbus.h>


/**
 * TEST_DBUS:
 * @_pid: variable to store pid in.
 *
 * Spawns a D-Bus bus daemon which may be used for testing purposes.  You
 * may create connections to the daemon with either the system bus or session
 * bus addresses, which will both contain its address; alternatively use
 * TEST_DBUS_OPEN() to establish a connection.
 *
 * The pid of the daemon process is stored in @_pid.
 *
 * The daemon should be killed with TEST_DBUS_END() once you have finished
 * with it.
 **/
#define TEST_DBUS(_pid)							\
	do {								\
		int _test_fds[2];					\
		assert0 (pipe (_test_fds));				\
		assert ((_pid = fork ()) >= 0);				\
		if (_pid > 0) {						\
			char _test_address[128] = { 0 };		\
			close (_test_fds[1]);				\
			assert (read (_test_fds[0], _test_address,	\
				      sizeof (_test_address)) > 0);	\
			close (_test_fds[0]);				\
									\
			while (strrchr (_test_address, '\n'))		\
				*(strrchr (_test_address, '\n')) = '\0'; \
									\
			assert0 (setenv ("DBUS_SYSTEM_BUS_ADDRESS",	\
					 _test_address, TRUE));		\
			assert0 (setenv ("DBUS_SESSION_BUS_ADDRESS",	\
					 _test_address, TRUE));		\
		} else if (_pid == 0) {					\
			close (_test_fds[0]);				\
			assert (dup2 (_test_fds[1], STDOUT_FILENO) >= 0); \
			assert0 (execlp ("dbus-daemon", "test_dbus-daemon",	\
					 "--session", "--print-address", NULL)); \
			exit (255);					\
		}							\
	} while (0)

/**
 * TEST_DBUS_OPEN:
 * @_conn: variable to store connection in.
 *
 * Creates a new connection to the temporary D-Bus server, ensuring that
 * it won't exit on disconnect and removing the NameAcquired signal from
 * the incoming queue.
 *
 * The connection may be closed again using TEST_DBUS_CLOSE().
 **/
#define TEST_DBUS_OPEN(_conn)						\
	do {								\
		DBusMessage *_test_message;				\
									\
		assert ((_conn = dbus_bus_get_private (DBUS_BUS_SYSTEM, NULL)) != NULL); \
		dbus_connection_set_exit_on_disconnect (_conn, FALSE);	\
									\
		while (! (_test_message = dbus_connection_pop_message (_conn))) \
			dbus_connection_read_write (_conn, -1);		\
									\
		assert (dbus_message_is_signal (			\
				_test_message, DBUS_INTERFACE_DBUS,	\
				"NameAcquired"));			\
									\
		dbus_message_unref (_test_message);			\
	} while (0)

/**
 * TEST_DBUS_MESSAGE:
 * @_conn: connection,
 * @_message: variable to store message in.
 *
 * Waits for a single message to arrive and pops it from the incoming queue,
 * placing it in @_message.
 **/
#define TEST_DBUS_MESSAGE(_conn, _message)			   \
	while (! (_message = dbus_connection_pop_message (_conn))) \
		dbus_connection_read_write (_conn, -1);

/**
 * TEST_DBUS_DISPATCH:
 * @_conn: connection.
 *
 * Reads and writes from the connection @_conn and dispatches whatever
 * messages were received or sent in that pulse.
 **/
#define TEST_DBUS_DISPATCH(_conn)					\
	do {								\
		dbus_connection_read_write (_conn, -1);			\
		while (dbus_connection_dispatch (_conn) != DBUS_DISPATCH_COMPLETE) \
			;						\
	} while (0)

/**
 * TEST_DBUS_CLOSE:
 * @_conn: connection to close.
 *
 * Closes the connection opened with TEST_DBUS_OPEN().
 **/
#define TEST_DBUS_CLOSE(_conn)			\
	do {					\
		dbus_connection_close (_conn);	\
		dbus_connection_unref (_conn);	\
	} while (0)

/**
 * TEST_DBUS_END:
 * @_pid: pid of dbus daemon.
 *
 * Terminates a dbus daemon that has been used for testing.
 **/
#define TEST_DBUS_END(_pid)				\
	do {						\
		kill (_pid, SIGTERM);			\
		waitpid (_pid, NULL, 0);		\
		unsetenv ("DBUS_SESSION_BUS_ADDRESS");	\
		unsetenv ("DBUS_SYSTEM_BUS_ADDRESS");	\
	} while (0)

#endif /* NIH_TEST_DBUS_H */
