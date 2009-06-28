DBusHandlerResult
my_com_netsplit_Nih_Test_Method_method (NihDBusObject * object,
                                        NihDBusMessage *message)
{
	DBusMessageIter    iter;
	DBusMessage *      reply;
	MyMethodStructure *structure;
	DBusMessageIter    structure_iter;
	const char *       structure_item0;
	uint32_t           structure_item1;

	nih_assert (object != NULL);
	nih_assert (message != NULL);

	/* Iterate the arguments to the message and demarshal into arguments
	 * for our own function call.
	 */
	dbus_message_iter_init (message->message, &iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		reply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,
		                                "Invalid arguments to Method method");
		if (! reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		if (! dbus_connection_send (message->connection, reply, NULL)) {
			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	/* Call the handler function */
	nih_error_push_context ();
	if (my_method (object->data, message, &structure) < 0) {
		NihError *err;

		err = nih_error_get ();
		if (err->number == ENOMEM) {
			nih_free (err);
			nih_error_pop_context ();

			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		} else if (err->number == NIH_DBUS_ERROR) {
			NihDBusError *dbus_err = (NihDBusError *)err;

			reply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));
			nih_free (err);
			nih_error_pop_context ();

			NIH_MUST (dbus_connection_send (message->connection, reply, NULL));

			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_HANDLED;
		} else {
			reply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));
			nih_free (err);
			nih_error_pop_context ();

			NIH_MUST (dbus_connection_send (message->connection, reply, NULL));

			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
	}
	nih_error_pop_context ();

	/* If the sender doesn't care about a reply, don't bother wasting
	 * effort constructing and sending one.
	 */
	if (dbus_message_get_no_reply (message->message))
		return DBUS_HANDLER_RESULT_HANDLED;

	do {
		__label__ enomem;

		/* Construct the reply message. */
		reply = dbus_message_new_method_return (message->message);
		if (! reply)
			goto enomem;

		dbus_message_iter_init_append (reply, &iter);

		/* Marshal a structure onto the message */
		if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT, NULL, &structure_iter)) {
			dbus_message_unref (reply);
			reply = NULL;
			goto enomem;
		}

		structure_item0 = structure->item0;

		/* Marshal a char * onto the message */
		if (! dbus_message_iter_append_basic (&structure_iter, DBUS_TYPE_STRING, &structure_item0)) {
			dbus_message_iter_abandon_container (&iter, &structure_iter);
			dbus_message_unref (reply);
			reply = NULL;
			goto enomem;
		}

		structure_item1 = structure->item1;

		/* Marshal a uint32_t onto the message */
		if (! dbus_message_iter_append_basic (&structure_iter, DBUS_TYPE_UINT32, &structure_item1)) {
			dbus_message_iter_abandon_container (&iter, &structure_iter);
			dbus_message_unref (reply);
			reply = NULL;
			goto enomem;
		}

		if (! dbus_message_iter_close_container (&iter, &structure_iter)) {
			dbus_message_unref (reply);
			reply = NULL;
			goto enomem;
		}
	enomem: __attribute__ ((unused));
	} while (! reply);

	/* Send the reply, appending it to the outgoing queue. */
	NIH_MUST (dbus_connection_send (message->connection, reply, NULL));

	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}
