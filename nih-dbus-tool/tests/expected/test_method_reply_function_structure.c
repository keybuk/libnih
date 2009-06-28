int
my_async_method_reply (NihDBusMessage *              message,
                       const MyAsyncMethodStructure *structure)
{
	DBusMessage *   reply;
	DBusMessageIter iter;
	DBusMessageIter structure_iter;
	const char *    structure_item0;
	uint32_t        structure_item1;

	nih_assert (message != NULL);
	nih_assert (structure != NULL);

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

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT, NULL, &structure_iter)) {
		dbus_message_unref (reply);
		return -1;
	}

	structure_item0 = structure->item0;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&structure_iter, DBUS_TYPE_STRING, &structure_item0)) {
		dbus_message_iter_abandon_container (&iter, &structure_iter);
		dbus_message_unref (reply);
		return -1;
	}

	structure_item1 = structure->item1;

	/* Marshal a uint32_t onto the message */
	if (! dbus_message_iter_append_basic (&structure_iter, DBUS_TYPE_UINT32, &structure_item1)) {
		dbus_message_iter_abandon_container (&iter, &structure_iter);
		dbus_message_unref (reply);
		return -1;
	}

	if (! dbus_message_iter_close_container (&iter, &structure_iter)) {
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
