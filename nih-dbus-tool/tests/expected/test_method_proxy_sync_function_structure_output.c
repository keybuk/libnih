int
my_method_sync (const void *        parent,
                NihDBusProxy *      proxy,
                MyMethodStructure **structure)
{
	DBusMessage *      method_call;
	DBusMessageIter    iter;
	DBusError          error;
	DBusMessage *      reply;
	MyMethodStructure *structure_local;
	DBusMessageIter    structure_local_iter;
	const char *       structure_local_item0_dbus;
	char *             structure_local_item0;
	uint32_t           structure_local_item1;

	nih_assert (proxy != NULL);
	nih_assert (structure != NULL);

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "com.netsplit.Nih.Test", "Method");
	if (! method_call)
		nih_return_no_memory_error (-1);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

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

	do {
		__label__ enomem;

		/* Demarshal a structure from the message */
		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRUCT) {
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_recurse (&iter, &structure_local_iter);

		structure_local = nih_new (parent, MyMethodStructure);
		if (! structure_local) {
			*structure = NULL;
			goto enomem;
		}

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&structure_local_iter) != DBUS_TYPE_STRING) {
			nih_free (structure_local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&structure_local_iter, &structure_local_item0_dbus);

		structure_local_item0 = nih_strdup (structure_local, structure_local_item0_dbus);
		if (! structure_local_item0) {
			nih_free (structure_local);
			*structure = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&structure_local_iter);

		structure_local->item0 = structure_local_item0;

		/* Demarshal a uint32_t from the message */
		if (dbus_message_iter_get_arg_type (&structure_local_iter) != DBUS_TYPE_UINT32) {
			nih_free (structure_local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&structure_local_iter, &structure_local_item1);

		dbus_message_iter_next (&structure_local_iter);

		structure_local->item1 = structure_local_item1;

		if (dbus_message_iter_get_arg_type (&structure_local_iter) != DBUS_TYPE_INVALID) {
			nih_free (structure_local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_next (&iter);

		*structure = structure_local;
	enomem: __attribute__ ((unused));
	} while (! *structure);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		nih_free (structure_local);
		*structure = NULL;
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	dbus_message_unref (reply);

	return 0;
}
