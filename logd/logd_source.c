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

static int
logd_source_flush_read_msgs(struct logd_source *ls)
{
	int ret = 0;
	struct logd_msg *m;

	while ((m = TAILQ_FIRST(&ls->read_msgs)) != NULL) {
		TAILQ_REMOVE(&ls->read_msgs, m, node);
		logd_msg_free(m);
	}

	return (ret);
}

/* XXX TODO: unify/abstract with above */
static int
logd_source_flush_write_msgs(struct logd_source *ls)
{
	int ret = 0;
	struct logd_msg *m;

	while ((m = TAILQ_FIRST(&ls->write_msgs)) != NULL) {
		TAILQ_REMOVE(&ls->write_msgs, m, node);
		logd_msg_free(m);
	}

	return (ret);
}

int
logd_source_add_read_msg(struct logd_source *ls, struct logd_msg *m)
{

	TAILQ_INSERT_TAIL(&ls->read_msgs, m, node);
	return (0);
}

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
	if (ls->fd == -1) {
		fprintf(stderr, "%s: called whilst it's closed\n", __func__);
		return;
	}

	event_add(ls->read_ev, NULL);
}

/*
 * Child method: stop read IO.
 */
void
logd_source_read_stop(struct logd_source *ls)
{
	if (ls->fd == -1) {
		fprintf(stderr, "%s: called whilst it's closed\n", __func__);
		return;
	}

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
	struct logd_msg *m;
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

#if 0
	fprintf(stderr, "%s: called; r=%d; rbuf.len=%d\n", __func__, r,
	    logd_buf_get_len(&ls->rbuf));
#endif

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

	/*
	 * XXX TODO: defer this?
	 *
	 * Whilst we have logd_msg entries, pass them up.
	 */
	while ((m = TAILQ_FIRST(&ls->read_msgs)) != NULL) {
		TAILQ_REMOVE(&ls->read_msgs, m, node);
		ls->owner_cb.cb_logmsg_read(ls, ls->owner_cb.cbdata, m);
		/* owner owns logmsg now */
	}

}

struct logd_source *
logd_source_create(struct event_base *eb)
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
	ls->fd = -1;
	ls->eb = eb;

	TAILQ_INIT(&ls->read_msgs);
	TAILQ_INIT(&ls->write_msgs);

	return (ls);
}

void
logd_source_free(struct logd_source *ls)
{

	/* Call close() if it hasn't been called yet */
	if (ls->fd == -1) {
		logd_source_close(ls);
	}

	/* Tell the child we're going away */
	if (ls->child_cb.cb_free != NULL) {
		ls->child_cb.cb_free(ls, ls->child_cb.cbdata);
	}

	logd_buf_done(&ls->rbuf);

	/* Iterate through, free queued sources */
	logd_source_flush_read_msgs(ls);
	logd_source_flush_write_msgs(ls);

	free(ls);
}

void
logd_source_set_child_callbacks(struct logd_source *ls,
    logd_source_read_cb *cb_read,
    logd_source_write_cb *cb_write,
    logd_source_error_cb *cb_error,
    logd_source_free_cb *cb_free,
    logd_source_open_cb *cb_open,
    logd_source_close_cb *cb_close,
    logd_source_sync_cb *cb_sync,
    logd_source_reopen_cb *cb_reopen,
    logd_source_flush_cb *cb_flush,
    void *cbdata)
{

	ls->child_cb.cb_read = cb_read;
	ls->child_cb.cb_write = cb_write;
	ls->child_cb.cb_error = cb_error;
	ls->child_cb.cb_free = cb_free;
	ls->child_cb.cb_open = cb_open;
	ls->child_cb.cb_close = cb_close;
	ls->child_cb.cb_reopen = cb_reopen;
	ls->child_cb.cb_sync = cb_sync;
	ls->child_cb.cb_flush = cb_flush;
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

int
logd_source_open(struct logd_source *ls)
{
	int ret;

	/*
	 * Initial setup - this will allocate a filedescriptor
	 * as appropriate.
	 */
	ret = ls->child_cb.cb_open(ls, ls->child_cb.cbdata);
	if (ret != 0)
		return (ret);

	/*
	 * Create IO events now that the FD is ready.
	 */
	ls->read_ev = event_new(ls->eb, ls->fd, EV_READ | EV_PERSIST,
	    logd_source_read_evcb, ls);

	/* Start reading - maybe this should be a method */
	event_add(ls->read_ev, NULL);

	return (0);
}

int
logd_source_close(struct logd_source *ls)
{

	/* XXX flush read events? */

	/* XXX flush write events? */

	/* Close the IO events */
	if (ls->read_ev != NULL) {
		event_del(ls->read_ev);
		event_free(ls->read_ev);
	}

	/* And the filedescriptor */
	if (ls->fd != -1) {
		close(ls->fd);
		ls->fd = -1;
	}

	return (ls->child_cb.cb_close(ls, ls->child_cb.cbdata));
}

int
logd_source_reopen(struct logd_source *ls)
{

	return (ls->child_cb.cb_reopen(ls, ls->child_cb.cbdata));
}

/*
 * Write to the destination log source.
 *
 * This adds it to the queue and calls the child method to start
 * writing.
 *
 * TODO: rate limiting, maximum queue depth, etc is a later thing.
 */
int
logd_source_write(struct logd_source *ls, struct logd_msg *m)
{

	TAILQ_INSERT_TAIL(&ls->write_msgs, m, node);

	return (ls->child_cb.cb_write(ls, ls->child_cb.cbdata));
}

int
logd_source_sync(struct logd_source *ls)
{

	return (ls->child_cb.cb_flush(ls, ls->child_cb.cbdata));
}

int
logd_source_flush(struct logd_source *ls)
{

	/* XXX TODO: flush read/write */
	return (ls->child_cb.cb_flush(ls, ls->child_cb.cbdata));
}
