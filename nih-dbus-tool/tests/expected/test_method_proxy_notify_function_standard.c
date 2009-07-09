void
my_com_netsplit_Nih_Test_Method_notify (DBusPendingCall *   pending_call,
                                        NihDBusPendingData *pending_data)
{
	DBusMessage *   reply;
	DBusMessageIter iter;
	NihDBusMessage *message;
	DBusError       error;
	char **         output;
	DBusMessageIter output_iter;
	size_t          output_size;
	int32_t         length;

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

		/* Demarshal an array from the message */
		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_recurse (&iter, &output_iter);

		output_size = 0;
		output = NULL;

		output = nih_alloc (message, sizeof (char *));
		if (! output) {
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		output[output_size] = NULL;

		while (dbus_message_iter_get_arg_type (&output_iter) != DBUS_TYPE_INVALID) {
			const char *output_element_dbus;
			char **     output_tmp;
			char *      output_element;

			/* Demarshal a char * from the message */
			if (dbus_message_iter_get_arg_type (&output_iter) != DBUS_TYPE_STRING) {
				if (output)
					nih_free (output);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&output_iter, &output_element_dbus);

			output_element = nih_strdup (output, output_element_dbus);
			if (! output_element) {
				if (output)
					nih_free (output);
				nih_free (message);
				message = NULL;
				goto enomem;
			}

			dbus_message_iter_next (&output_iter);

			if (output_size + 2 > SIZE_MAX / sizeof (char *)) {
				if (output)
					nih_free (output);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			output_tmp = nih_realloc (output, message, sizeof (char *) * (output_size + 2));
			if (! output_tmp) {
				if (output)
					nih_free (output);
				nih_free (message);
				message = NULL;
				goto enomem;
			}

			output = output_tmp;
			output[output_size] = output_element;
			output[output_size + 1] = NULL;

			output_size++;
		}

		dbus_message_iter_next (&iter);

		/* Demarshal a int32_t from the message */
		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&iter, &length);

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
		((MyMethodReply)pending_data->handler) (pending_data->data, message, output, length);
		nih_error_pop_context ();
	}

	nih_free (message);
	dbus_message_unref (reply);
}
