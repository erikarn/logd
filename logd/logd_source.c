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
		event_del(ls->read_ev);
		/* XXX TODO: notify owner */
		return;
	}

	/* XXX hard-coded maxread size */
	r = logd_buf_read_append(&ls->rbuf, ls->fd, 1024);

	/* buf full */
	if (r == -2) {
		fprintf(stderr, "%s: FD %d; incoming buf full; need to handle\n",
		    __func__, ls->fd);
		event_del(ls->read_ev);
		/* XXX TODO: notify owner */
		return;
	}

	/* Don't fail temporary errors */
	if ((r == -1 && errno_sockio_fatal(errno)) || r == 0) {
		fprintf(stderr, "%s: FD %d; failed; r=%d, errno=%d\n",
		    __func__,
		    fd,
		    r,
		    errno);

		event_del(ls->read_ev);
		/* XXX TODO: notify owner */
		return;
	}

	fprintf(stderr, "%s: called; r=%d; rbuf.len=%d\n", __func__, r,
	    logd_buf_get_len(&ls->rbuf));

	/* Loop over until we run out of data */
	while (logd_buf_get_len(&ls->rbuf) > 0) {
		/* Yes, reuse r */
		r = ls->child_cb.cb_read(ls, ls->child_cb.cbdata);

		/* Error */
		if (r < 0) {
			/* XXX TODO: notify owner */
			event_del(ls->read_ev);
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
logd_source_create(int fd, struct event_base *eb, logd_source_read_cb *cb_read,
    void *cbdata)
{
	struct logd_source *ls;

	ls = calloc(1, sizeof(*ls));
	if (ls == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	/* Yeah, should have an abstraction */
	if (logd_buf_init(&ls->rbuf, 1024) < 0) {
		free(ls);
		return (NULL);
	}
	ls->rbuf.size = 1024;

	/* Rest of the state */
	ls->fd = fd;
	ls->eb = eb;

	/* Child state */
	ls->child_cb.cb_read = cb_read;
	ls->child_cb.cbdata = cbdata;

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

	logd_buf_done(&ls->rbuf);

	/* libevent teardown */
	event_del(ls->read_ev);
	event_free(ls->read_ev);

	free(ls);
}

void
logd_source_init(void)
{
}

void
logd_source_shutdown(void)
{
}
