static const NihDBusArg my_com_netsplit_Nih_Test_Poke_method_args[] = {
	{ "address", "u", NIH_DBUS_ARG_IN  },
	{ "value",   "s", NIH_DBUS_ARG_IN  },
	{ NULL }
};

static const NihDBusArg my_com_netsplit_Nih_Test_Peek_method_args[] = {
	{ "address", "u", NIH_DBUS_ARG_IN  },
	{ "value",   "s", NIH_DBUS_ARG_OUT },
	{ NULL }
};

static const NihDBusArg my_com_netsplit_Nih_Test_IsValidAddress_method_args[] = {
	{ "address", "u", NIH_DBUS_ARG_IN  },
	{ NULL }
};

static const NihDBusMethod my_com_netsplit_Nih_Test_methods[] = {
	{ "Poke",           my_com_netsplit_Nih_Test_Poke_method_args,           my_com_netsplit_Nih_Test_Poke_method           },
	{ "Peek",           my_com_netsplit_Nih_Test_Peek_method_args,           my_com_netsplit_Nih_Test_Peek_method           },
	{ "IsValidAddress", my_com_netsplit_Nih_Test_IsValidAddress_method_args, my_com_netsplit_Nih_Test_IsValidAddress_method },
	{ NULL }
};

static const NihDBusArg my_com_netsplit_Nih_Test_Bounce_signal_args[] = {
	{ "height",   "u", NIH_DBUS_ARG_OUT },
	{ "velocity", "i", NIH_DBUS_ARG_OUT },
	{ NULL }
};

static const NihDBusArg my_com_netsplit_Nih_Test_Exploded_signal_args[] = {
	{ NULL }
};

static const NihDBusSignal my_com_netsplit_Nih_Test_signals[] = {
	{ "Bounce",   my_com_netsplit_Nih_Test_Bounce_signal_args,   NULL },
	{ "Exploded", my_com_netsplit_Nih_Test_Exploded_signal_args, NULL },
	{ NULL }
};

static const NihDBusProperty my_com_netsplit_Nih_Test_properties[] = {
	{ "colour", "s", NIH_DBUS_READWRITE, my_com_netsplit_Nih_Test_colour_get, my_com_netsplit_Nih_Test_colour_set },
	{ "size",   "u", NIH_DBUS_READ,      my_com_netsplit_Nih_Test_size_get,   NULL                                },
	{ "touch",  "b", NIH_DBUS_WRITE,     NULL,                                my_com_netsplit_Nih_Test_touch_set  },
	{ NULL }
};

const NihDBusInterface my_com_netsplit_Nih_Test = {
	"com.netsplit.Nih.Test",
	my_com_netsplit_Nih_Test_methods,
	my_com_netsplit_Nih_Test_signals,
	my_com_netsplit_Nih_Test_properties
};

static const NihDBusMethod my_com_netsplit_Nih_Foo_methods[] = {
	{ NULL }
};

static const NihDBusSignal my_com_netsplit_Nih_Foo_signals[] = {
	{ NULL }
};

static const NihDBusProperty my_com_netsplit_Nih_Foo_properties[] = {
	{ NULL }
};

const NihDBusInterface my_com_netsplit_Nih_Foo = {
	"com.netsplit.Nih.Foo",
	my_com_netsplit_Nih_Foo_methods,
	my_com_netsplit_Nih_Foo_signals,
	my_com_netsplit_Nih_Foo_properties
};

const NihDBusInterface *my_interfaces[] = {
	&my_com_netsplit_Nih_Test,
	&my_com_netsplit_Nih_Foo,
	NULL
};
