DBusPendingCall *
my_test_search (NihDBusProxy *          proxy,
                const MyTestSearchItem *item,
                MyTestSearchReply       handler,
                NihDBusErrorHandler     error_handler,
                void *                  data,
                int                     timeout)
{
	DBusMessage *       method_call;
	DBusMessageIter     iter;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data;
	DBusMessageIter     item_iter;
	const char *        item_item0;
	uint32_t            item_item1;

	nih_assert (proxy != NULL);
	nih_assert (item != NULL);
	nih_assert ((handler == NULL) || (error_handler != NULL));

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "com.netsplit.Nih.Test", "Search");
	if (! method_call)
		nih_return_no_memory_error (NULL);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT, NULL, &item_iter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	item_item0 = item->item0;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&item_iter, DBUS_TYPE_STRING, &item_item0)) {
		dbus_message_iter_abandon_container (&iter, &item_iter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	item_item1 = item->item1;

	/* Marshal a uint32_t onto the message */
	if (! dbus_message_iter_append_basic (&item_iter, DBUS_TYPE_UINT32, &item_item1)) {
		dbus_message_iter_abandon_container (&iter, &item_iter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	if (! dbus_message_iter_close_container (&iter, &item_iter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Handle a fire-and-forget message */
	if (! error_handler) {
		dbus_message_set_no_reply (method_call, TRUE);
		if (! dbus_connection_send (proxy->connection, method_call, NULL)) {
			dbus_message_unref (method_call);
			nih_return_no_memory_error (NULL);
		}

		dbus_message_unref (method_call);
		return (DBusPendingCall *)TRUE;
	}

	/* Send the message and set up the reply notification. */
	pending_data = nih_dbus_pending_data_new (NULL, proxy->connection,
	                                          (NihDBusReplyHandler)handler,
	                                          error_handler, data);
	if (! pending_data) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	pending_call = NULL;
	if (! dbus_connection_send_with_reply (proxy->connection, method_call,
	                                       &pending_call, timeout)) {
		dbus_message_unref (method_call);
		nih_free (pending_data);
		nih_return_no_memory_error (NULL);
	}

	dbus_message_unref (method_call);

	if (! pending_call) {
		nih_dbus_error_raise (DBUS_ERROR_DISCONNECTED,
		                      "Connection is closed");
		nih_free (pending_data);
		return NULL;
	}

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_Search_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_Search_notify (DBusPendingCall *   pending_call,
                                        NihDBusPendingData *pending_data)
{
	DBusMessage *       reply;
	DBusMessageIter     iter;
	NihDBusMessage *    message;
	DBusError           error;
	MyTestSearchResult *result;
	DBusMessageIter     result_iter;
	const char *        result_item0_dbus;
	char *              result_item0;
	const char *        result_item1_dbus;
	char *              result_item1;

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

		dbus_message_iter_recurse (&iter, &result_iter);

		result = nih_new (message, MyTestSearchResult);
		if (! result) {
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&result_iter) != DBUS_TYPE_STRING) {
			nih_free (result);
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&result_iter, &result_item0_dbus);

		result_item0 = nih_strdup (result, result_item0_dbus);
		if (! result_item0) {
			nih_free (result);
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&result_iter);

		result->item0 = result_item0;

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&result_iter) != DBUS_TYPE_STRING) {
			nih_free (result);
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&result_iter, &result_item1_dbus);

		result_item1 = nih_strdup (result, result_item1_dbus);
		if (! result_item1) {
			nih_free (result);
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&result_iter);

		result->item1 = result_item1;

		if (dbus_message_iter_get_arg_type (&result_iter) != DBUS_TYPE_INVALID) {
			nih_free (result);
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
		((MyTestSearchReply)pending_data->handler) (pending_data->data, message, result);
		nih_error_pop_context ();
	}

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_search_sync (const void *            parent,
                     NihDBusProxy *          proxy,
                     const MyTestSearchItem *item,
                     MyTestSearchResult **   result)
{
	DBusMessage *       method_call;
	DBusMessageIter     iter;
	DBusError           error;
	DBusMessage *       reply;
	DBusMessageIter     item_iter;
	const char *        item_item0;
	uint32_t            item_item1;
	MyTestSearchResult *result_local;
	DBusMessageIter     result_local_iter;
	const char *        result_local_item0_dbus;
	char *              result_local_item0;
	const char *        result_local_item1_dbus;
	char *              result_local_item1;

	nih_assert (proxy != NULL);
	nih_assert (item != NULL);
	nih_assert (result != NULL);

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "com.netsplit.Nih.Test", "Search");
	if (! method_call)
		nih_return_no_memory_error (-1);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT, NULL, &item_iter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	item_item0 = item->item0;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&item_iter, DBUS_TYPE_STRING, &item_item0)) {
		dbus_message_iter_abandon_container (&iter, &item_iter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	item_item1 = item->item1;

	/* Marshal a uint32_t onto the message */
	if (! dbus_message_iter_append_basic (&item_iter, DBUS_TYPE_UINT32, &item_item1)) {
		dbus_message_iter_abandon_container (&iter, &item_iter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	if (! dbus_message_iter_close_container (&iter, &item_iter)) {
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

	do {
		__label__ enomem;

		/* Demarshal a structure from the message */
		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRUCT) {
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_recurse (&iter, &result_local_iter);

		result_local = nih_new (parent, MyTestSearchResult);
		if (! result_local) {
			*result = NULL;
			goto enomem;
		}

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&result_local_iter) != DBUS_TYPE_STRING) {
			nih_free (result_local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&result_local_iter, &result_local_item0_dbus);

		result_local_item0 = nih_strdup (result_local, result_local_item0_dbus);
		if (! result_local_item0) {
			nih_free (result_local);
			*result = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&result_local_iter);

		result_local->item0 = result_local_item0;

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&result_local_iter) != DBUS_TYPE_STRING) {
			nih_free (result_local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&result_local_iter, &result_local_item1_dbus);

		result_local_item1 = nih_strdup (result_local, result_local_item1_dbus);
		if (! result_local_item1) {
			nih_free (result_local);
			*result = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&result_local_iter);

		result_local->item1 = result_local_item1;

		if (dbus_message_iter_get_arg_type (&result_local_iter) != DBUS_TYPE_INVALID) {
			nih_free (result_local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_next (&iter);

		*result = result_local;
	enomem: __attribute__ ((unused));
	} while (! *result);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		nih_free (result_local);
		*result = NULL;
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	dbus_message_unref (reply);

	return 0;
}


static DBusHandlerResult
my_com_netsplit_Nih_Test_NewSearch_signal (DBusConnection *    connection,
                                           DBusMessage *       signal,
                                           NihDBusProxySignal *proxied)
{
	DBusMessageIter       iter;
	NihDBusMessage *      message;
	MyTestNewSearchQuery *query;
	DBusMessageIter       query_iter;
	const char *          query_item0_dbus;
	char *                query_item0;
	const char *          query_item1_dbus;
	char *                query_item1;
	uint32_t              query_item2;

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

	dbus_message_iter_recurse (&iter, &query_iter);

	query = nih_new (message, MyTestNewSearchQuery);
	if (! query) {
		nih_free (message);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	/* Demarshal a char * from the message */
	if (dbus_message_iter_get_arg_type (&query_iter) != DBUS_TYPE_STRING) {
		nih_free (query);
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_get_basic (&query_iter, &query_item0_dbus);

	query_item0 = nih_strdup (query, query_item0_dbus);
	if (! query_item0) {
		nih_free (query);
		nih_free (message);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_iter_next (&query_iter);

	query->item0 = query_item0;

	/* Demarshal a char * from the message */
	if (dbus_message_iter_get_arg_type (&query_iter) != DBUS_TYPE_STRING) {
		nih_free (query);
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_get_basic (&query_iter, &query_item1_dbus);

	query_item1 = nih_strdup (query, query_item1_dbus);
	if (! query_item1) {
		nih_free (query);
		nih_free (message);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_iter_next (&query_iter);

	query->item1 = query_item1;

	/* Demarshal a uint32_t from the message */
	if (dbus_message_iter_get_arg_type (&query_iter) != DBUS_TYPE_UINT32) {
		nih_free (query);
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_get_basic (&query_iter, &query_item2);

	dbus_message_iter_next (&query_iter);

	query->item2 = query_item2;

	if (dbus_message_iter_get_arg_type (&query_iter) != DBUS_TYPE_INVALID) {
		nih_free (query);
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
	((MyTestNewSearchHandler)proxied->handler) (proxied->data, message, query);
	nih_error_pop_context ();
	nih_free (message);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


DBusPendingCall *
my_test_get_last_search (NihDBusProxy *           proxy,
                         MyTestGetLastSearchReply handler,
                         NihDBusErrorHandler      error_handler,
                         void *                   data,
                         int                      timeout)
{
	DBusMessage *       method_call;
	DBusMessageIter     iter;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data;
	const char *        interface;
	const char *        property;

	nih_assert (proxy != NULL);
	nih_assert ((handler != NULL) && (error_handler != NULL));

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "Get");
	if (! method_call)
		nih_return_no_memory_error (NULL);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	property = "last_search";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Send the message and set up the reply notification. */
	pending_data = nih_dbus_pending_data_new (NULL, proxy->connection,
	                                          (NihDBusReplyHandler)handler,
	                                          error_handler, data);
	if (! pending_data) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	pending_call = NULL;
	if (! dbus_connection_send_with_reply (proxy->connection, method_call,
	                                       &pending_call, timeout)) {
		dbus_message_unref (method_call);
		nih_free (pending_data);
		nih_return_no_memory_error (NULL);
	}

	dbus_message_unref (method_call);

	if (! pending_call) {
		nih_dbus_error_raise (DBUS_ERROR_DISCONNECTED,
		                      "Connection is closed");
		nih_free (pending_data);
		return NULL;
	}

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_last_search_get_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_last_search_get_notify (DBusPendingCall *   pending_call,
                                                 NihDBusPendingData *pending_data)
{
	DBusMessage *     reply;
	DBusMessageIter   iter;
	DBusMessageIter   variter;
	NihDBusMessage *  message;
	DBusError         error;
	DBusMessageIter   value_iter;
	const char *      value_item0_dbus;
	char *            value_item0;
	uint32_t          value_item1;
	MyTestLastSearch *value;

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
		 * over and recurse into the arguments.
		 */
		message = nih_dbus_message_new (pending_data, pending_data->connection, reply);
		if (! message)
			goto enomem;

		dbus_message_iter_init (message->message, &iter);

		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_recurse (&iter, &variter);

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

		dbus_message_iter_recurse (&variter, &value_iter);

		value = nih_new (message, MyTestLastSearch);
		if (! value) {
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_STRING) {
			nih_free (value);
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&value_iter, &value_item0_dbus);

		value_item0 = nih_strdup (value, value_item0_dbus);
		if (! value_item0) {
			nih_free (value);
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&value_iter);

		value->item0 = value_item0;

		/* Demarshal a uint32_t from the message */
		if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_UINT32) {
			nih_free (value);
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&value_iter, &value_item1);

		dbus_message_iter_next (&value_iter);

		value->item1 = value_item1;

		if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {
			nih_free (value);
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
	nih_error_push_context ();
	((MyTestGetLastSearchReply)pending_data->handler) (pending_data->data, message, value);
	nih_error_pop_context ();

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_get_last_search_sync (const void *       parent,
                              NihDBusProxy *     proxy,
                              MyTestLastSearch **value)
{
	DBusMessage *     method_call;
	DBusMessageIter   iter;
	DBusMessageIter   variter;
	DBusError         error;
	DBusMessage *     reply;
	const char *      interface;
	const char *      property;
	DBusMessageIter   local_iter;
	const char *      local_item0_dbus;
	char *            local_item0;
	uint32_t          local_item1;
	MyTestLastSearch *local;

	nih_assert (proxy != NULL);
	nih_assert (value != NULL);

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "Get");
	if (! method_call)
		nih_return_no_memory_error (-1);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	property = "last_search";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
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

	/* Iterate the method arguments, recursing into the variant */
	dbus_message_iter_init (reply, &iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	dbus_message_iter_recurse (&iter, &variter);

	dbus_message_iter_next (&iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	do {
		__label__ enomem;

		/* Demarshal a structure from the message */
		if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRUCT) {
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_recurse (&variter, &local_iter);

		local = nih_new (parent, MyTestLastSearch);
		if (! local) {
			*value = NULL;
			goto enomem;
		}

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&local_iter) != DBUS_TYPE_STRING) {
			nih_free (local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&local_iter, &local_item0_dbus);

		local_item0 = nih_strdup (local, local_item0_dbus);
		if (! local_item0) {
			nih_free (local);
			*value = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&local_iter);

		local->item0 = local_item0;

		/* Demarshal a uint32_t from the message */
		if (dbus_message_iter_get_arg_type (&local_iter) != DBUS_TYPE_UINT32) {
			nih_free (local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&local_iter, &local_item1);

		dbus_message_iter_next (&local_iter);

		local->item1 = local_item1;

		if (dbus_message_iter_get_arg_type (&local_iter) != DBUS_TYPE_INVALID) {
			nih_free (local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_next (&variter);

		*value = local;
	enomem: __attribute__ ((unused));
	} while (! *value);

	dbus_message_unref (reply);

	return 0;
}


DBusPendingCall *
my_test_set_annotation (NihDBusProxy *           proxy,
                        const MyTestAnnotation * value,
                        MyTestSetAnnotationReply handler,
                        NihDBusErrorHandler      error_handler,
                        void *                   data,
                        int                      timeout)
{
	DBusMessage *       method_call;
	DBusMessageIter     iter;
	DBusMessageIter     variter;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data;
	const char *        interface;
	const char *        property;
	DBusMessageIter     value_iter;
	const char *        value_item0;
	const char *        value_item1;

	nih_assert (proxy != NULL);
	nih_assert (value != NULL);
	nih_assert ((handler == NULL) || (error_handler != NULL));

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "Set");
	if (! method_call)
		nih_return_no_memory_error (NULL);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	property = "annotation";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, "(ss)", &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT, NULL, &value_iter)) {
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	value_item0 = value->item0;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_item0)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	value_item1 = value->item1;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_item1)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	if (! dbus_message_iter_close_container (&variter, &value_iter)) {
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	if (! dbus_message_iter_close_container (&iter, &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Handle a fire-and-forget message */
	if (! error_handler) {
		dbus_message_set_no_reply (method_call, TRUE);
		if (! dbus_connection_send (proxy->connection, method_call, NULL)) {
			dbus_message_unref (method_call);
			nih_return_no_memory_error (NULL);
		}

		dbus_message_unref (method_call);
		return (DBusPendingCall *)TRUE;
	}

	/* Send the message and set up the reply notification. */
	pending_data = nih_dbus_pending_data_new (NULL, proxy->connection,
	                                          (NihDBusReplyHandler)handler,
	                                          error_handler, data);
	if (! pending_data) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	pending_call = NULL;
	if (! dbus_connection_send_with_reply (proxy->connection, method_call,
	                                       &pending_call, timeout)) {
		dbus_message_unref (method_call);
		nih_free (pending_data);
		nih_return_no_memory_error (NULL);
	}

	dbus_message_unref (method_call);

	if (! pending_call) {
		nih_dbus_error_raise (DBUS_ERROR_DISCONNECTED,
		                      "Connection is closed");
		nih_free (pending_data);
		return NULL;
	}

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_annotation_set_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_annotation_set_notify (DBusPendingCall *   pending_call,
                                                NihDBusPendingData *pending_data)
{
	DBusMessage *   reply;
	DBusMessageIter iter;
	NihDBusMessage *message;
	DBusError       error;

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

	/* Create a message context for the reply, and check
	 * there are no arguments.
	 */
	message = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));
	dbus_message_iter_init (message->message, &iter);

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

	/* Call the handler function */
	if (pending_data->handler) {
		nih_error_push_context ();
		((MyTestSetAnnotationReply)pending_data->handler) (pending_data->data, message);
		nih_error_pop_context ();
	}

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_set_annotation_sync (const void *            parent,
                             NihDBusProxy *          proxy,
                             const MyTestAnnotation *value)
{
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter variter;
	DBusError       error;
	DBusMessage *   reply;
	const char *    interface;
	const char *    property;
	DBusMessageIter value_iter;
	const char *    value_item0;
	const char *    value_item1;

	nih_assert (proxy != NULL);
	nih_assert (value != NULL);

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "Set");
	if (! method_call)
		nih_return_no_memory_error (-1);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	property = "annotation";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, "(ss)", &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT, NULL, &value_iter)) {
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	value_item0 = value->item0;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_item0)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	value_item1 = value->item1;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_item1)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	if (! dbus_message_iter_close_container (&variter, &value_iter)) {
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	if (! dbus_message_iter_close_container (&iter, &variter)) {
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

	/* Check the reply has no arguments */
	dbus_message_unref (method_call);
	dbus_message_iter_init (reply, &iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	dbus_message_unref (reply);

	return 0;
}


DBusPendingCall *
my_test_get_preferences (NihDBusProxy *            proxy,
                         MyTestGetPreferencesReply handler,
                         NihDBusErrorHandler       error_handler,
                         void *                    data,
                         int                       timeout)
{
	DBusMessage *       method_call;
	DBusMessageIter     iter;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data;
	const char *        interface;
	const char *        property;

	nih_assert (proxy != NULL);
	nih_assert ((handler != NULL) && (error_handler != NULL));

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "Get");
	if (! method_call)
		nih_return_no_memory_error (NULL);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	property = "preferences";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Send the message and set up the reply notification. */
	pending_data = nih_dbus_pending_data_new (NULL, proxy->connection,
	                                          (NihDBusReplyHandler)handler,
	                                          error_handler, data);
	if (! pending_data) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	pending_call = NULL;
	if (! dbus_connection_send_with_reply (proxy->connection, method_call,
	                                       &pending_call, timeout)) {
		dbus_message_unref (method_call);
		nih_free (pending_data);
		nih_return_no_memory_error (NULL);
	}

	dbus_message_unref (method_call);

	if (! pending_call) {
		nih_dbus_error_raise (DBUS_ERROR_DISCONNECTED,
		                      "Connection is closed");
		nih_free (pending_data);
		return NULL;
	}

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_preferences_get_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_preferences_get_notify (DBusPendingCall *   pending_call,
                                                 NihDBusPendingData *pending_data)
{
	DBusMessage *      reply;
	DBusMessageIter    iter;
	DBusMessageIter    variter;
	NihDBusMessage *   message;
	DBusError          error;
	DBusMessageIter    value_iter;
	uint32_t           value_item0;
	const char *       value_item1_dbus;
	char *             value_item1;
	MyTestPreferences *value;

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
		 * over and recurse into the arguments.
		 */
		message = nih_dbus_message_new (pending_data, pending_data->connection, reply);
		if (! message)
			goto enomem;

		dbus_message_iter_init (message->message, &iter);

		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_recurse (&iter, &variter);

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

		dbus_message_iter_recurse (&variter, &value_iter);

		value = nih_new (message, MyTestPreferences);
		if (! value) {
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		/* Demarshal a uint32_t from the message */
		if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_UINT32) {
			nih_free (value);
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&value_iter, &value_item0);

		dbus_message_iter_next (&value_iter);

		value->item0 = value_item0;

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_STRING) {
			nih_free (value);
			nih_error_push_context ();
			nih_error_raise (NIH_DBUS_INVALID_ARGS,
			                 _(NIH_DBUS_INVALID_ARGS_STR));
			pending_data->error_handler (pending_data->data, message);
			nih_error_pop_context ();

			nih_free (message);
			dbus_message_unref (reply);
			return;
		}

		dbus_message_iter_get_basic (&value_iter, &value_item1_dbus);

		value_item1 = nih_strdup (value, value_item1_dbus);
		if (! value_item1) {
			nih_free (value);
			nih_free (message);
			message = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&value_iter);

		value->item1 = value_item1;

		if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {
			nih_free (value);
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
	nih_error_push_context ();
	((MyTestGetPreferencesReply)pending_data->handler) (pending_data->data, message, value);
	nih_error_pop_context ();

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_get_preferences_sync (const void *        parent,
                              NihDBusProxy *      proxy,
                              MyTestPreferences **value)
{
	DBusMessage *      method_call;
	DBusMessageIter    iter;
	DBusMessageIter    variter;
	DBusError          error;
	DBusMessage *      reply;
	const char *       interface;
	const char *       property;
	DBusMessageIter    local_iter;
	uint32_t           local_item0;
	const char *       local_item1_dbus;
	char *             local_item1;
	MyTestPreferences *local;

	nih_assert (proxy != NULL);
	nih_assert (value != NULL);

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "Get");
	if (! method_call)
		nih_return_no_memory_error (-1);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	property = "preferences";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
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

	/* Iterate the method arguments, recursing into the variant */
	dbus_message_iter_init (reply, &iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	dbus_message_iter_recurse (&iter, &variter);

	dbus_message_iter_next (&iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	do {
		__label__ enomem;

		/* Demarshal a structure from the message */
		if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRUCT) {
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_recurse (&variter, &local_iter);

		local = nih_new (parent, MyTestPreferences);
		if (! local) {
			*value = NULL;
			goto enomem;
		}

		/* Demarshal a uint32_t from the message */
		if (dbus_message_iter_get_arg_type (&local_iter) != DBUS_TYPE_UINT32) {
			nih_free (local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&local_iter, &local_item0);

		dbus_message_iter_next (&local_iter);

		local->item0 = local_item0;

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&local_iter) != DBUS_TYPE_STRING) {
			nih_free (local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&local_iter, &local_item1_dbus);

		local_item1 = nih_strdup (local, local_item1_dbus);
		if (! local_item1) {
			nih_free (local);
			*value = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&local_iter);

		local->item1 = local_item1;

		if (dbus_message_iter_get_arg_type (&local_iter) != DBUS_TYPE_INVALID) {
			nih_free (local);
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_next (&variter);

		*value = local;
	enomem: __attribute__ ((unused));
	} while (! *value);

	dbus_message_unref (reply);

	return 0;
}

DBusPendingCall *
my_test_set_preferences (NihDBusProxy *            proxy,
                         const MyTestPreferences * value,
                         MyTestSetPreferencesReply handler,
                         NihDBusErrorHandler       error_handler,
                         void *                    data,
                         int                       timeout)
{
	DBusMessage *       method_call;
	DBusMessageIter     iter;
	DBusMessageIter     variter;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data;
	const char *        interface;
	const char *        property;
	DBusMessageIter     value_iter;
	uint32_t            value_item0;
	const char *        value_item1;

	nih_assert (proxy != NULL);
	nih_assert (value != NULL);
	nih_assert ((handler == NULL) || (error_handler != NULL));

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "Set");
	if (! method_call)
		nih_return_no_memory_error (NULL);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	property = "preferences";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, "(us)", &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT, NULL, &value_iter)) {
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	value_item0 = value->item0;

	/* Marshal a uint32_t onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_UINT32, &value_item0)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	value_item1 = value->item1;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_item1)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	if (! dbus_message_iter_close_container (&variter, &value_iter)) {
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	if (! dbus_message_iter_close_container (&iter, &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Handle a fire-and-forget message */
	if (! error_handler) {
		dbus_message_set_no_reply (method_call, TRUE);
		if (! dbus_connection_send (proxy->connection, method_call, NULL)) {
			dbus_message_unref (method_call);
			nih_return_no_memory_error (NULL);
		}

		dbus_message_unref (method_call);
		return (DBusPendingCall *)TRUE;
	}

	/* Send the message and set up the reply notification. */
	pending_data = nih_dbus_pending_data_new (NULL, proxy->connection,
	                                          (NihDBusReplyHandler)handler,
	                                          error_handler, data);
	if (! pending_data) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	pending_call = NULL;
	if (! dbus_connection_send_with_reply (proxy->connection, method_call,
	                                       &pending_call, timeout)) {
		dbus_message_unref (method_call);
		nih_free (pending_data);
		nih_return_no_memory_error (NULL);
	}

	dbus_message_unref (method_call);

	if (! pending_call) {
		nih_dbus_error_raise (DBUS_ERROR_DISCONNECTED,
		                      "Connection is closed");
		nih_free (pending_data);
		return NULL;
	}

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_preferences_set_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_preferences_set_notify (DBusPendingCall *   pending_call,
                                                 NihDBusPendingData *pending_data)
{
	DBusMessage *   reply;
	DBusMessageIter iter;
	NihDBusMessage *message;
	DBusError       error;

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

	/* Create a message context for the reply, and check
	 * there are no arguments.
	 */
	message = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));
	dbus_message_iter_init (message->message, &iter);

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

	/* Call the handler function */
	if (pending_data->handler) {
		nih_error_push_context ();
		((MyTestSetPreferencesReply)pending_data->handler) (pending_data->data, message);
		nih_error_pop_context ();
	}

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_set_preferences_sync (const void *             parent,
                              NihDBusProxy *           proxy,
                              const MyTestPreferences *value)
{
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter variter;
	DBusError       error;
	DBusMessage *   reply;
	const char *    interface;
	const char *    property;
	DBusMessageIter value_iter;
	uint32_t        value_item0;
	const char *    value_item1;

	nih_assert (proxy != NULL);
	nih_assert (value != NULL);

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "Set");
	if (! method_call)
		nih_return_no_memory_error (-1);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	property = "preferences";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, "(us)", &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT, NULL, &value_iter)) {
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	value_item0 = value->item0;

	/* Marshal a uint32_t onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_UINT32, &value_item0)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	value_item1 = value->item1;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_item1)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	if (! dbus_message_iter_close_container (&variter, &value_iter)) {
		dbus_message_iter_abandon_container (&iter, &variter);
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	if (! dbus_message_iter_close_container (&iter, &variter)) {
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

	/* Check the reply has no arguments */
	dbus_message_unref (method_call);
	dbus_message_iter_init (reply, &iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	dbus_message_unref (reply);

	return 0;
}


DBusPendingCall *
my_test_get_all (NihDBusProxy *      proxy,
                 MyTestGetAllReply   handler,
                 NihDBusErrorHandler error_handler,
                 void *              data,
                 int                 timeout)
{
	DBusMessage *       method_call;
	DBusMessageIter     iter;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data;
	const char *        interface;

	nih_assert (proxy != NULL);
	nih_assert ((handler != NULL) && (error_handler != NULL));

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "GetAll");
	if (! method_call)
		nih_return_no_memory_error (NULL);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Send the message and set up the reply notification. */
	pending_data = nih_dbus_pending_data_new (NULL, proxy->connection,
	                                          (NihDBusReplyHandler)handler,
	                                          error_handler, data);
	if (! pending_data) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	pending_call = NULL;
	if (! dbus_connection_send_with_reply (proxy->connection, method_call,
	                                       &pending_call, timeout)) {
		dbus_message_unref (method_call);
		nih_free (pending_data);
		nih_return_no_memory_error (NULL);
	}

	dbus_message_unref (method_call);

	if (! pending_call) {
		nih_dbus_error_raise (DBUS_ERROR_DISCONNECTED,
		                      "Connection is closed");
		nih_free (pending_data);
		return NULL;
	}

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_get_all_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_get_all_notify (DBusPendingCall *   pending_call,
                                         NihDBusPendingData *pending_data)
{
	DBusMessage *      reply;
	DBusMessageIter    iter;
	DBusMessageIter    arrayiter;
	DBusMessageIter    dictiter;
	DBusMessageIter    variter;
	NihDBusMessage *   message;
	DBusError          error;
	const char *       property;
	MyTestProperties * properties;
	size_t             property_count;
	MyTestLastSearch * last_search;
	DBusMessageIter    last_search_iter;
	const char *       last_search_item0_dbus;
	char *             last_search_item0;
	uint32_t           last_search_item1;
	MyTestPreferences *preferences;
	DBusMessageIter    preferences_iter;
	uint32_t           preferences_item0;
	const char *       preferences_item1_dbus;
	char *             preferences_item1;

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

	properties = NIH_MUST (nih_new (message, MyTestProperties));
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

		if (! strcmp (property, "last_search")) {
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

			dbus_message_iter_recurse (&variter, &last_search_iter);

			last_search = nih_new (properties, MyTestLastSearch);
			if (! last_search) {
				goto enomem;
			}

			/* Demarshal a char * from the message */
			if (dbus_message_iter_get_arg_type (&last_search_iter) != DBUS_TYPE_STRING) {
				nih_free (last_search);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&last_search_iter, &last_search_item0_dbus);

			last_search_item0 = nih_strdup (last_search, last_search_item0_dbus);
			if (! last_search_item0) {
				nih_free (last_search);
				goto enomem;
			}

			dbus_message_iter_next (&last_search_iter);

			last_search->item0 = last_search_item0;

			/* Demarshal a uint32_t from the message */
			if (dbus_message_iter_get_arg_type (&last_search_iter) != DBUS_TYPE_UINT32) {
				nih_free (last_search);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&last_search_iter, &last_search_item1);

			dbus_message_iter_next (&last_search_iter);

			last_search->item1 = last_search_item1;

			if (dbus_message_iter_get_arg_type (&last_search_iter) != DBUS_TYPE_INVALID) {
				nih_free (last_search);
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

			properties->last_search = last_search;

			nih_assert (++property_count);
		}

		if (! strcmp (property, "preferences")) {
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

			dbus_message_iter_recurse (&variter, &preferences_iter);

			preferences = nih_new (properties, MyTestPreferences);
			if (! preferences) {
				goto enomem;
			}

			/* Demarshal a uint32_t from the message */
			if (dbus_message_iter_get_arg_type (&preferences_iter) != DBUS_TYPE_UINT32) {
				nih_free (preferences);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&preferences_iter, &preferences_item0);

			dbus_message_iter_next (&preferences_iter);

			preferences->item0 = preferences_item0;

			/* Demarshal a char * from the message */
			if (dbus_message_iter_get_arg_type (&preferences_iter) != DBUS_TYPE_STRING) {
				nih_free (preferences);
				nih_error_push_context ();
				nih_error_raise (NIH_DBUS_INVALID_ARGS,
				                 _(NIH_DBUS_INVALID_ARGS_STR));
				pending_data->error_handler (pending_data->data, message);
				nih_error_pop_context ();

				nih_free (message);
				dbus_message_unref (reply);
				return;
			}

			dbus_message_iter_get_basic (&preferences_iter, &preferences_item1_dbus);

			preferences_item1 = nih_strdup (preferences, preferences_item1_dbus);
			if (! preferences_item1) {
				nih_free (preferences);
				goto enomem;
			}

			dbus_message_iter_next (&preferences_iter);

			preferences->item1 = preferences_item1;

			if (dbus_message_iter_get_arg_type (&preferences_iter) != DBUS_TYPE_INVALID) {
				nih_free (preferences);
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

			properties->preferences = preferences;

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

	if (property_count < 2) {
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
	((MyTestGetAllReply)pending_data->handler) (pending_data->data, message, properties);
	nih_error_pop_context ();

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_get_all_sync (const void *       parent,
                      NihDBusProxy *     proxy,
                      MyTestProperties **properties)
{
	DBusMessage *      method_call;
	DBusMessageIter    iter;
	DBusMessageIter    arrayiter;
	DBusMessageIter    dictiter;
	DBusMessageIter    variter;
	DBusError          error;
	DBusMessage *      reply;
	size_t             property_count;
	const char *       interface;
	const char *       property;
	MyTestLastSearch * last_search;
	DBusMessageIter    last_search_iter;
	const char *       last_search_item0_dbus;
	char *             last_search_item0;
	uint32_t           last_search_item1;
	MyTestPreferences *preferences;
	DBusMessageIter    preferences_iter;
	uint32_t           preferences_item0;
	const char *       preferences_item1_dbus;
	char *             preferences_item1;

	nih_assert (proxy != NULL);
	nih_assert (properties != NULL);

	/* Construct the method call message. */
	method_call = dbus_message_new_method_call (proxy->name, proxy->path, "org.freedesktop.DBus.Properties", "GetAll");
	if (! method_call)
		nih_return_no_memory_error (-1);

	dbus_message_set_auto_start (method_call, proxy->auto_start);

	dbus_message_iter_init_append (method_call, &iter);

	interface = "com.netsplit.Nih.Test";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {
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

	/* Iterate the method arguments, recursing into the array */
	dbus_message_iter_init (reply, &iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	*properties = NIH_MUST (nih_new (parent, MyTestProperties));
	property_count = 0;

	dbus_message_iter_recurse (&iter, &arrayiter);

	while (dbus_message_iter_get_arg_type (&arrayiter) != DBUS_TYPE_INVALID) {
		__label__ enomem;

		if (dbus_message_iter_get_arg_type (&arrayiter) != DBUS_TYPE_DICT_ENTRY) {
			nih_free (*properties);
			*properties = NULL;
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_recurse (&arrayiter, &dictiter);

		if (dbus_message_iter_get_arg_type (&dictiter) != DBUS_TYPE_STRING) {
			nih_free (*properties);
			*properties = NULL;
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&dictiter, &property);

		dbus_message_iter_next (&dictiter);

		if (dbus_message_iter_get_arg_type (&dictiter) != DBUS_TYPE_VARIANT) {
			nih_free (*properties);
			*properties = NULL;
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_recurse (&dictiter, &variter);

		if (! strcmp (property, "last_search")) {
			/* Demarshal a structure from the message */
			if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRUCT) {
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_recurse (&variter, &last_search_iter);

			last_search = nih_new (*properties, MyTestLastSearch);
			if (! last_search) {
				goto enomem;
			}

			/* Demarshal a char * from the message */
			if (dbus_message_iter_get_arg_type (&last_search_iter) != DBUS_TYPE_STRING) {
				nih_free (last_search);
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_get_basic (&last_search_iter, &last_search_item0_dbus);

			last_search_item0 = nih_strdup (last_search, last_search_item0_dbus);
			if (! last_search_item0) {
				nih_free (last_search);
				goto enomem;
			}

			dbus_message_iter_next (&last_search_iter);

			last_search->item0 = last_search_item0;

			/* Demarshal a uint32_t from the message */
			if (dbus_message_iter_get_arg_type (&last_search_iter) != DBUS_TYPE_UINT32) {
				nih_free (last_search);
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_get_basic (&last_search_iter, &last_search_item1);

			dbus_message_iter_next (&last_search_iter);

			last_search->item1 = last_search_item1;

			if (dbus_message_iter_get_arg_type (&last_search_iter) != DBUS_TYPE_INVALID) {
				nih_free (last_search);
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_next (&variter);

			(*properties)->last_search = last_search;

			nih_assert (++property_count);
		}

		if (! strcmp (property, "preferences")) {
			/* Demarshal a structure from the message */
			if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRUCT) {
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_recurse (&variter, &preferences_iter);

			preferences = nih_new (*properties, MyTestPreferences);
			if (! preferences) {
				goto enomem;
			}

			/* Demarshal a uint32_t from the message */
			if (dbus_message_iter_get_arg_type (&preferences_iter) != DBUS_TYPE_UINT32) {
				nih_free (preferences);
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_get_basic (&preferences_iter, &preferences_item0);

			dbus_message_iter_next (&preferences_iter);

			preferences->item0 = preferences_item0;

			/* Demarshal a char * from the message */
			if (dbus_message_iter_get_arg_type (&preferences_iter) != DBUS_TYPE_STRING) {
				nih_free (preferences);
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_get_basic (&preferences_iter, &preferences_item1_dbus);

			preferences_item1 = nih_strdup (preferences, preferences_item1_dbus);
			if (! preferences_item1) {
				nih_free (preferences);
				goto enomem;
			}

			dbus_message_iter_next (&preferences_iter);

			preferences->item1 = preferences_item1;

			if (dbus_message_iter_get_arg_type (&preferences_iter) != DBUS_TYPE_INVALID) {
				nih_free (preferences);
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_next (&variter);

			(*properties)->preferences = preferences;

			nih_assert (++property_count);
		}

		dbus_message_iter_next (&dictiter);

		if (dbus_message_iter_get_arg_type (&dictiter) != DBUS_TYPE_INVALID) {
			nih_free (*properties);
			*properties = NULL;
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_next (&arrayiter);
	enomem: __attribute__ ((unused));
	}

	dbus_message_iter_next (&iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		nih_free (*properties);
		*properties = NULL;
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	if (property_count < 2) {
		nih_free (*properties);
		*properties = NULL;
		dbus_message_unref (reply);
		nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
		                  _(NIH_DBUS_INVALID_ARGS_STR));
	}

	dbus_message_unref (reply);

	return 0;
}
