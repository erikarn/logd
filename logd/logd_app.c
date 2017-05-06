#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libutil.h>
#include <strings.h>
#include <string.h>
#include <err.h>

#include <sys/queue.h>

#include <event2/event.h>
//#include <event2/evsignal.h>

#include "config.h"

#include "logd_buf.h"
#include "logd_msg.h"
#include "logd_source.h"

#include "logd_app.h"

void
logd_app_init(struct logd_app *la)
{
	bzero(la, sizeof(*la));

	/* pidfile init */
	la->pidfile_path = strdup(LOGD_PID_FILE);

	/* Daemon settings init */
	la->do_background = 1;

	/* libevent state */
	la->eb = event_base_new();
}

void
logd_app_set_pidfile(struct logd_app *la, const char *pidfile)
{

	free(la->pidfile_path);
	la->pidfile_path = strdup(pidfile);
}

int
logd_app_open_pidfile(struct logd_app *la)
{

	la->pid_fh = pidfile_open(la->pidfile_path, 0600, &la->pid);
	if (la->pid_fh == NULL) {

		/* XXX this should be returned to the caller */
		if (errno == EEXIST)
			errx(1, "logd already running, pid %d\n", la->pid);

		warn("%s: pidfile_open", __func__);
		return (-1);
	}

	return (0);
}

int
logd_app_write_pidfile(struct logd_app *la)
{
	int ret;

	if (la->pid_fh == NULL)
		return (0);

	ret = pidfile_write(la->pid_fh);
	if (ret != 0) {
		warn("%s: pidfile_write", __func__);
		return (-1);
	}

	return (0);
}

int
logd_app_close_pidfile(struct logd_app *la)
{

	if (la->pid_fh == NULL)
		return (0);

	pidfile_close(la->pid_fh);
	la->pid_fh = NULL;

	return (0);
}

int
logd_app_remove_pidfile(struct logd_app *la)
{

	if (la->pid_fh == NULL)
		return (0);

	pidfile_remove(la->pid_fh);

	return (0);
}

int
logd_app_set_background(struct logd_app *la, int do_background)
{

	la->do_background = do_background;

	return (0);
}

int
logd_app_run(struct logd_app *la)
{

	/* XXX remember, occasionally this errors out, sigh */
	(void) event_base_loop(la->eb, EVLOOP_NO_EXIT_ON_EMPTY);

	return (0);
}
