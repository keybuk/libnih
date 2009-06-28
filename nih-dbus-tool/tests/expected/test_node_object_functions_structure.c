static DBusHandlerResult
my_com_netsplit_Nih_Test_Search_method (NihDBusObject * object,
                                        NihDBusMessage *message)
{
	DBusMessageIter   iter;
	DBusMessage *     reply;
	MyTestSearchItem *item;
	DBusMessageIter   item_iter;
	const char *      item_item0_dbus;
	char *            item_item0;
	uint32_t          item_item1;

	nih_assert (object != NULL);
	nih_assert (message != NULL);

	/* Iterate the arguments to the message and demarshal into arguments
	 * for our own function call.
	 */
	dbus_message_iter_init (message->message, &iter);

	/* Demarshal a structure from the message */
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRUCT) {
		reply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,
		                                "Invalid arguments to Search method");
		if (! reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		if (! dbus_connection_send (message->connection, reply, NULL)) {
			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_recurse (&iter, &item_iter);

	item = nih_new (message, MyTestSearchItem);
	if (! item) {
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	/* Demarshal a char * from the message */
	if (dbus_message_iter_get_arg_type (&item_iter) != DBUS_TYPE_STRING) {
		nih_free (item);
		reply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,
		                                "Invalid arguments to Search method");
		if (! reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		if (! dbus_connection_send (message->connection, reply, NULL)) {
			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_get_basic (&item_iter, &item_item0_dbus);

	item_item0 = nih_strdup (item, item_item0_dbus);
	if (! item_item0) {
		nih_free (item);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_iter_next (&item_iter);

	item->item0 = item_item0;

	/* Demarshal a uint32_t from the message */
	if (dbus_message_iter_get_arg_type (&item_iter) != DBUS_TYPE_UINT32) {
		nih_free (item);
		reply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,
		                                "Invalid arguments to Search method");
		if (! reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		if (! dbus_connection_send (message->connection, reply, NULL)) {
			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_get_basic (&item_iter, &item_item1);

	dbus_message_iter_next (&item_iter);

	item->item1 = item_item1;

	if (dbus_message_iter_get_arg_type (&item_iter) != DBUS_TYPE_INVALID) {
		nih_free (item);
		reply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,
		                                "Invalid arguments to Search method");
		if (! reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		if (! dbus_connection_send (message->connection, reply, NULL)) {
			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_next (&iter);

	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {
		reply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,
		                                "Invalid arguments to Search method");
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
	if (my_test_search (object->data, message, item) < 0) {
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

	return DBUS_HANDLER_RESULT_HANDLED;
}

int
my_test_search_reply (NihDBusMessage *          message,
                      const MyTestSearchResult *result)
{
	DBusMessage *   reply;
	DBusMessageIter iter;
	DBusMessageIter result_iter;
	const char *    result_item0;
	const char *    result_item1;

	nih_assert (message != NULL);
	nih_assert (result != NULL);

	/* If the sender doesn't care about a reply, don't bother wasting
	 * effort constructing and sending one.
	 */
	if (dbus_message_get_no_reply (message->message))
		return 0;

	/* Construct the reply message. */
	reply = dbus_message_new_method_return (message->message);
	if (! reply)
		return -1;

	dbus_message_iter_init_append (reply, &iter);

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT, NULL, &result_iter)) {
		dbus_message_unref (reply);
		return -1;
	}

	result_item0 = result->item0;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&result_iter, DBUS_TYPE_STRING, &result_item0)) {
		dbus_message_iter_abandon_container (&iter, &result_iter);
		dbus_message_unref (reply);
		return -1;
	}

	result_item1 = result->item1;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&result_iter, DBUS_TYPE_STRING, &result_item1)) {
		dbus_message_iter_abandon_container (&iter, &result_iter);
		dbus_message_unref (reply);
		return -1;
	}

	if (! dbus_message_iter_close_container (&iter, &result_iter)) {
		dbus_message_unref (reply);
		return -1;
	}

	/* Send the reply, appending it to the outgoing queue. */
	if (! dbus_connection_send (message->connection, reply, NULL)) {
		dbus_message_unref (reply);
		return -1;
	}

	dbus_message_unref (reply);

	return 0;
}


int
my_test_emit_new_search (DBusConnection *            connection,
                         const char *                origin_path,
                         const MyTestNewSearchQuery *query)
{
	DBusMessage *   signal;
	DBusMessageIter iter;
	DBusMessageIter query_iter;
	const char *    query_item0;
	const char *    query_item1;
	uint32_t        query_item2;

	nih_assert (connection != NULL);
	nih_assert (origin_path != NULL);
	nih_assert (query != NULL);

	/* Construct the message. */
	signal = dbus_message_new_signal (origin_path, "com.netsplit.Nih.Test", "NewSearch");
	if (! signal)
		return -1;

	dbus_message_iter_init_append (signal, &iter);

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT, NULL, &query_iter)) {
		dbus_message_unref (signal);
		return -1;
	}

	query_item0 = query->item0;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&query_iter, DBUS_TYPE_STRING, &query_item0)) {
		dbus_message_iter_abandon_container (&iter, &query_iter);
		dbus_message_unref (signal);
		return -1;
	}

	query_item1 = query->item1;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&query_iter, DBUS_TYPE_STRING, &query_item1)) {
		dbus_message_iter_abandon_container (&iter, &query_iter);
		dbus_message_unref (signal);
		return -1;
	}

	query_item2 = query->item2;

	/* Marshal a uint32_t onto the message */
	if (! dbus_message_iter_append_basic (&query_iter, DBUS_TYPE_UINT32, &query_item2)) {
		dbus_message_iter_abandon_container (&iter, &query_iter);
		dbus_message_unref (signal);
		return -1;
	}

	if (! dbus_message_iter_close_container (&iter, &query_iter)) {
		dbus_message_unref (signal);
		return -1;
	}

	/* Send the signal, appending it to the outgoing queue. */
	if (! dbus_connection_send (connection, signal, NULL)) {
		dbus_message_unref (signal);
		return -1;
	}

	dbus_message_unref (signal);

	return 0;
}


