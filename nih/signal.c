/* libnih
 *
 * signal.c - easier and main-loop signal handling
 *
 * Copyright © 2007 Scott James Remnant <scott@netsplit.com>.
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


#include <signal.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/main.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "signal.h"


/**
 * NUM_SIGNALS:
 *
 * Highest number that is not used for a signal that we care about, on
 * Linux this is always 32.
 **/
#define NUM_SIGNALS 32

/**
 * signals_caught:
 *
 * This array is used to indicate whether a signal has been caught or not,
 * the signal handler increments the appropriate array entry for the signal
 * and nih_signal_poll() clears it again once functions have been dispatched.
 **/
static volatile sig_atomic_t signals_caught[NUM_SIGNALS];

/**
 * signals:
 *
 * This is the list of registered signals, not sorted into any particular
 * order.  Each item is an NihSignal structure.
 **/
static NihList *signals = NULL;


/**
 * nih_signal_init:
 *
 * Initialise the list of signals.
 **/
static inline void
nih_signal_init (void)
{
	if (! signals)
		NIH_MUST (signals = nih_list_new (NULL));
}


/**
 * nih_signal_set_handler:
 * @signum: signal number,
 * @handler: new handler function.
 *
 * Sets signal @signum to call the @handler function when raised, with
 * sensible defaults for the flags and signal mask.
 *
 * Returns: zero on success, negative value on invalid signal.
 **/
int
nih_signal_set_handler (int    signum,
			void (*handler)(int))
{
	struct sigaction act;

	nih_assert (signum > 0);
	nih_assert (signum < NUM_SIGNALS);
	nih_assert (handler != NULL);

	act.sa_handler = handler;
	act.sa_flags = 0;
	if (signum != SIGALRM)
		act.sa_flags |= SA_RESTART;
	if (signum == SIGCHLD)
		act.sa_flags |= SA_NOCLDSTOP;
	sigemptyset (&act.sa_mask);

	if (sigaction (signum, &act, NULL) < 0)
		return -1;

	return 0;
}

/**
 * nih_signal_set_default:
 * @signum: signal number.
 *
 * Sets signal @signum to perform the operating system default action when
 * raised, with sensible defaults for the flags and signal mask.
 *
 * Returns: zero on success, negative value on invalid signal.
 **/
int
nih_signal_set_default (int signum)
{
	struct sigaction act;

	nih_assert (signum > 0);
	nih_assert (signum < NUM_SIGNALS);

	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;
	sigemptyset (&act.sa_mask);

	if (sigaction (signum, &act, NULL) < 0)
		return -1;

	return 0;
}

/**
 * nih_signal_set_ignore:
 * @signum: signal number.
 *
 * Sets signal @signum to be ignored, with sensible defaults for the flags
 * and signal mask.
 *
 * Returns: zero on success, negative value on invalid signal.
 **/
int
nih_signal_set_ignore (int signum)
{
	struct sigaction act;

	nih_assert (signum > 0);
	nih_assert (signum < NUM_SIGNALS);

	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	sigemptyset (&act.sa_mask);

	if (sigaction (signum, &act, NULL) < 0)
		return -1;

	return 0;
}

/**
 * nih_signal_reset:
 *
 * Resets all signals to their default handling.
 **/
void
nih_signal_reset (void)
{
	int i;

	for (i = 1; i < NUM_SIGNALS; i++)
		nih_signal_set_default (i);
}


/**
 * nih_signal_add_handler:
 * @parent: parent of structure,
 * @signum: signal number to catch,
 * @handler: function to call,
 * @data: pointer to pass to @handler.
 *
 * Adds @handler to the list of functions that should be called by
 * nih_signal_poll() if the @signum signal was raised.  The signal must first
 * have been set to nih_signal_handler() using nih_signal_set_handler(),
 *
 * The callback structure is allocated using nih_alloc() and stored in a
 * linked list, a default destructor is set that removes the handler from
 * the list.  Removal of the handler can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the signal information, or NULL if insufficient memory.
 **/
NihSignal *
nih_signal_add_handler (const void       *parent,
			int               signum,
			NihSignalHandler  handler,
			void             *data)
{
	NihSignal *signal;

	nih_assert (signum > 0);
	nih_assert (signum < NUM_SIGNALS);
	nih_assert (handler != NULL);

	nih_signal_init ();

	signal = nih_new (parent, NihSignal);
	if (! signal)
		return NULL;

	nih_list_init (&signal->entry);
	nih_alloc_set_destructor (signal, (NihDestructor)nih_list_destructor);

	signal->signum = signum;

	signal->handler = handler;
	signal->data = data;

	nih_list_add (signals, &signal->entry);

	return signal;
}


/**
 * nih_signal_handler:
 * @signum: signal raised.
 *
 * This signal handler should be used for any signals that you wish to use
 * nih_signal_add_handler() and nih_signal_poll() for.  It simply sets a
 * volatile sig_atomic_t variable so that the signal can be detected in
 * the main loop.
 **/
void
nih_signal_handler (int signum)
{
	nih_assert (signum > 0);
	nih_assert (signum < NUM_SIGNALS);

	signals_caught[signum]++;

	nih_main_loop_interrupt ();
}

/**
 * nih_signal_poll:
 *
 * Iterate the list of registered signal handlers and call the function
 * if that signal has been raised since the last time nih_signal_poll() was
 * called.
 *
 * It is safe for the handler to remove itself.
 **/
void
nih_signal_poll (void)
{
	int s;

	nih_signal_init ();

	NIH_LIST_FOREACH_SAFE (signals, iter) {
		NihSignal *signal = (NihSignal *)iter;

		if (! signals_caught[signal->signum])
			continue;

		signal->handler (signal->data, signal);
	}

	for (s = 0; s < NUM_SIGNALS; s++)
		signals_caught[s] = 0;
}
