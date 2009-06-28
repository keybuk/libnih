int
my_async_method_reply (NihDBusMessage *message,
                       const int32_t * output,
                       size_t          output_len)
{
	DBusMessage *   reply;
	DBusMessageIter iter;
	DBusMessageIter output_iter;

	nih_assert (message != NULL);
	nih_assert ((output_len == 0) || (output != NULL));

	/* If the sender doesn't care about a reply, don't bother wasting
	 * effort constructing and sending one.
	 */
	if (dbus_message_get_no_reply (message->message))
		return 0;

	/* Construct the reply message. */
	reply = dbus_message_new_method_return (message->message);
	if (! reply)
		return -1;

	dbus_message_iter_init_append (reply, &iter);

	/* Marshal an array onto the message */
	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "i", &output_iter)) {
		dbus_message_unref (reply);
		return -1;
	}

	for (size_t output_i = 0; output_i < output_len; output_i++) {
		int32_t output_element;

		output_element = output[output_i];

		/* Marshal a int32_t onto the message */
		if (! dbus_message_iter_append_basic (&output_iter, DBUS_TYPE_INT32, &output_element)) {
			dbus_message_iter_abandon_container (&iter, &output_iter);
			dbus_message_unref (reply);
			return -1;
		}
	}

	if (! dbus_message_iter_close_container (&iter, &output_iter)) {
		dbus_message_unref (reply);
		return -1;
	}

	/* Send the reply, appending it to the outgoing queue. */
	if (! dbus_connection_send (message->connection, reply, NULL)) {
		dbus_message_unref (reply);
		return -1;
	}

	dbus_message_unref (reply);

	return 0;
}