static int
my_com_netsplit_Nih_Test_last_search_get (NihDBusObject *  object,
                                          NihDBusMessage * message,
                                          DBusMessageIter *iter)
{
	DBusMessageIter   variter;
	DBusMessageIter   value_iter;
	const char *      value_item0;
	uint32_t          value_item1;
	MyTestLastSearch *value;

	nih_assert (object != NULL);
	nih_assert (message != NULL);
	nih_assert (iter != NULL);

	/* Call the handler function */
	if (my_test_get_last_search (object->data, message, &value) < 0)
		return -1;

	/* Append a variant onto the message to contain the property value. */
	if (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "(su)", &variter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT, NULL, &value_iter)) {
		dbus_message_iter_abandon_container (iter, &variter);
		nih_error_raise_no_memory ();
		return -1;
	}

	value_item0 = value->item0;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_item0)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (iter, &variter);
		nih_error_raise_no_memory ();
		return -1;
	}

	value_item1 = value->item1;

	/* Marshal a uint32_t onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_UINT32, &value_item1)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (iter, &variter);
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! dbus_message_iter_close_container (&variter, &value_iter)) {
		dbus_message_iter_abandon_container (iter, &variter);
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Finish the variant */
	if (! dbus_message_iter_close_container (iter, &variter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	return 0;
}


static int
my_com_netsplit_Nih_Test_annotation_set (NihDBusObject *  object,
                                         NihDBusMessage * message,
                                         DBusMessageIter *iter)
{
	DBusMessageIter   variter;
	DBusMessageIter   value_iter;
	const char *      value_item0_dbus;
	char *            value_item0;
	const char *      value_item1_dbus;
	char *            value_item1;
	MyTestAnnotation *value;

	nih_assert (object != NULL);
	nih_assert (message != NULL);
	nih_assert (iter != NULL);

	/* Recurse into the variant */
	if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to annotation property");
		return -1;
	}

	dbus_message_iter_recurse (iter, &variter);

	/* Demarshal a structure from the message */
	if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRUCT) {
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to annotation property");
		return -1;
	}

	dbus_message_iter_recurse (&variter, &value_iter);

	value = nih_new (message, MyTestAnnotation);
	if (! value) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Demarshal a char * from the message */
	if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_STRING) {
		nih_free (value);
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to annotation property");
		return -1;
	}

	dbus_message_iter_get_basic (&value_iter, &value_item0_dbus);

	value_item0 = nih_strdup (value, value_item0_dbus);
	if (! value_item0) {
		nih_free (value);
		nih_error_raise_no_memory ();
		return -1;
	}

	dbus_message_iter_next (&value_iter);

	value->item0 = value_item0;

	/* Demarshal a char * from the message */
	if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_STRING) {
		nih_free (value);
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to annotation property");
		return -1;
	}

	dbus_message_iter_get_basic (&value_iter, &value_item1_dbus);

	value_item1 = nih_strdup (value, value_item1_dbus);
	if (! value_item1) {
		nih_free (value);
		nih_error_raise_no_memory ();
		return -1;
	}

	dbus_message_iter_next (&value_iter);

	value->item1 = value_item1;

	if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {
		nih_free (value);
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to annotation property");
		return -1;
	}

	dbus_message_iter_next (&variter);

	dbus_message_iter_next (iter);

	if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to annotation property");
		return -1;
	}

	/* Call the handler function */
	if (my_test_set_annotation (object->data, message, value) < 0)
		return -1;

	return 0;
}


