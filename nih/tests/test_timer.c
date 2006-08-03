/* libnih
 *
 * test_timer.c - test suite for nih/timer.c
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

#include <config.h>


#include <time.h>
#include <stdio.h>
#include <string.h>

#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/timer.h>


static int callback_called = 0;
static void *last_data;
static NihTimer *last_timer;

static void
my_callback (void *data, NihTimer *timer)
{
	callback_called++;
	last_data = data;
	last_timer = timer;
}

int
test_add_timeout (void)
{
	NihTimer *timer, *ptr;
	time_t    t1, t2;
	int       ret = 0;

	printf ("Testing nih_timer_add_timeout()\n");
	t1 = time (NULL);
	timer = nih_timer_add_timeout (NULL, 10, my_callback, &t1);
	t2 = time (NULL);

	/* Timer should be a timeout one */
	if (timer->type != NIH_TIMER_TIMEOUT) {
		printf ("BAD: type set incorrectly.\n");
		ret = 1;
	}

	/* Due time should be in 10s time */
	if ((timer->due < t1 + 10) || (timer->due > t2 + 10)) {
		printf ("BAD: due time set incorrectly.\n");
		ret = 1;
	}

	/* Timeout should be set to that given */
	if (timer->timeout != 10) {
		printf ("BAD: timeout set incorrectly.\n");
		ret = 1;
	}

	/* Callback should be callback given */
	if (timer->callback != my_callback) {
		printf ("BAD: callback set incorrectly.\n");
		ret = 1;
	}

	/* Callback data should be pointer given */
	if (timer->data != &t1) {
		printf ("BAD: callback data set incorrectly.\n");
		ret = 1;
	}

	/* Should be in the timer list */
	if (NIH_LIST_EMPTY (&timer->entry)) {
		printf ("BAD: not placed into timers list.\n");
		ret = 1;
	}

	/* Should have been allocated using nih_alloc */
	if (nih_alloc_size (timer) != sizeof (NihTimer)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}


	/* Timer should be next due */
	ptr = nih_timer_next_due ();
	if (ptr != timer) {
		printf ("BAD: timer not next due.\n");
		ret = 1;
	}

	nih_list_free (&timer->entry);

	return ret;
}

int
test_add_periodic (void)
{
	NihTimer *timer, *ptr;
	time_t    t1, t2;
	int       ret = 0;

	printf ("Testing nih_timer_add_periodic()\n");
	t1 = time (NULL);
	timer = nih_timer_add_periodic (NULL, 25, my_callback, &t1);
	t2 = time (NULL);

	/* Timer should be a periodic one */
	if (timer->type != NIH_TIMER_PERIODIC) {
		printf ("BAD: type set incorrectly.\n");
		ret = 1;
	}

	/* Due time should be in 25s time */
	if ((timer->due < t1 + 25) || (timer->due > t2 + 25)) {
		printf ("BAD: due time set incorrectly.\n");
		ret = 1;
	}

	/* Period should be that given */
	if (timer->period != 25) {
		printf ("BAD: period set incorrectly.\n");
		ret = 1;
	}

	/* Callback should be callback given */
	if (timer->callback != my_callback) {
		printf ("BAD: callback set incorrectly.\n");
		ret = 1;
	}

	/* Callback data should be pointer given */
	if (timer->data != &t1) {
		printf ("BAD: callback data set incorrectly.\n");
		ret = 1;
	}

	/* Should be in the timer list */
	if (NIH_LIST_EMPTY (&timer->entry)) {
		printf ("BAD: not placed into timers list.\n");
		ret = 1;
	}

	/* Should have been allocated using nih_alloc */
	if (nih_alloc_size (timer) != sizeof (NihTimer)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}


	/* Timer should be next due */
	ptr = nih_timer_next_due ();
	if (ptr != timer) {
		printf ("BAD: timer not next due.\n");
		ret = 1;
	}

	nih_list_free (&timer->entry);

	return ret;
}

