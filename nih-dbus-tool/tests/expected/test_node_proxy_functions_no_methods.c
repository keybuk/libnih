static DBusHandlerResult
my_com_netsplit_Nih_Test_Bounce_signal (DBusConnection *    connection,
                                        DBusMessage *       signal,
                                        NihDBusProxySignal *proxied)
{
	DBusMessageIter iter;
	NihDBusMessage *message;
	uint32_t        height;
	int32_t         velocity;

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

	/* Demarshal a uint32_t from the message */
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_get_basic (&iter, &height);

	dbus_message_iter_next (&iter);

	/* Demarshal a int32_t from the message */
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_get_basic (&iter, &velocity);

	dbus_message_iter_next (&iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	/* Call the handler function */
	nih_error_push_context ();
	((MyTestBounceHandler)proxied->handler) (proxied->data, message, height, velocity);
	nih_error_pop_context ();
	nih_free (message);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static DBusHandlerResult
my_com_netsplit_Nih_Test_Exploded_signal (DBusConnection *    connection,
                                          DBusMessage *       signal,
                                          NihDBusProxySignal *proxied)
{
	DBusMessageIter iter;
	NihDBusMessage *message;

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

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	/* Call the handler function */
	nih_error_push_context ();
	((MyTestExplodedHandler)proxied->handler) (proxied->data, message);
	nih_error_pop_context ();
	nih_free (message);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


DBusPendingCall *
my_test_get_colour (NihDBusProxy *       proxy,
                    MyTestGetColourReply handler,
                    NihDBusErrorHandler  error_handler,
                    void *               data,
                    int                  timeout)
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

	property = "colour";
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

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_get_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_colour_get_notify (DBusPendingCall *   pending_call,
                                            NihDBusPendingData *pending_data)
{
	DBusMessage *   reply;
	DBusMessageIter iter;
	DBusMessageIter variter;
	NihDBusMessage *message;
	DBusError       error;
	const char *    value_dbus;
	char *          value;

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

		dbus_message_iter_get_basic (&variter, &value_dbus);

		value = nih_strdup (message, value_dbus);
		if (! value) {
			nih_free (message);
			message = NULL;
			goto enomem;
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
	((MyTestGetColourReply)pending_data->handler) (pending_data->data, message, value);
	nih_error_pop_context ();

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_get_colour_sync (const void *  parent,
                         NihDBusProxy *proxy,
                         char **       value)
{
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter variter;
	DBusError       error;
	DBusMessage *   reply;
	const char *    interface;
	const char *    property;
	const char *    local_dbus;
	char *          local;

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

	property = "colour";
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

		/* Demarshal a char * from the message */
		if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&variter, &local_dbus);

		local = nih_strdup (parent, local_dbus);
		if (! local) {
			*value = NULL;
			goto enomem;
		}

		dbus_message_iter_next (&variter);

		*value = local;
	enomem: __attribute__ ((unused));
	} while (! *value);

	dbus_message_unref (reply);

	return 0;
}

DBusPendingCall *
my_test_set_colour (NihDBusProxy *       proxy,
                    const char *         value,
                    MyTestSetColourReply handler,
                    NihDBusErrorHandler  error_handler,
                    void *               data,
                    int                  timeout)
{
	DBusMessage *       method_call;
	DBusMessageIter     iter;
	DBusMessageIter     variter;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data;
	const char *        interface;
	const char *        property;

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

	property = "colour";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, "s", &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {
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

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_colour_set_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_colour_set_notify (DBusPendingCall *   pending_call,
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
		((MyTestSetColourReply)pending_data->handler) (pending_data->data, message);
		nih_error_pop_context ();
	}

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_set_colour_sync (const void *  parent,
                         NihDBusProxy *proxy,
                         const char *  value)
{
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter variter;
	DBusError       error;
	DBusMessage *   reply;
	const char *    interface;
	const char *    property;

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

	property = "colour";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, "s", &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING, &value)) {
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
my_test_get_size (NihDBusProxy *      proxy,
                  MyTestGetSizeReply  handler,
                  NihDBusErrorHandler error_handler,
                  void *              data,
                  int                 timeout)
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

	property = "size";
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

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_size_get_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_size_get_notify (DBusPendingCall *   pending_call,
                                          NihDBusPendingData *pending_data)
{
	DBusMessage *   reply;
	DBusMessageIter iter;
	DBusMessageIter variter;
	NihDBusMessage *message;
	DBusError       error;
	uint32_t        value;

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

		dbus_message_iter_get_basic (&variter, &value);

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
	((MyTestGetSizeReply)pending_data->handler) (pending_data->data, message, value);
	nih_error_pop_context ();

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_get_size_sync (const void *  parent,
                       NihDBusProxy *proxy,
                       uint32_t *    value)
{
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter variter;
	DBusError       error;
	DBusMessage *   reply;
	const char *    interface;
	const char *    property;
	uint32_t        local;

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

	property = "size";
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

		/* Demarshal a uint32_t from the message */
		if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {
			dbus_message_unref (reply);
			nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
			                  _(NIH_DBUS_INVALID_ARGS_STR));
		}

		dbus_message_iter_get_basic (&variter, &local);

		dbus_message_iter_next (&variter);

		*value = local;
	enomem: __attribute__ ((unused));
	} while (! *value);

	dbus_message_unref (reply);

	return 0;
}


DBusPendingCall *
my_test_set_touch (NihDBusProxy *      proxy,
                   int                 value,
                   MyTestSetTouchReply handler,
                   NihDBusErrorHandler error_handler,
                   void *              data,
                   int                 timeout)
{
	DBusMessage *       method_call;
	DBusMessageIter     iter;
	DBusMessageIter     variter;
	DBusPendingCall *   pending_call;
	NihDBusPendingData *pending_data;
	const char *        interface;
	const char *        property;

	nih_assert (proxy != NULL);
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

	property = "touch";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, "b", &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (NULL);
	}

	/* Marshal a int onto the message */
	if (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {
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

	NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)my_com_netsplit_Nih_Test_touch_set_notify,
	                                        pending_data, (DBusFreeFunction)nih_discard));

	return pending_call;
}

static void
my_com_netsplit_Nih_Test_touch_set_notify (DBusPendingCall *   pending_call,
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
		((MyTestSetTouchReply)pending_data->handler) (pending_data->data, message);
		nih_error_pop_context ();
	}

	nih_free (message);
	dbus_message_unref (reply);
}

int
my_test_set_touch_sync (const void *  parent,
                        NihDBusProxy *proxy,
                        int           value)
{
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter variter;
	DBusError       error;
	DBusMessage *   reply;
	const char *    interface;
	const char *    property;

	nih_assert (proxy != NULL);

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

	property = "touch";
	if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, "b", &variter)) {
		dbus_message_unref (method_call);
		nih_return_no_memory_error (-1);
	}

	/* Marshal a int onto the message */
	if (! dbus_message_iter_append_basic (&variter, DBUS_TYPE_BOOLEAN, &value)) {
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
	DBusMessage *     reply;
	DBusMessageIter   iter;
	DBusMessageIter   arrayiter;
	DBusMessageIter   dictiter;
	DBusMessageIter   variter;
	NihDBusMessage *  message;
	DBusError         error;
	const char *      property;
	MyTestProperties *properties;
	size_t            property_count;
	char *            colour;
	const char *      colour_dbus;
	uint32_t          size;

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

		if (! strcmp (property, "colour")) {
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

			dbus_message_iter_get_basic (&variter, &colour_dbus);

			colour = nih_strdup (properties, colour_dbus);
			if (! colour) {
				goto enomem;
			}

			dbus_message_iter_next (&variter);

			properties->colour = colour;

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
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter arrayiter;
	DBusMessageIter dictiter;
	DBusMessageIter variter;
	DBusError       error;
	DBusMessage *   reply;
	size_t          property_count;
	const char *    interface;
	const char *    property;
	char *          colour;
	const char *    colour_dbus;
	uint32_t        size;

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

		if (! strcmp (property, "colour")) {
			/* Demarshal a char * from the message */
			if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRING) {
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_get_basic (&variter, &colour_dbus);

			colour = nih_strdup (*properties, colour_dbus);
			if (! colour) {
				goto enomem;
			}

			dbus_message_iter_next (&variter);

			(*properties)->colour = colour;

			nih_assert (++property_count);
		}

		if (! strcmp (property, "size")) {
			/* Demarshal a uint32_t from the message */
			if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_UINT32) {
				nih_free (*properties);
				*properties = NULL;
				dbus_message_unref (reply);
				nih_return_error (-1, NIH_DBUS_INVALID_ARGS,
				                  _(NIH_DBUS_INVALID_ARGS_STR));
			}

			dbus_message_iter_get_basic (&variter, &size);

			dbus_message_iter_next (&variter);

			(*properties)->size = size;

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


static DBusHandlerResult
my_com_netsplit_Nih_Foo_NewResult_signal (DBusConnection *    connection,
                                          DBusMessage *       signal,
                                          NihDBusProxySignal *proxied)
{
	DBusMessageIter iter;
	NihDBusMessage *message;

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

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		nih_free (message);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	/* Call the handler function */
	nih_error_push_context ();
	((MyFooNewResultHandler)proxied->handler) (proxied->data, message);
	nih_error_pop_context ();
	nih_free (message);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
