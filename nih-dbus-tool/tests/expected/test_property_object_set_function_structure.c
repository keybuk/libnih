int
my_com_netsplit_Nih_Test_property_set (NihDBusObject *  object,
                                       NihDBusMessage * message,
                                       DBusMessageIter *iter)
{
	DBusMessageIter variter;
	DBusMessageIter value_iter;
	const char *    value_item0_dbus;
	char *          value_item0;
	uint32_t        value_item1;
	MyProperty *    value;

	nih_assert (object != NULL);
	nih_assert (message != NULL);
	nih_assert (iter != NULL);

	/* Recurse into the variant */
	if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to property property");
		return -1;
	}

	dbus_message_iter_recurse (iter, &variter);

	/* Demarshal a structure from the message */
	if (dbus_message_iter_get_arg_type (&variter) != DBUS_TYPE_STRUCT) {
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to property property");
		return -1;
	}

	dbus_message_iter_recurse (&variter, &value_iter);

	value = nih_new (message, MyProperty);
	if (! value) {
		nih_error_raise_no_memory ();
		return -1;
	}

	/* Demarshal a char * from the message */
	if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_STRING) {
		nih_free (value);
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to property property");
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

	/* Demarshal a uint32_t from the message */
	if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_UINT32) {
		nih_free (value);
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to property property");
		return -1;
	}

	dbus_message_iter_get_basic (&value_iter, &value_item1);

	dbus_message_iter_next (&value_iter);

	value->item1 = value_item1;

	if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {
		nih_free (value);
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to property property");
		return -1;
	}

	dbus_message_iter_next (&variter);

	dbus_message_iter_next (iter);

	if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {
		nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,
		                             "Invalid arguments to property property");
		return -1;
	}

	/* Call the handler function */
	if (my_set_property (object->data, message, value) < 0)
		return -1;

	return 0;
}
