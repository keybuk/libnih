/* libnih
 *
 * timer.c - timeouts, periodic and scheduled timers
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
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

#include "timer.h"


/**
 * timers:
 *
 * This is the list of all registered timers, it is not sorted into any
 * particular order.  The due time of timers should be set when the timer
 * is added to this list, or rescheduled; it is not calculated on the fly.
 *
 * Each item is an NihTimer structure.
 **/
static NihList *timers = NULL;


/**
 * nih_timer_init:
 *
 * Initialise the timer list.
 **/
static inline void
nih_timer_init (void)
{
	if (! timers)
		NIH_MUST (timers = nih_list_new (NULL));
}


/**
 * nih_timer_add_timeout:
 * @parent: parent of timer,
 * @timeout: seconds to wait before triggering,
 * @callback: function to be called,
 * @data: pointer to pass to function as first argument.
 *
 * Arranges for the @callback function to be called in @timeout seconds
 * time, or the soonest period thereafter.  A timer may be called
 * immediately by passing zero or a non-negative number as @timeout.
 *
 * The timer structure is allocated using nih_alloc() and stored in a linked
 * list, a default destructor is set that removes the timer from the list.
 * Cancellation of the timer can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the new timer information, or NULL if insufficient memory.
 **/
NihTimer *
nih_timer_add_timeout (const void *parent,
		       time_t      timeout,
		       NihTimerCb  callback,
		       void       *data)
{
	NihTimer *timer;

	nih_assert (callback != NULL);

	nih_timer_init ();

	timer = nih_new (parent, NihTimer);
	if (! timer)
		return NULL;

	nih_list_init (&timer->entry);
	nih_alloc_set_destructor (timer, (NihDestructor)nih_list_destructor);

	timer->type = NIH_TIMER_TIMEOUT;
	timer->timeout = timeout;

	timer->callback = callback;
	timer->data = data;

	timer->due = time (NULL) + timeout;

	nih_list_add (timers, &timer->entry);

	return timer;
}

/**
 * nih_timer_add_periodic:
 * @parent: parent of timer,
 * @period: number of seconds between calls,
 * @callback: function to be called,
 * @data: pointer to pass to function as first argument.
 *
 * Arranges for the @callback function to be called every @period seconds,
 * or the soonest time thereafter.
 *
 * The timer structure is allocated using nih_alloc() and stored in a linked
 * list, a default destructor is set that removes the timer from the list.
 * Cancellation of the timer can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the new timer information, or NULL if insufficient memory.
 **/
NihTimer *
nih_timer_add_periodic (const void *parent,
			time_t      period,
			NihTimerCb  callback,
			void       *data)
{
	NihTimer *timer;

	nih_assert (callback != NULL);
	nih_assert (period > 0);

	nih_timer_init ();

	timer = nih_new (parent, NihTimer);
	if (! timer)
		return NULL;

	nih_list_init (&timer->entry);
	nih_alloc_set_destructor (timer, (NihDestructor)nih_list_destructor);

	timer->type = NIH_TIMER_PERIODIC;
	timer->period = period;

	timer->callback = callback;
	timer->data = data;

	timer->due = time (NULL) + period;

	nih_list_add (timers, &timer->entry);

	return timer;
}

/**
 * nih_timer_add_scheduled:
 * @parent: parent of timer,
 * @schedule: trigger schedule,
 * @callback: function to be called,
 * @data: pointer to pass to function as first argument.
 *
 * Arranges for the @callback function to be called based on the @schedule
 * given.
 *
 * The timer structure is allocated using nih_alloc() and stored in a linked
 * list, a default destructor is set that removes the timer from the list.
 * Cancellation of the timer can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
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
	nih_alloc_set_destructor (timer, (NihDestructor)nih_list_destructor);

	timer->type = NIH_TIMER_SCHEDULED;
	memcpy (&timer->schedule, schedule, sizeof (NihTimerSchedule));

	timer->callback = callback;
	timer->data = data;

	/* FIXME Not implemented */
	timer->due = 0;

	nih_list_add (timers, &timer->entry);

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
	NIH_LIST_FOREACH (timers, iter) {
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
	time_t now;

	nih_timer_init ();

	now = time (NULL);
	NIH_LIST_FOREACH_SAFE (timers, iter) {
		NihTimer *timer = (NihTimer *)iter;

		if (timer->due > now)
			continue;

		timer->callback (timer->data, timer);

		switch (timer->type) {
		case NIH_TIMER_TIMEOUT:
			nih_list_free (&timer->entry);
			break;
		case NIH_TIMER_PERIODIC:
			timer->due = now + timer->period;
			break;
		case NIH_TIMER_SCHEDULED:
			/* FIXME Not implemented */
			timer->due = 0;
			break;
		}
	}
}
