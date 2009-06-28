int
my_emit_signal (DBusConnection *connection,
                const char *    origin_path,
                const int32_t * value,
                size_t          value_len)
{
	DBusMessage *   signal;
	DBusMessageIter iter;
	DBusMessageIter value_iter;

	nih_assert (connection != NULL);
	nih_assert (origin_path != NULL);
	nih_assert ((value_len == 0) || (value != NULL));

	/* Construct the message. */
	signal = dbus_message_new_signal (origin_path, "com.netsplit.Nih.Test", "Signal");
	if (! signal)
		return -1;

	dbus_message_iter_init_append (signal, &iter);

	/* Marshal an array onto the message */
	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "i", &value_iter)) {
		dbus_message_unref (signal);
		return -1;
	}

	for (size_t value_i = 0; value_i < value_len; value_i++) {
		int32_t value_element;

		value_element = value[value_i];

		/* Marshal a int32_t onto the message */
		if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_INT32, &value_element)) {
			dbus_message_iter_abandon_container (&iter, &value_iter);
			dbus_message_unref (signal);
			return -1;
		}
	}

	if (! dbus_message_iter_close_container (&iter, &value_iter)) {
		dbus_message_unref (signal);
		return -1;
	}

	/* Send the signal, appending it to the outgoing queue. */
	if (! dbus_connection_send (connection, signal, NULL)) {
		dbus_message_unref (signal);
		return -1;
	}

	dbus_message_unref (signal);

	return 0;
}
