/* test
 *
 * Copyright (C) 2009 Joe Bloggs.
 *
 * This file was automatically generated; see the source for copying
 * conditions.
 */

#ifndef TEST_TEST_H
#define TEST_TEST_H

#include <dbus/dbus.h>

#include <stdint.h>

#include <nih/macros.h>

#include <nih-dbus/dbus_interface.h>
#include <nih-dbus/dbus_message.h>


typedef struct my_foo_preferences {
	uint32_t item0;
	char *   item1;
} MyFooPreferences;


NIH_BEGIN_EXTERN

extern const NihDBusInterface  my_com_netsplit_Nih_Test;
extern const NihDBusInterface  my_com_netsplit_Nih_Foo;
extern const NihDBusInterface *my_interfaces[];


int my_test_peek_reply     (NihDBusMessage *message, const char *value)
	__attribute__ ((warn_unused_result));
int my_test_emit_bounce    (DBusConnection *connection, const char *origin_path, uint32_t height, int32_t velocity)
	__attribute__ ((warn_unused_result));
int my_test_emit_exploded  (DBusConnection *connection, const char *origin_path)
	__attribute__ ((warn_unused_result));
int my_foo_emit_new_result (DBusConnection *connection, const char *origin_path)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* TEST_TEST_H */