int
test_add_scheduled (void)
{
	NihTimer         *timer, *ptr;
	NihTimerSchedule  schedule;
	time_t            t1, t2;
	int               ret = 0;

	memset (&schedule, 0, sizeof (NihTimerSchedule));

	printf ("Testing nih_timer_add_scheduled()\n");
	t1 = time (NULL);
	timer = nih_timer_add_scheduled (NULL, &schedule, my_callback, &t1);
	t2 = time (NULL);

	/* Timer should be a scheduled one */
	if (timer->type != NIH_TIMER_SCHEDULED) {
		printf ("BAD: type set incorrectly.\n");
		ret = 1;
	}

	/* FIXME check due time when it's set to something sensible */

	/* Schedule should be a copy of the one we gave */
	if ((timer->schedule.minutes != schedule.minutes)
	    || (timer->schedule.hours != schedule.hours)
	    || (timer->schedule.mdays != schedule.mdays)
	    || (timer->schedule.months != schedule.months)
	    || (timer->schedule.wdays != schedule.wdays)) {
		printf ("BAD: schedule set incorrectly.\n");
		ret = 1;
	}

	/* Callback should be callback given */
	if (timer->callback != my_callback) {
		printf ("BAD: callback set incorrectly.\n");
		ret = 1;
	}

	/* Callback data should be pointer given */
	if (timer->data != &t1) {
		printf ("BAD: callback data set incorrectly.\n");
		ret = 1;
	}

	/* Should be in the timer list */
	if (NIH_LIST_EMPTY (&timer->entry)) {
		printf ("BAD: not placed into timers list.\n");
		ret = 1;
	}

	/* Should have been allocated using nih_alloc */
	if (nih_alloc_size (timer) != sizeof (NihTimer)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}


	/* Timer should be next due */
	ptr = nih_timer_next_due ();
	if (ptr != timer) {
		printf ("BAD: timer not next due.\n");
		ret = 1;
	}

	nih_list_free (&timer->entry);

	return ret;
}


int
test_next_due (void)
{
	NihTimer *timer1, *timer2, *timer3, *ptr;
	int       ret = 0;

	printf ("Testing nih_timer_next_due()\n");
	timer1 = nih_timer_add_timeout (NULL, 10, my_callback, &ret);
	timer2 = nih_timer_add_timeout (NULL, 5, my_callback, &ret);
	timer3 = nih_timer_add_timeout (NULL, 15, my_callback, &ret);

	/* First timer due should be the second timer */
	ptr = nih_timer_next_due ();
	if (ptr != timer2) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}
	nih_list_free (&ptr->entry);

	/* Second timer due should be the first timer */
	ptr = nih_timer_next_due ();
	if (ptr != timer1) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}
	nih_list_free (&ptr->entry);

	/* Third timer due should be the third timer */
	ptr = nih_timer_next_due ();
	if (ptr != timer3) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}
	nih_list_free (&ptr->entry);

	/* No more timers should be due */
	ptr = nih_timer_next_due ();
	if (ptr != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	return ret;
}

static int destroyed = 0;

static int
my_destructor (void *ptr)
{
	destroyed = 1;

	return 0;
}

int
test_poll (void)
{
	NihTimer *timer1, *timer2;
	time_t    t1, t2;
	int       ret = 0;

	printf ("Testing nih_timer_poll()\n");
	timer1 = nih_timer_add_timeout (NULL, 10, my_callback, &t1);
	timer2 = nih_timer_add_periodic (NULL, 20, my_callback, &t2);

	nih_alloc_set_destructor (timer1, my_destructor);
	nih_alloc_set_destructor (timer2, my_destructor);

	callback_called = 0;
	last_data = NULL;
	last_timer = NULL;
	destroyed = 0;

	timer1->due = time (NULL) - 5;
	nih_timer_poll ();

	/* Only one timer should have been triggered */
	if (callback_called != 1) {
		printf ("BAD: incorrect number of timers called.\n");
		ret = 1;
	}

	/* Timer should have been the first one */
	if (last_timer != timer1) {
		printf ("BAD: last timer called wasn't what we expected.\n");
		ret = 1;
	}

	/* Data should have been the data given to that timer */
	if (last_data != &t1) {
		printf ("BAD: last data wasn't what we expected.\n");
		ret = 1;
	}

	/* Timer should have been destroyed */
	if (! destroyed) {
		printf ("BAD: timer wasn't destroyed.\n");
		ret = 1;
	}

	callback_called = 0;
	last_data = NULL;
	last_timer = NULL;
	destroyed = 0;

	timer2->due = time (NULL) - 5;
	t1 = time (NULL);
	nih_timer_poll ();
	t2 = time (NULL);

	/* Only one timer should have been triggered */
	if (callback_called != 1) {
		printf ("BAD: incorrect number of timers called.\n");
		ret = 1;
	}

	/* Timer should have been the second one */
	if (last_timer != timer2) {
		printf ("BAD: last timer called wasn't what we expected.\n");
		ret = 1;
	}

	/* Data should have been the data given to that timer */
	if (last_data != &t2) {
		printf ("BAD: last data wasn't what we expected.\n");
		ret = 1;
	}

	/* Timer should not have been destroyed */
	if (destroyed) {
		printf ("BAD: timer was destroyed.\n");
		ret = 1;
	}

	/* Timer due date should have been updated */
	if ((timer2->due < t1 + 20) || (timer2->due > t2 + 20)) {
		printf ("BAD: timer due date not updated.\n");
		ret = 1;
	}

	nih_list_free (&timer2->entry);

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_add_timeout ();
	ret |= test_add_periodic ();
	ret |= test_add_scheduled ();
	ret |= test_next_due ();
	ret |= test_poll ();

	return ret;
}
