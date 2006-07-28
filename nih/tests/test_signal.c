/* libnih
 *
 * test_signal.c - test suite for nih/signal.c
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

#include <stdio.h>
#include <assert.h>
#include <signal.h>

#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/signal.h>


static void
my_handler (int signum) {
}

int
test_set_handler (void)
{
	struct sigaction act;
	int              ret = 0, retval, i;

	printf ("Testing nih_signal_set_handler()\n");
	retval = nih_signal_set_handler (SIGUSR1, my_handler);

	/* Return value should be zero */
	if (retval != 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Check installed signal action */
	assert (sigaction (SIGUSR1, NULL, &act) == 0);

	/* Handler should be function given */
	if (act.sa_handler != my_handler) {
		printf ("BAD: signal handler set incorrectly.\n");
		ret = 1;
	}

	/* Flags should contain SA_RESTART */
	if (! (act.sa_flags & SA_RESTART)) {
		printf ("BAD: signal flags set incorrectly.\n");
		ret = 1;
	}

	/* Flags should not contain SA_RESETHAND */
	if (act.sa_flags & SA_RESETHAND) {
		printf ("BAD: signal flags set incorrectly.\n");
		ret = 1;
	}

	/* Mask should be empty */
	for (i = 1; i < 32; i++) {
		if (sigismember (&act.sa_mask, i)) {
			printf ("BAD: signal mask not empty set.\n");
			ret = 1;
			break;
		}
	}

	return ret;
}

int
test_set_default (void)
{
	struct sigaction act;
	int              ret = 0, retval, i;

	printf ("Testing nih_signal_set_default()\n");
	retval = nih_signal_set_default (SIGUSR1);

	/* Return value should be zero */
	if (retval != 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Check installed signal action */
	assert (sigaction (SIGUSR1, NULL, &act) == 0);

	/* Handler should be the default */
	if (act.sa_handler != SIG_DFL) {
		printf ("BAD: signal handler set incorrectly.\n");
		ret = 1;
	}

	/* Flags should contain SA_RESTART */
	if (! (act.sa_flags & SA_RESTART)) {
		printf ("BAD: signal flags set incorrectly.\n");
		ret = 1;
	}

	/* Flags should not contain SA_RESETHAND */
	if (act.sa_flags & SA_RESETHAND) {
		printf ("BAD: signal flags set incorrectly.\n");
		ret = 1;
	}

	/* Mask should be empty */
	for (i = 1; i < 32; i++) {
		if (sigismember (&act.sa_mask, i)) {
			printf ("BAD: signal mask not empty set.\n");
			ret = 1;
			break;
		}
	}

	return ret;
}

int
test_set_ignore (void)
{
	struct sigaction act;
	int              ret = 0, retval, i;

	printf ("Testing nih_signal_set_ignore()\n");
	retval = nih_signal_set_ignore (SIGUSR1);

	/* Return value should be zero */
	if (retval != 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Check installed signal action */
	assert (sigaction (SIGUSR1, NULL, &act) == 0);

	/* Handler should be the ignore special */
	if (act.sa_handler != SIG_IGN) {
		printf ("BAD: signal handler set incorrectly.\n");
		ret = 1;
	}

	/* Flags should contain SA_RESTART */
	if (! (act.sa_flags & SA_RESTART)) {
		printf ("BAD: signal flags set incorrectly.\n");
		ret = 1;
	}

	/* Flags should not contain SA_RESETHAND */
	if (act.sa_flags & SA_RESETHAND) {
		printf ("BAD: signal flags set incorrectly.\n");
		ret = 1;
	}

	/* Mask should be empty */
	for (i = 1; i < 32; i++) {
		if (sigismember (&act.sa_mask, i)) {
			printf ("BAD: signal mask not empty set.\n");
			ret = 1;
			break;
		}
	}

	return ret;
}


static int callback_called = 0;
static void *last_data;
static NihSignal *last_signal;

static void
my_callback (void *data, NihSignal *signal)
{
	callback_called++;
	last_data = data;
	last_signal = signal;
}

int
test_add_callback (void)
{
	NihSignal *signal;
	int        ret = 0;

	printf ("Testing nih_signal_add_callback()\n");
	signal = nih_signal_add_callback (SIGUSR1, my_callback, &ret);

	/* Signal number should be number given */
	if (signal->signum != SIGUSR1) {
		printf ("BAD: signal number set incorrectly.\n");
		ret = 1;
	}

	/* Callback should be callback given */
	if (signal->callback != my_callback) {
		printf ("BAD: callback set incorrectly.\n");
		ret = 1;
	}

	/* Callback data should be pointer given */
	if (signal->data != &ret) {
		printf ("BAD: callback data set incorrectly.\n");
		ret = 1;
	}

	/* Should be in the signals list */
	if (NIH_LIST_EMPTY (&signal->entry)) {
		printf ("BAD: not placed into signals list.\n");
		ret = 1;
	}

	/* Should have been allocated using nih_alloc */
	if (nih_alloc_size (signal) != sizeof (NihSignal)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	nih_list_free (&signal->entry);

	return ret;
}

int
test_poll (void)
{
	NihSignal *signal1, *signal2;
	int        ret = 0;

	printf ("Testing nih_signal_poll()\n");
	signal1 = nih_signal_add_callback (SIGUSR1, my_callback, &ret);
	signal2 = nih_signal_add_callback (SIGUSR2, my_callback, &ret);

	callback_called = 0;
	last_data = NULL;
	last_signal = NULL;
	nih_signal_handler (SIGUSR1);
	nih_signal_poll ();

	/* Only one signal should have been triggered */
	if (callback_called != 1) {
		printf ("BAD: incorrect number of signals called.\n");
		ret = 1;
	}

	/* Signal should have been the first one */
	if (last_signal != signal1) {
		printf ("BAD: last signal wasn't what we expected.\n");
		ret = 1;
	}

	/* Data should have been the data pointer of the first one */
	if (last_data != &ret) {
		printf ("BAD: last data wasn't what we expected.\n");
		ret = 1;
	}

	callback_called = 0;
	last_data = NULL;
	last_signal = NULL;
	nih_signal_handler (SIGUSR2);
	nih_signal_poll ();

	/* Only one signal should have been triggered */
	if (callback_called != 1) {
		printf ("BAD: incorrect number of signals called.\n");
		ret = 1;
	}

	/* Signal should have been the second one */
	if (last_signal != signal2) {
		printf ("BAD: last signal wasn't what we expected.\n");
		ret = 1;
	}

	/* Data should have been the data pointer of the first one */
	if (last_data != &ret) {
		printf ("BAD: last data wasn't what we expected.\n");
		ret = 1;
	}

	callback_called = 0;
	nih_signal_handler (SIGINT);
	nih_signal_poll ();

	/* No signals should have been triggered */
	if (callback_called != 0) {
		printf ("BAD: signals called unexpectedly.\n");
		ret = 1;
	}

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_set_handler ();
	ret |= test_set_default ();
	ret |= test_set_ignore ();
	ret |= test_add_callback ();
	ret |= test_poll ();

	return ret;
}
