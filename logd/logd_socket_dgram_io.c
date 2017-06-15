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

#include "logd_buf.h"
#include "logd_socket_dgram_io.h"

/*
 * Implement a bufferevent style reader abstraction for UDP/dgrams.
 *
 * For now it simply will do send() / recv() as it assumes it's been
 * given a valid connected socket.  Later iterations should handle
 * unconnected sockets.
 */

static int
logd_socket_dgram_io_flush_read_msgs(struct logd_socket_dgram_io *ls)
{
	int ret = 0;
	struct logd_buf *m;

	while ((m = TAILQ_FIRST(&ls->read_msgs)) != NULL) {
		TAILQ_REMOVE(&ls->read_msgs, m, node);
		logd_buf_done(m);
	}
	ls->num_read_msgs = 0;

	return (ret);
}

/* XXX TODO: unify/abstract with above */
static int
logd_socket_dgram_io_flush_write_msgs(struct logd_socket_dgram_io *ls)
{
	int ret = 0;
	struct logd_buf *m;

	while ((m = TAILQ_FIRST(&ls->write_msgs)) != NULL) {
		TAILQ_REMOVE(&ls->write_msgs, m, node);
		logd_buf_done(m);
	}
	ls->num_write_msgs = 0;

	return (ret);
}

#if 0
/*
 * Handle an error whilst doing read IO.
 *
 * This stops subsequent read IO, notifies the owner that there's an error,
 * and the owner gets to determine what they'll do.
 */
static void
logd_socket_dgram_io_read_error(struct logd_socket_dgram_io *ls, int errcode, int xerror)
{

	event_del(ls->read_sock_ev);
	event_del(ls->read_sched_ev);
	ls->owner_cb.cb_error(ls, ls->owner_cb.cbdata,
	    errcode);
}
#endif

/*
 * Child method: start read IO.
 */
void
logd_socket_dgram_io_read_start(struct logd_socket_dgram_io *ls)
{

	event_add(ls->read_sock_ev, NULL);
}

/*
 * Child method: stop read IO.
 */
void
logd_socket_dgram_io_read_stop(struct logd_socket_dgram_io *ls)
{

	event_del(ls->read_sock_ev);
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
#if 0
static void
logd_socket_dgram_io_read_evcb(evutil_socket_t fd, short what, void *arg)
{

}
#endif

struct logd_socket_dgram_io *
logd_socket_dgram_io_create(struct event_base *eb, int fd)
{
	struct logd_socket_dgram_io *ls;

	ls = calloc(1, sizeof(*ls));
	if (ls == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	/* Rest of the state */
	ls->fd = fd;
	ls->eb = eb;

	TAILQ_INIT(&ls->read_msgs);
	TAILQ_INIT(&ls->write_msgs);

	/* Setup/start events */

	return (ls);
}

void
logd_socket_dgram_io_free(struct logd_socket_dgram_io *ls)
{

	/* XXX TODO: close socket, delete events */

	/* Iterate through, free queued sources */
	logd_socket_dgram_io_flush_read_msgs(ls);
	logd_socket_dgram_io_flush_write_msgs(ls);

	free(ls);
}

void
logd_socket_dgram_io_set_owner_callbacks(struct logd_socket_dgram_io *ls,
    logd_socket_dgram_io_read_cb *cb_read,
    logd_socket_dgram_io_error_cb *cb_error,
    logd_socket_dgram_io_write_cb *cb_write,
    void *cbdata)
{

	ls->owner_cb.cb_read = cb_read;
	ls->owner_cb.cb_write = cb_write;
	ls->owner_cb.cb_error = cb_error;
	ls->owner_cb.cbdata = cbdata;
}

void
logd_socket_dgram_io_init(void)
{
}

void
logd_socket_dgram_io_shutdown(void)
{
}

/*
 * Queue a write; schedule the write callback.
 *
 * TODO: rate limiting, maximum queue depth, etc is a later thing.
 */
int
logd_socket_dgram_io_write(struct logd_socket_dgram_io *ls, struct logd_buf *m)
{

	fprintf(stderr, "%s: TODO\n", __func__);
	return (-1);
}

int
logd_socket_dgram_io_sync(struct logd_socket_dgram_io *ls)
{

	fprintf(stderr, "%s: TODO\n", __func__);
	return (-1);
}

int
logd_socket_dgram_io_flush(struct logd_socket_dgram_io *ls)
{

	fprintf(stderr, "%s: TODO\n", __func__);
	return (-1);
}
