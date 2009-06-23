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

#ifndef NIH_SIGNAL_H
#define NIH_SIGNAL_H

#include <nih/macros.h>
#include <nih/list.h>

#include <signal.h>


/**
 * NihSignalHandler:
 * @data: pointer given with handler,
 * @signal: signal watch structure.
 *
 * A signal handler is called whenever the signal has been raised, it
 * is passed the NihSignal structure rather than the actual signal so that
 * it can be removed if desired.
 **/
typedef struct nih_signal NihSignal;
typedef void (*NihSignalHandler) (void *data, NihSignal *signal);

/**
 * NihSignal:
 * @entry: list header,
 * @signum: signal to catch,
 * @handler: function called when caught,
 * @data: pointer passed to @handler.
 *
 * This structure contains information about a function that should be
 * called whenever a particular signal is raised.  The calling is done
 * inside the main loop rather than inside the signal handler, so the
 * function is free to do whatever it wishes.
 *
 * The callback can be removed by using nih_list_remove() as they are
 * held in a list internally.
 **/
struct nih_signal {
	NihList           entry;
	int               signum;

	NihSignalHandler  handler;
	void             *data;
};


NIH_BEGIN_EXTERN

extern NihList *nih_signals;


void        nih_signal_init        (void);

int         nih_signal_set_handler (int signum, void (*handler)(int));
int         nih_signal_set_default (int signum);
int         nih_signal_set_ignore  (int signum);
void        nih_signal_reset       (void);

NihSignal * nih_signal_add_handler (const void *parent, int signum,
				   NihSignalHandler handler, void *data)
	__attribute__ ((warn_unused_result, malloc));

void        nih_signal_handler     (int signum);
void        nih_signal_poll        (void);

const char *nih_signal_to_name     (int signum);
int         nih_signal_from_name   (const char *signame);

NIH_END_EXTERN

#endif /* NIH_SIGNAL_H */
