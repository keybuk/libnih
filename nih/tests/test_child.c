/* libnih
 *
 * test_child.c - test suite for nih/child.c
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

#include <nih/test.h>

#if HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#endif /* HAVE_VALGRIND_VALGRIND_H */

#include <sys/ptrace.h>

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/child.h>


static int handler_called = 0;
static void *last_data = NULL;
static pid_t last_pid;
static NihChildEvents last_event = -1;
static int last_status;

static void
my_handler (void           *data,
	    pid_t           pid,
	    NihChildEvents  event,
	    int             status)
{
	handler_called++;
	last_data = data;
	last_pid = pid;
	last_event = event;
	last_status = status;
}

void
test_add_watch (void)
{
	NihChildWatch *watch;

	TEST_FUNCTION ("nih_child_add_watch");
	nih_child_poll ();


	/* Check that we can add a watch on a specific pid, and that the
	 * structure is filled in correctly and part of a list.
	 */
	TEST_FEATURE ("with pid");
	TEST_ALLOC_FAIL {
		watch = nih_child_add_watch (NULL, getpid (), NIH_CHILD_EXITED,
					     my_handler, &watch);

		if (test_alloc_failed) {
			TEST_EQ_P (watch, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (watch, sizeof (NihChildWatch));
		TEST_EQ (watch->pid, getpid ());
		TEST_EQ (watch->events, NIH_CHILD_EXITED);
		TEST_EQ_P (watch->handler, my_handler);
		TEST_EQ_P (watch->data, &watch);
		TEST_LIST_NOT_EMPTY (&watch->entry);

		nih_free (watch);
	}


	/* Check that we can add a watch on a pid of -1, which represents
	 * any child.
	 */
	TEST_FEATURE ("with -1 for pid");
	TEST_ALLOC_FAIL {
		watch = nih_child_add_watch (NULL, -1, NIH_CHILD_ALL,
					     my_handler, &watch);

		if (test_alloc_failed) {
			TEST_EQ_P (watch, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (watch, sizeof (NihChildWatch));
		TEST_EQ (watch->pid, -1);
		TEST_EQ (watch->events, NIH_CHILD_ALL);
		TEST_EQ_P (watch->handler, my_handler);
		TEST_EQ_P (watch->data, &watch);
		TEST_LIST_NOT_EMPTY (&watch->entry);

		nih_free (watch);
	}
}


void
test_poll (void)
{
	NihChildWatch *watch;
	siginfo_t      siginfo;
	pid_t          pid, child;
	unsigned long  data;
	char           corefile[PATH_MAX + 1];

	TEST_FUNCTION ("nih_child_poll");

	/* Check that when a child exits normally, the handler receives
	 * an exited event and the zero status code and is then removed from
	 * the list and freed.
	 */
	TEST_FEATURE ("with normal termination");

	TEST_CHILD (pid) {
		exit (0);
	}

	watch = nih_child_add_watch (NULL, pid, NIH_CHILD_EXITED,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	waitid (P_PID, pid, &siginfo, WEXITED | WNOWAIT);

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	TEST_EQ (last_event, NIH_CHILD_EXITED);
	TEST_EQ (last_status, 0);
	TEST_FREE (watch);


	/* Check that when a child exits with a non-zero status code, the
	 * reaper receives an exited event and the status code and is then
	 * removed from the list and freed.
	 */
	TEST_FEATURE ("with normal non-zero termination");

	TEST_CHILD (pid) {
		exit (123);
	}

	watch = nih_child_add_watch (NULL, pid, NIH_CHILD_EXITED,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	waitid (P_PID, pid, &siginfo, WEXITED | WNOWAIT);

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	TEST_EQ (last_event, NIH_CHILD_EXITED);
	TEST_EQ (last_status, 123);
	TEST_FREE (watch);


	/* Check that when a child is killed by a signal, the reaper receives
	 * a killed event with the signal in the status field before being
	 * removed from the list and freed.
	 */
	TEST_FEATURE ("with termination by signal");

	TEST_CHILD (pid) {
		pause ();
	}

	watch = nih_child_add_watch (NULL, pid,
				     NIH_CHILD_KILLED | NIH_CHILD_DUMPED,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	kill (pid, SIGTERM);
	waitid (P_PID, pid, &siginfo, WEXITED | WNOWAIT);

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	TEST_EQ (last_event, NIH_CHILD_KILLED);
	TEST_EQ (last_status, SIGTERM);
	TEST_FREE (watch);


	/* Check that when a child is killed by aborting, the reaper receives
	 * a dumped event with the signal in the status field before being
	 * removed from the list and freed.
	 */
	TEST_FEATURE ("with termination by abort");

	TEST_CHILD (pid) {
		abort ();
	}

	watch = nih_child_add_watch (NULL, pid,
				     NIH_CHILD_KILLED | NIH_CHILD_DUMPED,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	waitid (P_PID, pid, &siginfo, WEXITED | WNOWAIT);

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	/* We might get killed if we never dumped core... fiddling with
	 * the limit doesn't help, since we might be under gdb and that
	 * never lets us dump core.
	 */
	if (last_event != NIH_CHILD_KILLED)
		TEST_EQ (last_event, NIH_CHILD_DUMPED);

	TEST_EQ (last_status, SIGABRT);
	TEST_FREE (watch);

	unlink ("core");

	sprintf (corefile, "core.%d", pid);
	unlink (corefile);

	sprintf (corefile, "vgcore.%d", pid);
	unlink (corefile);


	/* Check that when a child emits the stopped signal, the reaper
	 * receives a stopped event with nothing relevant in the status field.
	 * It should not be removed from the list, since the child hasn't
	 * gone anyway.
	 */
	TEST_FEATURE ("with stopped child");

	TEST_CHILD (pid) {
		raise (SIGSTOP);
		pause ();
		exit (0);
	}

	watch = nih_child_add_watch (NULL, pid,
				     NIH_CHILD_STOPPED | NIH_CHILD_CONTINUED,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	waitid (P_PID, pid, &siginfo, WSTOPPED | WNOWAIT);

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	TEST_EQ (last_event, NIH_CHILD_STOPPED);
	TEST_EQ (last_status, SIGSTOP);
	TEST_NOT_FREE (watch);


	/* Check that when the child is continued again, the reaper
	 * receives a continued event with nothing relevant in the status
	 * field.  It should still not be removed from the list since the
	 * child still hasn't gone away.
	 */
	TEST_FEATURE ("with continued child");
	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	kill (pid, SIGCONT);

	waitid (P_PID, pid, &siginfo, WCONTINUED | WNOWAIT);

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	TEST_EQ (last_event, NIH_CHILD_CONTINUED);
	TEST_EQ (last_status, SIGCONT);
	TEST_NOT_FREE (watch);

	kill (pid, SIGTERM);
	waitid (P_PID, pid, &siginfo, WEXITED);
	nih_free (watch);


	/* Check that a signal raised from a traced child causes the reaper
	 * to be called with a traced event and the event in the status
	 * field.  It should not be removed from the list since the child
	 * hasn't gone away.
	 */
	TEST_FEATURE ("with signal from traced child");

	TEST_CHILD (pid) {
		assert0 (ptrace (PTRACE_TRACEME, 0, NULL, NULL));

		raise (SIGSTOP);
		raise (SIGCHLD);
		pause ();
		exit (0);
	}

	waitid (P_PID, pid, &siginfo, WSTOPPED);

	assert0 (ptrace (PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACESYSGOOD));
	assert0 (ptrace (PTRACE_CONT, pid, NULL, SIGCONT));

	waitid (P_PID, pid, &siginfo, WSTOPPED | WNOWAIT);

	watch = nih_child_add_watch (NULL, pid, NIH_CHILD_TRAPPED,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	TEST_EQ (last_event, NIH_CHILD_TRAPPED);
	TEST_EQ (last_status, SIGCHLD);
	TEST_NOT_FREE (watch);

	assert0 (ptrace (PTRACE_DETACH, pid, NULL, 0));

	kill (pid, SIGTERM);
	waitid (P_PID, pid, &siginfo, WEXITED);
	nih_free (watch);


#if HAVE_VALGRIND_VALGRIND_H
	/* These tests fail when running under valgrind.
	 */
	if (! RUNNING_ON_VALGRIND) {
#endif
	/* Check that when a traced child forks it causes the reaper
	 * to be called with a ptrace event and the fork event in the
	 * status field.  It should not be removed from the list since the
	 * child hasn't gone away.
	 */
	TEST_FEATURE ("with fork by traced child");

	TEST_CHILD (pid) {
		assert0 (ptrace (PTRACE_TRACEME, 0, NULL, NULL));

		raise (SIGSTOP);

		child = fork ();
		assert (child >= 0);

		pause ();

		exit (0);
	}

	waitid (P_PID, pid, &siginfo, WSTOPPED);

	assert0 (ptrace (PTRACE_SETOPTIONS, pid, NULL,
			 PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK));
	assert0 (ptrace (PTRACE_CONT, pid, NULL, SIGCONT));

	/* Wait for ptrace to stop the parent (signalling the fork) */
	waitid (P_PID, pid, &siginfo, WSTOPPED | WNOWAIT);

	/* Will be able to get the child pid now, we have to do it here
	 * because we want to wait on it to ensure the test is synchronous;
	 * otherwise nih_child_poll() could actually eat the child event
	 * before it returns -- normally we'd do this inside the handler,
	 * so things would probably work (we iter the handlers for each
	 * event, so you can add one).
	 */
	data = 0;
	assert0 (ptrace (PTRACE_GETEVENTMSG, pid, NULL, &data));
	assert (data != 0);
	child = (pid_t)data;

	/* Wait for ptrace to stop the child, otherwise it might not be
	 * ready for us to actually detach from.
	 */
	waitid (P_PID, child, &siginfo, WSTOPPED | WNOWAIT);

	watch = nih_child_add_watch (NULL, pid, NIH_CHILD_PTRACE,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	TEST_EQ (last_event, NIH_CHILD_PTRACE);
	TEST_EQ (last_status, PTRACE_EVENT_FORK);
	TEST_NOT_FREE (watch);

	assert0 (ptrace (PTRACE_DETACH, child, NULL, SIGCONT));
	kill (child, SIGTERM);

	assert0 (ptrace (PTRACE_DETACH, pid, NULL, SIGCONT));
	kill (pid, SIGTERM);
	waitid (P_PID, pid, &siginfo, WEXITED);
	nih_free (watch);


	/* Check that when a traced child execs it causes the reaper
	 * to be called with a ptrace event and the exec event in the
	 * status field.  It should not be removed from the list since the
	 * child hasn't gone away.
	 */
	TEST_FEATURE ("with exec by traced child");

	TEST_CHILD (pid) {
		assert0 (ptrace (PTRACE_TRACEME, 0, NULL, NULL));

		raise (SIGSTOP);

		execl ("/bin/true", "true", NULL);
		exit (255);
	}

	waitid (P_PID, pid, &siginfo, WSTOPPED);

	assert0 (ptrace (PTRACE_SETOPTIONS, pid, NULL,
			 PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC));
	assert0 (ptrace (PTRACE_CONT, pid, NULL, SIGCONT));

	waitid (P_PID, pid, &siginfo, WSTOPPED | WNOWAIT);

	watch = nih_child_add_watch (NULL, pid, NIH_CHILD_PTRACE,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	TEST_EQ (last_event, NIH_CHILD_PTRACE);
	TEST_EQ (last_status, PTRACE_EVENT_EXEC);
	TEST_NOT_FREE (watch);

	assert0 (ptrace (PTRACE_DETACH, pid, NULL, SIGCONT));

	waitid (P_PID, pid, &siginfo, WEXITED);
	nih_free (watch);
#if HAVE_VALGRIND_VALGRIND_H
	}
#endif


	/* Check that we can watch for events from any process, which
	 * shouldn't be freed when the child dies.
	 */
	TEST_FEATURE ("with generic watcher");

	TEST_CHILD (pid) {
		pause ();
	}

	watch = nih_child_add_watch (NULL, -1, NIH_CHILD_ALL,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	kill (pid, SIGTERM);
	waitid (P_PID, pid, &siginfo, WEXITED | WNOWAIT);

	nih_child_poll ();

	TEST_TRUE (handler_called);
	TEST_EQ (last_pid, pid);
	TEST_EQ (last_event, NIH_CHILD_KILLED);
	TEST_EQ (last_status, SIGTERM);
	TEST_NOT_FREE (watch);

	nih_free (watch);


	/* Check that if we poll with an unknown pid, and no catch-all,
	 * nothing is triggered and the watch is not removed.
	 */
	TEST_FEATURE ("with pid-specific watcher and wrong pid");

	TEST_CHILD (pid) {
		pause ();
	}

	watch = nih_child_add_watch (NULL, pid - 1, NIH_CHILD_ALL,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	kill (pid, SIGTERM);
	waitid (P_PID, pid, &siginfo, WEXITED | WNOWAIT);

	nih_child_poll ();

	TEST_FALSE (handler_called);
	TEST_NOT_FREE (watch);

	nih_free (watch);


	/* Check that if we poll with a known pid but for a different event
	 * set, nothing is triggered and the watch is not removed.
	 */
	TEST_FEATURE ("with event-specific watcher and wrong event");

	TEST_CHILD (pid) {
		pause ();
	}

	watch = nih_child_add_watch (NULL, pid, NIH_CHILD_STOPPED,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	last_data = NULL;
	last_pid = 0;
	last_event = -1;
	last_status = 0;

	kill (pid, SIGTERM);
	waitid (P_PID, pid, &siginfo, WEXITED | WNOWAIT);

	nih_child_poll ();

	TEST_FALSE (handler_called);
	TEST_NOT_FREE (watch);

	nih_free (watch);


	/* Check that a poll when nothing has died does nothing. */
	TEST_FEATURE ("with nothing dead");

	TEST_CHILD (pid) {
		pause ();
	}

	watch = nih_child_add_watch (NULL, -1, NIH_CHILD_ALL,
				     my_handler, &watch);

	TEST_FREE_TAG (watch);

	handler_called = 0;
	nih_child_poll ();

	TEST_FALSE (handler_called);
	TEST_NOT_FREE (watch);

	kill (pid, SIGTERM);
	waitpid (pid, NULL, 0);


	/* Check that a poll when there are no child processes does nothing */
	TEST_FEATURE ("with no children");
	handler_called = 0;
	nih_child_poll ();

	TEST_FALSE (handler_called);
	TEST_NOT_FREE (watch);

	nih_free (watch);
}


int
main (int   argc,
      char *argv[])
{
	test_add_watch ();
	test_poll ();

	return 0;
}
