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
#include <nih-dbus/dbus_pending_data.h>
#include <nih-dbus/dbus_proxy.h>


typedef struct my_test_properties {
	char *   colour;
	uint32_t size;
} MyTestProperties;

typedef struct my_foo_preferences {
	uint32_t item0;
	char *   item1;
} MyFooPreferences;

typedef struct my_foo_properties {
	MyFooPreferences *preferences;
} MyFooProperties;


typedef void (*MyTestPokeReply) (void *data, NihDBusMessage *message);

typedef void (*MyTestPeekReply) (void *data, NihDBusMessage *message, const char *value);

typedef void (*MyTestIsValidAddressReply) (void *data, NihDBusMessage *message);

typedef void (*MyTestBounceHandler) (void *data, NihDBusMessage *message, uint32_t height, int32_t velocity);

typedef void (*MyTestExplodedHandler) (void *data, NihDBusMessage *message);

typedef void (*MyTestGetColourReply) (void *data, NihDBusMessage *message, const char *value);

typedef void (*MyTestSetColourReply) (void *data, NihDBusMessage *message);

typedef void (*MyTestGetSizeReply) (void *data, NihDBusMessage *message, uint32_t value);

typedef void (*MyTestSetTouchReply) (void *data, NihDBusMessage *message);

typedef void (*MyTestGetAllReply) (void *data, NihDBusMessage *message, const MyTestProperties *properties);

typedef void (*MyFooBingReply) (void *data, NihDBusMessage *message);

typedef void (*MyFooNewResultHandler) (void *data, NihDBusMessage *message);

typedef void (*MyFooGetPreferencesReply) (void *data, NihDBusMessage *message, const MyFooPreferences *value);

typedef void (*MyFooSetPreferencesReply) (void *data, NihDBusMessage *message);

typedef void (*MyFooGetAllReply) (void *data, NihDBusMessage *message, const MyFooProperties *properties);


NIH_BEGIN_EXTERN

extern const NihDBusInterface  my_com_netsplit_Nih_Test;
extern const NihDBusInterface  my_com_netsplit_Nih_Foo;
extern const NihDBusInterface *my_interfaces[];


DBusPendingCall *my_test_poke                  (NihDBusProxy *proxy, uint32_t address, const char *value, MyTestPokeReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_test_poke_sync             (const void *parent, NihDBusProxy *proxy, uint32_t address, const char *value)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_test_peek                  (NihDBusProxy *proxy, uint32_t address, MyTestPeekReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_test_peek_sync             (const void *parent, NihDBusProxy *proxy, uint32_t address, char **value)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_test_is_valid_address      (NihDBusProxy *proxy, uint32_t address, MyTestIsValidAddressReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_test_is_valid_address_sync (const void *parent, NihDBusProxy *proxy, uint32_t address)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_test_get_colour            (NihDBusProxy *proxy, MyTestGetColourReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_test_get_colour_sync       (const void *parent, NihDBusProxy *proxy, char **value)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_test_set_colour            (NihDBusProxy *proxy, const char *value, MyTestSetColourReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_test_set_colour_sync       (const void *parent, NihDBusProxy *proxy, const char *value)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_test_get_size              (NihDBusProxy *proxy, MyTestGetSizeReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_test_get_size_sync         (const void *parent, NihDBusProxy *proxy, uint32_t *value)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_test_set_touch             (NihDBusProxy *proxy, int value, MyTestSetTouchReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_test_set_touch_sync        (const void *parent, NihDBusProxy *proxy, int value)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_test_get_all               (NihDBusProxy *proxy, MyTestGetAllReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_test_get_all_sync          (const void *parent, NihDBusProxy *proxy, MyTestProperties **properties)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_foo_bing                   (NihDBusProxy *proxy, MyFooBingReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_foo_bing_sync              (const void *parent, NihDBusProxy *proxy)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_foo_get_preferences        (NihDBusProxy *proxy, MyFooGetPreferencesReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_foo_get_preferences_sync   (const void *parent, NihDBusProxy *proxy, MyFooPreferences **value)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_foo_set_preferences        (NihDBusProxy *proxy, const MyFooPreferences *value, MyFooSetPreferencesReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_foo_set_preferences_sync   (const void *parent, NihDBusProxy *proxy, const MyFooPreferences *value)
	__attribute__ ((warn_unused_result));
DBusPendingCall *my_foo_get_all                (NihDBusProxy *proxy, MyFooGetAllReply handler, NihDBusErrorHandler error_handler, void *data, int timeout)
	__attribute__ ((warn_unused_result));
int              my_foo_get_all_sync           (const void *parent, NihDBusProxy *proxy, MyFooProperties **properties)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* TEST_TEST_H */
