/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef NIH_TIMER_H
#define NIH_TIMER_H

#include <nih/macros.h>
#include <nih/list.h>

#include <time.h>


/**
 * NihTimerCb:
 * @data: pointer given with callback,
 * @timer: timer that triggered the call.
 *
 * The timer callback is called whenever the timer has been triggered.
 * For periodic and scheduled timers, the timer may be removed by calling
 * nih_list_remove() or similar; this happens automatically for timeouts.
 **/
typedef struct nih_timer NihTimer;
typedef void (*NihTimerCb) (void *data, NihTimer *timer);


/**
 * NihTimerType:
 *
 * Used to identify the different types of timers that can be registered;
 * note that scheduled timers are not yet implemented.
 **/
typedef enum {
	NIH_TIMER_TIMEOUT,
	NIH_TIMER_PERIODIC,
	NIH_TIMER_SCHEDULED
} NihTimerType;

/**
 * NihTimerSchedule:
 * @minutes: minutes past the hour (0-59),
 * @hours: hours (0-23),
 * @mdays: days of month (1-31),
 * @months: months (1-12),
 * @wdays: days of week (0-7).
 *
 * Indidcates when scheduled timers should be run, each member is a bit
 * field where the bit is 1 if the timer should be run for that value and
 * 0 if not.
 **/
typedef struct nih_timer_schedule {
	uint64_t minutes;
	uint32_t hours;
	uint32_t mdays;
	uint16_t months;
	uint8_t  wdays;
} NihTimerSchedule;

/**
 * NihTimer:
 * @entry: list header,
 * @due: time next due,
 * @type: type of timer,
 * @timeout: seconds after registration timer should be triggered (timeout),
 * @period: seconds between triggerings of timer (periodic),
 * @schedule: detail of when to call the timer (scheduled),
 * @callback: function called when timer triggered,
 * @data: pointer passed to callback.
 *
 * Timers may be used whenever a function needs to be called later in
 * the process.  They are divided into three types, identified by @type.
 *
 * Timeouts are called once, @timeout seconds after they were registered.
 * Periodic timers are called every @period seconds after they were registered.
 * Scheduled timers are called based on the information in @schedule.
 *
 * In all cases, a timer may be cancelled by calling nih_list_remove() on
 * it as they are held in a list internally.
 **/
struct nih_timer {
	NihList       entry;
	time_t        due;

	NihTimerType  type;
	union {
		time_t           timeout;
		time_t           period;
		NihTimerSchedule schedule;
	};

	NihTimerCb    callback;
	void         *data;
};


NIH_BEGIN_EXTERN

extern NihList *nih_timers;


void      nih_timer_init          (void);

NihTimer *nih_timer_add_timeout   (const void *parent, time_t timeout,
				   NihTimerCb callback, void *data)
	__attribute__ ((warn_unused_result, malloc));
NihTimer *nih_timer_add_periodic  (const void *parent, time_t period,
				   NihTimerCb callback, void *data)
	__attribute__ ((warn_unused_result, malloc));
NihTimer *nih_timer_add_scheduled (const void *parent,
				   NihTimerSchedule *schedule,
				   NihTimerCb callback, void *data)
	__attribute__ ((warn_unused_result, malloc));

NihTimer *nih_timer_next_due       (void);
void      nih_timer_poll           (void);

NIH_END_EXTERN

#endif /* NIH_TIMER_H */
