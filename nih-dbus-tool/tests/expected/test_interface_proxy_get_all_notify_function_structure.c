void
my_com_netsplit_Nih_Test_get_all_notify (DBusPendingCall *   pending_call,
                                         NihDBusPendingData *pending_data)
{
	DBusMessage *   reply;
	DBusMessageIter iter;
	DBusMessageIter arrayiter;
	DBusMessageIter dictiter;
	DBusMessageIter variter;
	NihDBusMessage *message;
	DBusError       error;
	const char *    property;
	MyProperties *  properties;
	size_t          property_count;
	char *          name;
	const char *    name_dbus;
	uint32_t        size;
	MyBirthday *    birthday;
	DBusMessageIter birthday_iter;
	int32_t         birthday_item0;
	int32_t         birthday_item1;
	int32_t         birthday_item2;

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

	/* Create a message context for the reply, and iterate
	 * over and recurse into the arguments.
	 */
	message = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));

	/* Iterate the method arguments, recursing into the array */
	dbus_message_iter_init (reply, &iter);

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

	properties = NIH_MUST (nih_new (message, MyProperties));
	property_count = 0;

	dbus_message_iter_recurse (&iter, &arrayiter);

	while (dbus_message_iter_get_arg_type (&arrayiter) != DBUS_TYPE_INVALID) {
		__label__ enomem;

		if (dbus_message_iter_get_arg_type (&arrayiter) != DBUS_TYPE_DICT_ENTRY) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_recurse (&arrayiter, &dictiter);

		if (dbus_message_iter_get_arg_type (&dictiter) != DBUS_TYPE_STRING) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&dictiter, &property);

		dbus_message_iter_next (&dictiter);

		if (dbus_message_iter_get_arg_type (&dictiter) != DBUS_TYPE_VARIANT) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_recurse (&dictiter, &variter);

		if (! strcmp (property, "name")) {
			/* Demarshal a char * from the message */
			if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&variter, &name_dbus);

			name = nih_strdup (properties, name_dbus);
			if (! name) {
				goto enomem;
			}

			dbus_message_iter_next (&variter);

			properties->name = name;

			nih_assert (++property_count);
		}

		if (! strcmp (property, "size")) {
			/* Demarshal a uint32_t from the message */
			if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&variter, &size);

			dbus_message_iter_next (&variter);

			properties->size = size;

			nih_assert (++property_count);
		}

		if (! strcmp (property, "birthday")) {
			/* Demarshal a structure from the message */
			if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRUCT) {
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_recurse (&variter, &birthday_iter);

			birthday = nih_new (properties, MyBirthday);
			if (! birthday) {
				goto enomem;
			}

			/* Demarshal a int32_t from the message */
			if (dbus_message_iter_get_arg_type (&birthday_iter) != DBUS_TYPE_INT32) {
				nih_free (birthday);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&birthday_iter, &birthday_item0);

			dbus_message_iter_next (&birthday_iter);

			birthday->item0 = birthday_item0;

			/* Demarshal a int32_t from the message */
			if (dbus_message_iter_get_arg_type (&birthday_iter) != DBUS_TYPE_INT32) {
				nih_free (birthday);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&birthday_iter, &birthday_item1);

			dbus_message_iter_next (&birthday_iter);

			birthday->item1 = birthday_item1;

			/* Demarshal a int32_t from the message */
			if (dbus_message_iter_get_arg_type (&birthday_iter) != DBUS_TYPE_INT32) {
				nih_free (birthday);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&birthday_iter, &birthday_item2);

			dbus_message_iter_next (&birthday_iter);

			birthday->item2 = birthday_item2;

			if (dbus_message_iter_get_arg_type (&birthday_iter) != DBUS_TYPE_INVALID) {
				nih_free (birthday);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_next (&variter);

			properties->birthday = birthday;

			nih_assert (++property_count);
		}

		dbus_message_iter_next (&dictiter);

		if (dbus_message_iter_get_arg_type (&dictiter) != DBUS_TYPE_INVALID) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_next (&arrayiter);
	enomem: __attribute__ ((unused));
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

	if (property_count < 3) {
		nih_error_push_context ();
		nih_error_raise (NIH_DBUS_INVALID_ARGS,
		                 _(NIH_DBUS_INVALID_ARGS_STR));
		pending_data->error_handler (pending_data->data, message);
		nih_error_pop_context ();

		nih_free (message);
		dbus_message_unref (reply);
		return;
	}

	/* Call the handler function */
	nih_error_push_context ();
	((MyGetAllReply)pending_data->handler) (pending_data->data, message, properties);
	nih_error_pop_context ();

	nih_free (message);
	dbus_message_unref (reply);
}
