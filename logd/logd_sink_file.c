#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libutil.h>
#include <strings.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <event2/event.h>

#include "config.h"

#include "logd_buf.h"
#include "logd_msg.h"
#include "logd_source.h"
#include "logd_sink_file.h"

#define	SINK_LOG_FD_TYPE_NONE	0
#define	SINK_LOG_FD_TYPE_FILE	1

/*
 * Implement a simple text log file sink.
 * This logs the given message to the given file.
 * It doesn't do any filtering; this is someone elses'
 * concern.
 */

struct logd_sink_file {
	int fd;
	char *path;
	int fd_type;
	int do_unlink;
};

/*
 * Always return error; we don't have any read handlers here.
 */
static int
logd_sink_file_read_cb(struct logd_source *ls, void *arg)
{

	fprintf(stderr, "%s: called\n", __func__);
	return (-1);
}

static int
logd_sink_file_error_cb(struct logd_source *ls, void *arg, int error)
{

	fprintf(stderr, "%s: called; error=%d\n", __func__, error);

	return (0);
}

static int
logd_sink_file_free_cb(struct logd_source *ls, void *arg)
{
	struct logd_sink_file *kl = arg;

	fprintf(stderr, "%s: called\n", __func__);
	/* Do teardown */

	free(kl->path);
	return (0);
}

static int
logd_sink_file_open_cb(struct logd_source *ls, void *arg)
{
	struct logd_sink_file *kl = arg;
	int fd;

	fprintf(stderr, "%s: called\n", __func__);

	switch (kl->fd_type) {
	case SINK_LOG_FD_TYPE_FILE:
		/* just a simple file */
		fd = open(kl->path, O_WRONLY | O_APPEND | O_CREAT);
		if (fd < 0) {
			warn("%s: open(%s)", __func__, kl->path);
			return (-1);
		}
		break;
	default:
		fprintf(stderr, "%s: unknown fd type (%d)\n", __func__,
		    kl->fd_type);
		return (-1);
	}

	/* XXX should be a method */
	ls->fd = fd;

	return (0);
}

static int
logd_sink_file_close_cb(struct logd_source *ls, void *arg)
{
	struct logd_sink_file *kl = arg;

	fprintf(stderr, "%s: called\n", __func__);

	/* Note: for now, main class has called close() already */
	if (kl->fd != -1) {
		close(kl->fd);
		/* XXX TODO method */
		kl->fd = -1;
	}

	if (kl->do_unlink) {
		unlink(kl->path);
	}
	return (0);
}

static int
logd_sink_file_write_cb(struct logd_source *ls, void *arg)
{

	fprintf(stderr, "%s: called\n", __func__);
	return (-1);
}

static int
logd_sink_file_reopen_cb(struct logd_source *ls, void *arg)
{

	fprintf(stderr, "%s: called\n", __func__);
	return (-1);
}

static int
logd_sink_file_sync_cb(struct logd_source *ls, void *arg)
{

	fprintf(stderr, "%s: called\n", __func__);
	return (-1);
}

static int
logd_sink_file_flush_cb(struct logd_source *ls, void *arg)
{

	fprintf(stderr, "%s: called\n", __func__);
	return (-1);
}

struct logd_source *
logd_sink_file_create_file(struct event_base *eb,
    const char *path)
{
	struct logd_source *ls;
	struct logd_sink_file *kl;

	kl = calloc(1, sizeof(*kl));
	if (kl == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	ls = logd_source_create(eb);
	if (ls == NULL) {
		free(kl);
		return (NULL);
	}

	/* Do other setup */
	logd_source_set_child_callbacks(ls,
	    logd_sink_file_read_cb,
	    logd_sink_file_write_cb,
	    logd_sink_file_error_cb,
	    logd_sink_file_free_cb,
	    logd_sink_file_open_cb,
	    logd_sink_file_close_cb,
	    logd_sink_file_sync_cb,
	    logd_sink_file_reopen_cb,
	    logd_sink_file_flush_cb,
	    kl);

	kl->path = strdup(path);
	kl->fd_type = SINK_LOG_FD_TYPE_FILE;

	return (ls);
}

void
logd_sink_file_init(void)
{

}

void
logd_sink_file_shutdown(void)
{

}
