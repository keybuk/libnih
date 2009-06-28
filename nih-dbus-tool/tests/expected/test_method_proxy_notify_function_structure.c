void
my_com_netsplit_Nih_Test_Method_notify (DBusPendingCall *   pending_call,
                                        NihDBusPendingData *pending_data)
{
	DBusMessage *      reply;
	DBusMessageIter    iter;
	NihDBusMessage *   message;
	DBusError          error;
	MyMethodStructure *structure;
	DBusMessageIter    structure_iter;
	const char *       structure_item0_dbus;
	char *             structure_item0;
	uint32_t           structure_item1;

	nih_assert (pending_call != NULL);
	nih_assert (pending_data != NULL);

	nih_assert (dbus_pending_call_get_completed (pending_call));

	/* Steal the reply from the pending call. */
	reply = dbus_pending_call_steal_reply (pending_call);
	nih_assert (reply != NULL);

	/* Handle error replies */
	if (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {
		message = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));

		dbus_error_init (&error);
		dbus_set_error_from_message (&error, message->message);

		nih_error_push_context ();
		nih_dbus_error_raise (error.name, error.message);
		pending_data->error_handler (pending_data->data, message);
		nih_error_pop_context ();

		dbus_error_free (&error);
		nih_free (message);
		dbus_message_unref (reply);
		return;
	}

	nih_assert (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_METHOD_RETURN);

	do {
		__label__ enomem;

		/* Create a message context for the reply, and iterate
		 * over its arguments.
		 */
		message = nih_dbus_message_new (pending_data, pending_data->connection, reply);
		if (! message)
			goto enomem;

		dbus_message_iter_init (message->message, &iter);

		/* Demarshal a structure from the message */
		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRUCT) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_recurse (&iter, &structure_iter);

		structure = nih_new (message, MyMethodStructure);
		if (! structure) {
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&structure_iter) != DBUS_TYPE_STRING) {
			nih_free (structure);
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&structure_iter, &structure_item0_dbus);

		structure_item0 = nih_strdup (structure, structure_item0_dbus);
		if (! structure_item0) {
			nih_free (structure);
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&structure_iter);

		structure->item0 = structure_item0;

		/* Demarshal a uint32_t from the message */
		if (dbus_message_iter_get_arg_type (&structure_iter) != DBUS_TYPE_UINT32) {
			nih_free (structure);
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&structure_iter, &structure_item1);

		dbus_message_iter_next (&structure_iter);

		structure->item1 = structure_item1;

		if (dbus_message_iter_get_arg_type (&structure_iter) != DBUS_TYPE_INVALID) {
			nih_free (structure);
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_next (&iter);

		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

	enomem: __attribute__ ((unused));
	} while (! message);

	/* Call the handler function */
	if (pending_data->handler) {
		nih_error_push_context ();
		((MyMethodReply)pending_data->handler) (pending_data->data, message, structure);
		nih_error_pop_context ();
	}

	nih_free (message);
	dbus_message_unref (reply);
}
