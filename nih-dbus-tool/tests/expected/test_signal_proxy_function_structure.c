DBusHandlerResult
my_com_netsplit_Nih_Test_Signal_signal (DBusConnection *    connection,
                                        DBusMessage *       signal,
                                        NihDBusProxySignal *proxied)
{
	DBusMessageIter    iter;
	NihDBusMessage *   message;
	MySignalStructure *structure;
	DBusMessageIter    structure_iter;
	const char *       structure_item0_dbus;
	char *             structure_item0;
	uint32_t           structure_item1;

	nih_assert (connection != NULL);
	nih_assert (signal != NULL);
	nih_assert (proxied != NULL);
	nih_assert (connection == proxied->proxy->connection);

	if (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (! dbus_message_has_path (signal, proxied->proxy->path))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (proxied->proxy->name)
		if (! dbus_message_has_sender (signal, proxied->proxy->owner))
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	message = nih_dbus_message_new (NULL, connection, signal);
	if (! message)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Iterate the arguments to the signal and demarshal into arguments
	 * for our own function call.
	 */
	dbus_message_iter_init (message->message, &iter);

	/* Demarshal a structure from the message */
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRUCT) {
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_recurse (&iter, &structure_iter);

	structure = nih_new (message, MySignalStructure);
	if (! structure) {
		nih_free (message);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	/* Demarshal a char * from the message */
	if (dbus_message_iter_get_arg_type (&structure_iter) != DBUS_TYPE_STRING) {
		nih_free (structure);
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_get_basic (&structure_iter, &structure_item0_dbus);

	structure_item0 = nih_strdup (structure, structure_item0_dbus);
	if (! structure_item0) {
		nih_free (structure);
		nih_free (message);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_iter_next (&structure_iter);

	structure->item0 = structure_item0;

	/* Demarshal a uint32_t from the message */
	if (dbus_message_iter_get_arg_type (&structure_iter) != DBUS_TYPE_UINT32) {
		nih_free (structure);
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_get_basic (&structure_iter, &structure_item1);

	dbus_message_iter_next (&structure_iter);

	structure->item1 = structure_item1;

	if (dbus_message_iter_get_arg_type (&structure_iter) != DBUS_TYPE_INVALID) {
		nih_free (structure);
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_next (&iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	/* Call the handler function */
	nih_error_push_context ();
	((MySignalHandler)proxied->handler) (proxied->data, message, structure);
	nih_error_pop_context ();
	nih_free (message);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
