/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

NihList *nih_signals;


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
