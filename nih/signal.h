/* libnih
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

#ifndef NIH_SIGNAL_H
#define NIH_SIGNAL_H

#include <nih/macros.h>
#include <nih/list.h>


/**
 * NihSignalCb:
 *
 * The signal callback is called whenever the signal has been raised, it
 * is passed the data pointer given when the signal was registered and the
 * signal structure.
 **/
typedef struct nih_signal NihSignal;
typedef void (*NihSignalCb) (void *, NihSignal *);

/**
 * NihSignal:
 * @entry: list header,
 * @signum: signal to catch,
 * @callback: function called when caught,
 * @data: pointer passed to callback.
 *
 * This structure contains information about a function that should be
 * called whenever a particular signal is raised.  The calling is done
 * inside the main loop rather than inside the signal handler, so the
 * function is free to do whatever it wishes.
 *
 * The callback can be removed by using #nih_list_remove as they are
 * held in a list internally.
 **/
struct nih_signal {
	NihList      entry;
	int          signum;

	NihSignalCb  callback;
	void        *data;
};


NIH_BEGIN_EXTERN

int        nih_signal_set_handler  (int signum, void (*handler)(int));
int        nih_signal_set_default  (int signum);
int        nih_signal_set_ignore   (int signum);

NihSignal *nih_signal_add_callback (int signum, NihSignalCb callback,
				    void *data);

void       nih_signal_handler      (int signum);
void       nih_signal_poll         (void);

NIH_END_EXTERN

#endif /* NIH_SIGNAL_H */
