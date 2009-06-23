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

#ifndef NIH_MAIN_H
#define NIH_MAIN_H

#include <nih/macros.h>
#include <nih/list.h>
#include <nih/signal.h>


/**
 * NihMainLoopCb:
 * @data: pointer given with callback,
 * @loop: loop callback structure.
 *
 * Main loop callbacks are called each time through the main loop to
 * perform any additional processing before the process is put back
 * to sleep.  For anything time critical, timers should be used instead.
 *
 * The @loop pointer can be used to remove the callback.
 **/
typedef struct nih_main_loop_func NihMainLoopFunc;
typedef void (*NihMainLoopCb) (void *data, NihMainLoopFunc *loop);

/**
 * NihMainLoopFunc:
 * @entry: list header,
 * @callback: function called,
 * @data: pointer passed to @callback.
 *
 * This structure contains information about a function that should be
 * called once in each main loop iteration.
 *
 * The callback can be removed by using nih_list_remove() as they are
 * held in a list internally.
 **/
struct nih_main_loop_func {
	NihList        entry;

	NihMainLoopCb  callback;
	void          *data;
};


/**
 * nih_main_init_gettext:
 *
 * Initialises gettext using the PACKAGE_NAME and LOCALEDIR macros that should
 * be set by Autoconf/Automake.
 **/
#if ENABLE_NLS
# ifndef LOCALEDIR
#  define LOCALEDIR NULL
# endif /* LOCALEDIR */

# define nih_main_init_gettext() \
	do { \
		setlocale (LC_ALL, ""); \
		bindtextdomain (PACKAGE_NAME, LOCALEDIR); \
		textdomain (PACKAGE_NAME); \
	} while (0)
#else /* ENABLE_NLS */
# define nih_main_init_gettext()
#endif /* ENABLE_NLS */

/**
 * nih_main_init:
 * @argv0: program name from arguments.
 *
 * Should be called at the beginning of main() to initialise the various
 * global variables exported from this module.  Expands both
 * nih_main_init_gettext() and nih_main_init_full()
 * passing values set by Autoconf AC_INIT and AC_COPYRIGHT macros.
 **/
#ifndef PACKAGE_COPYRIGHT
# define PACKAGE_COPYRIGHT NULL
#endif /* PACKAGE_COPYRIGHT */
#define nih_main_init(argv0) \
	do { \
		nih_main_init_gettext (); \
		nih_main_init_full (argv0, PACKAGE_NAME, PACKAGE_VERSION, \
				    PACKAGE_BUGREPORT, PACKAGE_COPYRIGHT); \
	} while (0);


NIH_BEGIN_EXTERN

extern const char *program_name;
extern const char *package_name;
extern const char *package_version;
extern const char *package_copyright;
extern const char *package_bugreport;
extern const char *package_string;

extern NihList *nih_main_loop_functions;


void             nih_main_init_full      (const char *argv0,
					  const char *package,
					  const char *version,
					  const char *bugreport,
					  const char *copyright);

void             nih_main_suggest_help   (void);
void             nih_main_version        (void);

int              nih_main_daemonise      (void)
	__attribute__ ((warn_unused_result));

void             nih_main_set_pidfile    (const char *filename);
const char *     nih_main_get_pidfile    (void);
pid_t            nih_main_read_pidfile   (void);
int              nih_main_write_pidfile  (pid_t pid)
	__attribute__ ((warn_unused_result));
void             nih_main_unlink_pidfile (void);

void             nih_main_loop_init      (void);
int              nih_main_loop           (void);
void             nih_main_loop_interrupt (void);
void             nih_main_loop_exit      (int status);

NihMainLoopFunc *nih_main_loop_add_func  (const void *parent,
					  NihMainLoopCb callback, void *data)
	__attribute__ ((warn_unused_result, malloc));

void             nih_main_term_signal    (void *data, NihSignal *signal);

NIH_END_EXTERN

#endif /* NIH_MAIN_H */
