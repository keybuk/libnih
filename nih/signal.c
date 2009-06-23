/* libnih
 *
 * signal.c - easier and main-loop signal handling
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


#include <string.h>
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
 * SignalName:
 * @num: number of signal,
 * @name: name of signal.
 *
 * Structure used to associate signal names and numbers to each other.
 **/
typedef struct {
	int         num;
	const char *name;
} SignalName;

/**
 * signal_names:
 *
 * List of signal name to number mappings, the last entry in the list has
 * a NULL name and negative number.
 **/
static const SignalName signal_names[] = {
	{ SIGHUP,    "HUP"    },
	{ SIGINT,    "INT"    },
	{ SIGQUIT,   "QUIT"   },
	{ SIGILL,    "ILL"    },
	{ SIGTRAP,   "TRAP"   },
	{ SIGABRT,   "ABRT"   },
	{ SIGIOT,    "IOT"    },
#ifdef SIGEMT
	{ SIGEMT,    "EMT"    },
#endif
	{ SIGBUS,    "BUS"    },
	{ SIGFPE,    "FPE"    },
	{ SIGKILL,   "KILL"   },
	{ SIGUSR1,   "USR1"   },
	{ SIGSEGV,   "SEGV"   },
	{ SIGUSR2,   "USR2"   },
	{ SIGPIPE,   "PIPE"   },
	{ SIGALRM,   "ALRM"   },
	{ SIGTERM,   "TERM"   },
#ifdef SIGSTKFLT
	{ SIGSTKFLT, "STKFLT" },
#endif
	{ SIGCHLD,   "CHLD"   },
	{ SIGCLD,    "CLD"    },
	{ SIGCONT,   "CONT"   },
	{ SIGSTOP,   "STOP"   },
	{ SIGTSTP,   "TSTP"   },
	{ SIGTTIN,   "TTIN"   },
	{ SIGTTOU,   "TTOU"   },
	{ SIGURG,    "URG"    },
	{ SIGXCPU,   "XCPU"   },
	{ SIGXFSZ,   "XFSZ"   },
	{ SIGVTALRM, "VTALRM" },
	{ SIGPROF,   "PROF"   },
	{ SIGWINCH,  "WINCH"  },
	{ SIGIO,     "IO"     },
	{ SIGPOLL,   "POLL"   },
#ifdef SIGLOST
	{ SIGLOST,   "LOST"   },
#endif
	{ SIGPWR,    "PWR"    },
	{ SIGSYS,    "SYS"    },
#ifdef SIGUNUSED
	{ SIGUNUSED, "UNUSED" },
#endif

	{ -1, NULL }
};

/**
 * signals_caught:
 *
 * This array is used to indicate whether a signal has been caught or not,
 * the signal handler increments the appropriate array entry for the signal
 * and nih_signal_poll() clears it again once functions have been dispatched.
 **/
static volatile sig_atomic_t signals_caught[NUM_SIGNALS];

/**
 * nih_signals:
 *
 * This is the list of registered signals, not sorted into any particular
 * order.  Each item is an NihSignal structure.
 **/
NihList *nih_signals = NULL;


/**
 * nih_signal_init:
 *
 * Initialise the list of signals.
 **/
void
nih_signal_init (void)
{
	if (! nih_signals)
		nih_signals = NIH_MUST (nih_list_new (NULL));
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
 * @parent: parent object for new signal,
 * @signum: signal number to catch,
 * @handler: function to call,
 * @data: pointer to pass to @handler.
 *
 * Adds @handler to the list of functions that should be called by
 * nih_signal_poll() if the @signum signal was raised.  The signal must first
 * have been set to nih_signal_handler() using nih_signal_set_handler(),
 *
 * The callback structure is allocated using nih_alloc() and stored in a
 * linked list; there is no non-allocated version because of this.
 *
 * Removal of the handler can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned signal.  When all parents
 * of the returned signal are freed, the returned signal will also be
 * freed.
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

	nih_alloc_set_destructor (signal, nih_list_destroy);

	signal->signum = signum;

	signal->handler = handler;
	signal->data = data;

	nih_list_add (nih_signals, &signal->entry);

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

	NIH_LIST_FOREACH_SAFE (nih_signals, iter) {
		NihSignal *signal = (NihSignal *)iter;

		if (! signals_caught[signal->signum])
			continue;

		signal->handler (signal->data, signal);
	}

	for (s = 0; s < NUM_SIGNALS; s++)
		signals_caught[s] = 0;
}


/**
 * nih_signal_to_name:
 * @signum: signal number to look up.
 *
 * Looks up @signum in the table of signal names and returns the common
 * name for the signal, in the form TERM/CHLD.
 *
 * Returns: static name string or NULL if signal is unknown.
 **/
const char *
nih_signal_to_name (int signum)
{
	const SignalName *sig;

	nih_assert (signum > 0);

	for (sig = signal_names; (sig->num > 0) && sig->name; sig++)
		if (sig->num == signum)
			return sig->name;

	return NULL;
}

/**
 * nih_signal_from_name:
 * @signame: signal name to look up.
 *
 * Looks up @signame in the table of signal names and returns the
 * number for the signal; @signame may be in the form SIGTERM or TERM.
 *
 * Returns: signal number or negative value if signal is unknown.
 **/
int
nih_signal_from_name (const char *signame)
{
	const SignalName *sig;

	nih_assert (signame != NULL);

	if (! strncmp (signame, "SIG", 3))
		signame += 3;

	for (sig = signal_names; (sig->num > 0) && sig->name; sig++)
		if (! strcmp (sig->name, signame))
			return sig->num;

	return -1;
}