static int
my_com_netsplit_Nih_Test_preferences_get (NihDBusObject *  object,
                                          NihDBusMessage * message,
                                          DBusMessageIter *iter)
{
	DBusMessageIter    variter;
	DBusMessageIter    value_iter;
	uint32_t           value_item0;
	const char *       value_item1;
	MyTestPreferences *value;

	nih_assert (object != NULL);
	nih_assert (message != NULL);
	nih_assert (iter != NULL);

	/* Call the handler function */
	if (my_test_get_preferences (object->data, message, &value) < 0)
		return -1;

	/* Append a variant onto the message to contain the property value. */
	if (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "(us)", &variter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Marshal a structure onto the message */
	if (! dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT, NULL, &value_iter)) {
		dbus_message_iter_abandon_container (iter, &variter);
		nih_error_raise_no_memory ();
		return -1;
	}

	value_item0 = value->item0;

	/* Marshal a uint32_t onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_UINT32, &value_item0)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (iter, &variter);
		nih_error_raise_no_memory ();
		return -1;
	}

	value_item1 = value->item1;

	/* Marshal a char * onto the message */
	if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_item1)) {
		dbus_message_iter_abandon_container (&variter, &value_iter);
		dbus_message_iter_abandon_container (iter, &variter);
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! dbus_message_iter_close_container (&variter, &value_iter)) {
		dbus_message_iter_abandon_container (iter, &variter);
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Finish the variant */
	if (! dbus_message_iter_close_container (iter, &variter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	return 0;
}

static int
my_com_netsplit_Nih_Test_preferences_set (NihDBusObject *  object,
                                          NihDBusMessage * message,
                                          DBusMessageIter *iter)
{
	DBusMessageIter    variter;
	DBusMessageIter    value_iter;
	uint32_t           value_item0;
	const char *       value_item1_dbus;
	char *             value_item1;
	MyTestPreferences *value;

	nih_assert (object != NULL);
	nih_assert (message != NULL);
	nih_assert (iter != NULL);

	/* Recurse into the variant */
	if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to preferences property");
		return -1;
	}

	dbus_message_iter_recurse (iter, &variter);

	/* Demarshal a structure from the message */
	if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRUCT) {
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to preferences property");
		return -1;
	}

	dbus_message_iter_recurse (&variter, &value_iter);

	value = nih_new (message, MyTestPreferences);
	if (! value) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Demarshal a uint32_t from the message */
	if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_UINT32) {
		nih_free (value);
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to preferences property");
		return -1;
	}

	dbus_message_iter_get_basic (&value_iter, &value_item0);

	dbus_message_iter_next (&value_iter);

	value->item0 = value_item0;

	/* Demarshal a char * from the message */
	if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_STRING) {
		nih_free (value);
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to preferences property");
		return -1;
	}

	dbus_message_iter_get_basic (&value_iter, &value_item1_dbus);

	value_item1 = nih_strdup (value, value_item1_dbus);
	if (! value_item1) {
		nih_free (value);
		nih_error_raise_no_memory ();
		return -1;
	}

	dbus_message_iter_next (&value_iter);

	value->item1 = value_item1;

	if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {
		nih_free (value);
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to preferences property");
		return -1;
	}

	dbus_message_iter_next (&variter);

	dbus_message_iter_next (iter);

	if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to preferences property");
		return -1;
	}

	/* Call the handler function */
	if (my_test_set_preferences (object->data, message, value) < 0)
		return -1;

	return 0;
}
