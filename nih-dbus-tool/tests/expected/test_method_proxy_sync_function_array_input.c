int
my_method_sync (const void *   parent,
                NihDBusProxy * proxy,
                const int32_t *value,
                size_t         value_len)
{
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusError       error;
	DBusMessage *   reply;
	DBusMessageIter value_iter;

	nih_assert (proxy != NULL);
	nih_assert ((value_len == 0) || (value != NULL));

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "com.netsplit.Nih.Test", "Method");
	if (! method_call)
		nih_return_no_memory_error (-1);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	/* Marshal an array onto the message */
	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "i", &value_iter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	for (size_t value_i = 0; value_i < value_len; value_i++) {
		int32_t value_element;

		value_element = value[value_i];

		/* Marshal a int32_t onto the message */
		if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_INT32, &value_element)) {
			dbus_message_iter_abandon_container (&iter, &value_iter);
			dbus_message_unref (method_call);
			nih_return_no_memory_error (-1);
		}
	}

	if (! dbus_message_iter_close_container (&iter, &value_iter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	/* Send the message, and wait for the reply. */
	dbus_error_init (&error);

	reply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call, -1, &error);
	if (! reply) {
		dbus_message_unref (method_call);

		if (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) {
			nih_error_raise_no_memory ();
		} else {
			nih_dbus_error_raise (error.name, error.message);
		}

		dbus_error_free (&error);
		return -1;
	}

	dbus_message_unref (method_call);

	/* Iterate the arguments of the reply */
	dbus_message_iter_init (reply, &iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	dbus_message_unref (reply);

	return 0;
}
