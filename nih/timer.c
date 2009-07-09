/* libnih
 *
 * timer.c - timeouts, periodic and scheduled timers
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <time.h>
#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "timer.h"


/**
 * nih_timers:
 *
 * This is the list of all registered timers, it is not sorted into any
 * particular order.  The due time of timers should be set when the timer
 * is added to this list, or rescheduled; it is not calculated on the fly.
 *
 * Each item is an NihTimer structure.
 **/
NihList *nih_timers = NULL;


/**
 * nih_timer_init:
 *
 * Initialise the timer list.
 **/
void
nih_timer_init (void)
{
	if (! nih_timers)
		nih_timers = NIH_MUST (nih_list_new (NULL));
}


/**
 * nih_timer_add_timeout:
 * @parent: parent object for new timer,
 * @timeout: seconds to wait before triggering,
 * @callback: function to be called,
 * @data: pointer to pass to function as first argument.
 *
 * Arranges for the @callback function to be called in @timeout seconds
 * time, or the soonest period thereafter.  A timer may be called
 * immediately by passing zero or a non-negative number as @timeout.
 *
 * The timer structure is allocated using nih_alloc() and stored in
 * a linked list; there is no non-allocated version of this function
 * because of this and because it will be automatically freed once called.
 *
 * Cancellation of the timer can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned timer.  When all parents
 * of the returned timer are freed, the returned timer will also be
 * freed.
 *
 * Returns: the new timer information, or NULL if insufficient memory.
 **/
NihTimer *
nih_timer_add_timeout (const void *parent,
		       time_t      timeout,
		       NihTimerCb  callback,
		       void       *data)
{
	NihTimer *      timer;
	struct timespec now;

	nih_assert (callback != NULL);

	nih_timer_init ();

	timer = nih_new (parent, NihTimer);
	if (! timer)
		return NULL;

	nih_list_init (&timer->entry);

	nih_alloc_set_destructor (timer, nih_list_destroy);

	timer->type = NIH_TIMER_TIMEOUT;
	timer->timeout = timeout;

	timer->callback = callback;
	timer->data = data;

	nih_assert (clock_gettime (CLOCK_MONOTONIC, &now) == 0);
	timer->due = now.tv_sec + timeout;

	nih_list_add (nih_timers, &timer->entry);

	return timer;
}

/**
 * nih_timer_add_periodic:
 * @parent: parent object for new timer,
 * @period: number of seconds between calls,
 * @callback: function to be called,
 * @data: pointer to pass to function as first argument.
 *
 * Arranges for the @callback function to be called every @period seconds,
 * or the soonest time thereafter.
 *
 * The timer structure is allocated using nih_alloc() and stored in
 * a linked list; there is no non-allocated version of this function
 * because of this.
 *
 * Cancellation of the timer can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned timer.  When all parents
 * of the returned timer are freed, the returned timer will also be
 * freed.
 *
 * Returns: the new timer information, or NULL if insufficient memory.
 **/
NihTimer *
nih_timer_add_periodic (const void *parent,
			time_t      period,
			NihTimerCb  callback,
			void       *data)
{
	NihTimer *      timer;
	struct timespec now;

	nih_assert (callback != NULL);
	nih_assert (period > 0);

	nih_timer_init ();

	timer = nih_new (parent, NihTimer);
	if (! timer)
		return NULL;

	nih_list_init (&timer->entry);

	nih_alloc_set_destructor (timer, nih_list_destroy);

	timer->type = NIH_TIMER_PERIODIC;
	timer->period = period;

	timer->callback = callback;
	timer->data = data;

	nih_assert (clock_gettime (CLOCK_MONOTONIC, &now) == 0);
	timer->due = now.tv_sec + period;

	nih_list_add (nih_timers, &timer->entry);

	return timer;
}

/**
 * nih_timer_add_scheduled:
 * @parent: parent object for new timer,
 * @schedule: trigger schedule,
 * @callback: function to be called,
 * @data: pointer to pass to function as first argument.
 *
 * Arranges for the @callback function to be called based on the @schedule
 * given.
 *
 * The timer structure is allocated using nih_alloc() and stored in
 * a linked list; there is no non-allocated version of this function
 * because of this.
 *
 * Cancellation of the timer can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned timer.  When all parents
 * of the returned timer are freed, the returned timer will also be
 * freed.
 *
 * Returns: the new timer information, or NULL if insufficient memory.
 **/
NihTimer *
nih_timer_add_scheduled (const void       *parent,
			 NihTimerSchedule *schedule,
			 NihTimerCb        callback,
			 void             *data)
{
	NihTimer *timer;

	nih_assert (callback != NULL);
	nih_assert (schedule != NULL);

	nih_timer_init ();

	timer = nih_new (parent, NihTimer);
	if (! timer)
		return NULL;

	nih_list_init (&timer->entry);

	nih_alloc_set_destructor (timer, nih_list_destroy);

	timer->type = NIH_TIMER_SCHEDULED;
	memcpy (&timer->schedule, schedule, sizeof (NihTimerSchedule));

	timer->callback = callback;
	timer->data = data;

	/* FIXME Not implemented */
	timer->due = 0;

	nih_list_add (nih_timers, &timer->entry);

	return timer;
}


/**
 * nih_timer_next_due:
 *
 * Iterates the complete list of timers looking for the one with the
 * lowest due time, so that the timer returned is either due to be triggered
 * now or in some period's time.
 *
 * Normally used to determine how long we can sleep for by subtracting the
 * current time from the due time of the next timer.
 *
 * Returns: next timer due, or NULL if there are no timers.
 **/
NihTimer *
nih_timer_next_due (void)
{
	NihTimer *next;

	nih_timer_init ();

	next = NULL;
	NIH_LIST_FOREACH (nih_timers, iter) {
		NihTimer *timer = (NihTimer *)iter;

		if ((next == NULL) || (timer->due < next->due))
			next = timer;
	}

	return next;
}


/**
 * nih_timer_poll:
 *
 * Iterates the complete list of timers and triggers any for which the
 * due time is less than or equal to the current time by calling their
 * callback functions.
 *
 * Arranges for the timer to be rescheuled, unless it is a timeout in which
 * case it is removed from the timer list.
 **/
void
nih_timer_poll (void)
{
	struct timespec now;

	nih_timer_init ();

	nih_assert (clock_gettime (CLOCK_MONOTONIC, &now) == 0);

	NIH_LIST_FOREACH_SAFE (nih_timers, iter) {
		NihTimer *timer = (NihTimer *)iter;
		int       free_when_done = FALSE;

		if (timer->due > now.tv_sec)
			continue;

		switch (timer->type) {
		case NIH_TIMER_TIMEOUT:
			nih_ref (timer, nih_timers);
			free_when_done = TRUE;
			break;
		case NIH_TIMER_PERIODIC:
			timer->due = now.tv_sec + timer->period;
			break;
		case NIH_TIMER_SCHEDULED:
			/* FIXME Not implemented */
			timer->due = 0;
			break;
		}

		nih_error_push_context ();
		timer->callback (timer->data, timer);
		nih_error_pop_context ();

		if (free_when_done)
			nih_free (timer);
	}
}
