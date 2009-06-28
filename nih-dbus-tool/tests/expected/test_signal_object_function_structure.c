int
my_emit_signal (DBusConnection *         connection,
                const char *             origin_path,
                const MySignalStructure *structure)
{
	DBusMessage *   signal;
	DBusMessageIter iter;
	DBusMessageIter structure_iter;
	const char *    structure_item0;
	uint32_t        structure_item1;

	nih_assert (connection != NULL);
	nih_assert (origin_path != NULL);
	nih_assert (structure != NULL);

	/* Construct the message. */
	signal = dbus_message_new_signal (origin_path, "com.netsplit.Nih.Test", "Signal");
	if (! signal)
		return -1;

	dbus_message_iter_init_append (signal, &iter);

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT, NULL, &structure_iter)) {
		dbus_message_unref (signal);
		return -1;
	}

	structure_item0 = structure->item0;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&structure_iter, DBUS_TYPE_STRING, &structure_item0)) {
		dbus_message_iter_abandon_container (&iter, &structure_iter);
		dbus_message_unref (signal);
		return -1;
	}

	structure_item1 = structure->item1;

	/* Marshal a uint32_t onto the message */
	if (! dbus_message_iter_append_basic (&structure_iter, DBUS_TYPE_UINT32, &structure_item1)) {
		dbus_message_iter_abandon_container (&iter, &structure_iter);
		dbus_message_unref (signal);
		return -1;
	}

	if (! dbus_message_iter_close_container (&iter, &structure_iter)) {
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
