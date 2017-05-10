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

#include "config.h"

#include "logd_util.h"
#include "logd_buf.h"
#include "logd_msg.h"
#include "logd_source.h"

/*
 * Implement a bufferevent style reader abstraction, which will
 * handle consuming random log sources and (eventually) pushing back up
 * status (eg close/error) and logd_msg's.
 *
 * This will eventually grow to (optionally) handle writing logd_msg's
 * to things - so yeah, this may become both a source/sink, and some
 * bits only do one.
 */

/*
 * Handle an error whilst doing read IO.
 *
 * This stops subsequent read IO, notifies the child that there's an error,
 * and the child gets to determine what they'll do.
 */
static void
logd_source_read_error(struct logd_source *ls, int errcode, int xerror)
{

	event_del(ls->read_ev);
	ls->child_cb.cb_error(ls, ls->child_cb.cbdata,
	    errcode);
}

/*
 * Child method: start read IO.
 */
void
logd_source_read_start(struct logd_source *ls)
{

	event_add(ls->read_ev, NULL);
}

/*
 * Child method: stop read IO.
 */
void
logd_source_read_stop(struct logd_source *ls)
{

	event_del(ls->read_ev);
}

/*
 * Read some data from the sender side.  Yes, this should really
 * just use bufferevents.
 *
 * syslogd consumes log lines, instead of binary data.
 *
 * This logd is aimed at eventually growing to also handle
 * binary TLVs,  the hope is to make this a bufferevent-y thing
 * where we read binary data, and call the read callback to
 * consume buffers until they say we're done (and there's potentially
 * a partial buffer), or there's an error.  Or, indeed, they may
 * decide they can't find anything and we hit our read buffer size -
 * which means that either we've been fed garbage (so we should
 * like, toss data) or close the connection.
 *
 * That'll come!
 */
static void
logd_source_read_evcb(evutil_socket_t fd, short what, void *arg)
{
	struct logd_source *ls = arg;
	int r;

	if (logd_buf_get_freespace(&ls->rbuf) == 0) {
		fprintf(stderr, "%s: FD %d; incoming buf full; need to handle\n",
		    __func__, ls->fd);
		/* notify child */
		logd_source_read_error(ls, LOGD_SOURCE_ERROR_READ_FULL, 0);
		return;
	}

	/* XXX hard-coded maxread size */
	r = logd_buf_read_append(&ls->rbuf, ls->fd, 1024);

	/* buf full */
	if (r == -2) {
		fprintf(stderr, "%s: FD %d; incoming buf full; need to handle\n",
		    __func__, ls->fd);
		/* notify child */
		logd_source_read_error(ls, LOGD_SOURCE_ERROR_READ_FULL, 0);
		return;
	}

	/* Don't fail temporary errors */
	if (r == -1 && errno_sockio_fatal(errno)) {
		fprintf(stderr, "%s: FD %d; failed; r=%d, errno=%d\n",
		    __func__,
		    fd,
		    r,
		    errno);
		/* notify child; they can notify owner */
		logd_source_read_error(ls, LOGD_SOURCE_ERROR_READ_ERROR, errno);
		return;
	}

	/* Not fatal socket errors - eg, EWOULDBLOCK */
	if (r == -1) {
		return;
	}

	if (r == 0) {
		/*
		 * notify child; they can tell the owner about EOF
		 * and/or re-open things as appropriate.
		 */
		logd_source_read_error(ls, LOGD_SOURCE_ERROR_READ_EOF, 0);
		return;
	}

	fprintf(stderr, "%s: called; r=%d; rbuf.len=%d\n", __func__, r,
	    logd_buf_get_len(&ls->rbuf));

	/*
	 * Loop over until we run out of data or error.
	 */
	while (logd_buf_get_len(&ls->rbuf) > 0) {
		/* Yes, reuse r */
		r = ls->child_cb.cb_read(ls, ls->child_cb.cbdata);

		/* Error */
		if (r < 0) {
			/* notify owner - they can decide what to do */
			ls->owner_cb.cb_error(ls, ls->owner_cb.cbdata,
			    LOGD_SOURCE_ERROR_READ_ERROR);
			break;
		}

		/* Didn't consume anything from this yet */
		if (r == 0) {
			break;
		}

		/* r > 0; we consumed some data */
	}
}

struct logd_source *
logd_source_create(int fd, struct event_base *eb)
{
	struct logd_source *ls;

	ls = calloc(1, sizeof(*ls));
	if (ls == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	/*
	 * Setup incoming buffer.
	 *
	 * Maybe this should be part of the child, not this!
	 */
	if (logd_buf_init(&ls->rbuf, 1024) < 0) {
		free(ls);
		return (NULL);
	}
	ls->rbuf.size = 1024;

	/* Rest of the state */
	ls->fd = fd;
	ls->eb = eb;

	/* libevent setup */
	ls->read_ev = event_new(ls->eb, ls->fd, EV_READ | EV_PERSIST,
	    logd_source_read_evcb, ls);

	/* Start reading - maybe this should be a method */
	event_add(ls->read_ev, NULL);

	return (ls);
}

void
logd_source_free(struct logd_source *ls)
{

	/* Tell the child we're going away */
	if (ls->child_cb.cb_close != NULL) {
		ls->child_cb.cb_close(ls, ls->child_cb.cbdata);
	}

	logd_buf_done(&ls->rbuf);

	/* libevent teardown */
	event_del(ls->read_ev);
	event_free(ls->read_ev);

	free(ls);
}

void
logd_source_set_child_callbacks(struct logd_source *ls,
    logd_source_read_cb *cb_read,
    logd_source_error_cb *cb_error,
    logd_source_close_cb *cb_close,
    void *cbdata)
{

	ls->child_cb.cb_read = cb_read;
	ls->child_cb.cb_error = cb_error;
	ls->child_cb.cb_close = cb_close;
	ls->child_cb.cbdata = cbdata;
}

void
logd_source_set_owner_callbacks(struct logd_source *ls,
    logd_source_logmsg_read_cb *cb_logmsg_read,
    logd_source_error_cb *cb_error,
    void *cbdata)
{

	ls->owner_cb.cb_logmsg_read = cb_logmsg_read;
	ls->owner_cb.cb_error = cb_error;
	ls->owner_cb.cbdata = cbdata;
}

void
logd_source_init(void)
{
}

void
logd_source_shutdown(void)
{
}
