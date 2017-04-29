#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <paths.h>

#include <fcntl.h>

#include <err.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "wait.h"

static void	wait_timedout(int);

/*
 * fork off and become a daemon, but wait for the child to come online
 * before returning to the parent, or we get disk thrashing at boot etc.
 * Set a timer so we don't hang forever if it wedges.
 */
int
wait_daemon(int maxwait)
{
	int fd;
	int status;
	pid_t pid, childpid;

	switch (childpid = fork()) {
	case -1:
		return (-1);
	case 0:
		break;
	default:
		signal(SIGALRM, wait_timedout);
		alarm(maxwait);
		while ((pid = wait3(&status, 0, NULL)) != -1) {
			if (WIFEXITED(status))
				errx(1, "child pid %d exited with return code %d",
					pid, WEXITSTATUS(status));
			if (WIFSIGNALED(status))
				errx(1, "child pid %d exited on signal %d%s",
					pid, WTERMSIG(status),
					WCOREDUMP(status) ? " (core dumped)" :
					"");
			if (pid == childpid)	/* it's gone... */
				break;
		}
		fprintf(stderr, "%s: child is ready!\n", __func__);
		exit(0);
	}

	if (setsid() == -1)
		return (-1);

	(void)chdir("/");
	if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
		(void)dup2(fd, STDIN_FILENO);
		(void)dup2(fd, STDOUT_FILENO);
		(void)dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			(void)close(fd);
	}
	return (getppid());
}

/*
 * We get a SIGALRM from the child when it's running and finished doing it's
 * fsync()'s or O_SYNC writes for all the boot messages.
 *
 * We also get a signal from the kernel if the timer expires, so check to
 * see what happened.
 */
static void
wait_timedout(int sig __unused)
{
	int left;
	left = alarm(0);
	signal(SIGALRM, SIG_DFL);
	if (left == 0)
		errx(1, "timed out waiting for child");
	else
		_exit(0);
}
