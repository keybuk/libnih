int
my_method_sync (const void *  parent,
                NihDBusProxy *proxy,
                char ***      output,
                int32_t *     length)
{
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusError       error;
	DBusMessage *   reply;
	char **         output_local;
	DBusMessageIter output_local_iter;
	size_t          output_local_size;
	int32_t         length_local;

	nih_assert (proxy != NULL);
	nih_assert (output != NULL);
	nih_assert (length != NULL);

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

		/* Demarshal an array from the message */
		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_recurse (&iter, &output_local_iter);

		output_local_size = 0;
		output_local = NULL;

		output_local = nih_alloc (parent, sizeof (char *));
		if (! output_local) {
			*output = NULL;
			goto enomem;
		}

		output_local[output_local_size] = NULL;

		while (dbus_message_iter_get_arg_type (&output_local_iter) != DBUS_TYPE_INVALID) {
			const char *output_local_element_dbus;
			char **     output_local_tmp;
			char *      output_local_element;

			/* Demarshal a char * from the message */
			if (dbus_message_iter_get_arg_type (&output_local_iter) != DBUS_TYPE_STRING) {
				if (output_local)
					nih_free (output_local);
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_get_basic (&output_local_iter, &output_local_element_dbus);

			output_local_element = nih_strdup (output_local, output_local_element_dbus);
			if (! output_local_element) {
				if (output_local)
					nih_free (output_local);
				*output = NULL;
				goto enomem;
			}

			dbus_message_iter_next (&output_local_iter);

			if (output_local_size + 2 > SIZE_MAX / sizeof (char *)) {
				if (output_local)
					nih_free (output_local);
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			output_local_tmp = nih_realloc (output_local, parent, sizeof (char *) * (output_local_size + 2));
			if (! output_local_tmp) {
				if (output_local)
					nih_free (output_local);
				*output = NULL;
				goto enomem;
			}

			output_local = output_local_tmp;
			output_local[output_local_size] = output_local_element;
			output_local[output_local_size + 1] = NULL;

			output_local_size++;
		}

		dbus_message_iter_next (&iter);

		*output = output_local;
	enomem: __attribute__ ((unused));
	} while (! *output);

	do {
		__label__ enomem;

		/* Demarshal a int32_t from the message */
		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {
			nih_free (output_local);
			*output = NULL;
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&iter, &length_local);

		dbus_message_iter_next (&iter);

		*length = length_local;
	enomem: __attribute__ ((unused));
	} while (! *length);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		nih_free (output_local);
		*output = NULL;
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	dbus_message_unref (reply);

	return 0;
}
