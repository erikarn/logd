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
		logd_buf_free(m);
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
		logd_buf_free(m);
	}
	ls->num_write_msgs = 0;

	return (ret);
}

/*
 * Handle an error.
 *
 * This stops subsequent read/write IO, notifies the owner that there's an
 * error and the owner gets to determine what they'll do.
 */
static void
logd_socket_dgram_io_error(struct logd_socket_dgram_io *ls, int errcode,
	    int xerror)
{

	ls->is_running = 0;
	event_del(ls->read_sock_ev);
	event_del(ls->read_sched_ev);
	ls->owner_cb.cb_error(ls, ls->owner_cb.cbdata,
	    errcode, xerror);
}

/*
 * Child method: start read IO.
 */
void
logd_socket_dgram_io_read_start(struct logd_socket_dgram_io *ls)
{

	ls->is_running = 1;
	event_add(ls->read_sock_ev, NULL);
}

/*
 * Child method: stop read IO
 */
void
logd_socket_dgram_io_read_stop(struct logd_socket_dgram_io *ls)
{

	ls->is_running = 0;
	event_del(ls->read_sock_ev);
}

static void
logd_socket_dgram_io_sched_readcb(struct logd_socket_dgram_io *ls)
{

	if (ls->is_running == 0)
		return;

	event_add(ls->read_sched_ev, NULL);
}

static void
logd_socket_dgram_io_read_evcb(evutil_socket_t fd, short what, void *arg)
{
	struct logd_socket_dgram_io *ls = arg;
	int r;
	struct logd_buf *m;

	/*
	 * Loop over, consuming buffers until we hit would-block or the
	 * queue is full
	 */
	while (ls->num_read_msgs < ls->max_read_msgs) {
		m = logd_buf_alloc(ls->read_buf_size);
		if (m == NULL) {
			/* Stop IO; error to owner */
			logd_socket_dgram_io_error(ls,
			    LOGD_SOCK_DGRAM_IO_ERR_SOCKET_READ_ALLOC,
			    0);
			break;
		}

		/* Read */
		r = recv(ls->fd, logd_buf_get_bufp(m),
		    logd_buf_get_size(m), 0);

		/* Handle errors */
		if (r == -1 && errno_sockio_fatal(errno)) {
			/* Fatal socket error */
			logd_buf_free(m);
			logd_socket_dgram_io_error(ls,
			    LOGD_SOCK_DGRAM_IO_ERR_SOCKET_READ,
			    errno);
			break;
		} else if (r == -1) {
			logd_buf_free(m);
			/* Non-fatal socket error */
			break;
		}

		/* Add */
		TAILQ_INSERT_TAIL(&ls->read_msgs, m, node);
		ls->num_read_msgs++;
	}

	/*
	 * Stop further IO if we've hit the IO limit; wait until we've
	 * pushed IO up to the owner.
	 */
	if (ls->num_read_msgs >= ls->num_read_msgs)
		logd_socket_dgram_io_read_stop(ls);

	/* If there are anything in the read buffer then call the owner */
	if (ls->num_read_msgs > 0)
		logd_socket_dgram_io_sched_readcb(ls);
}

/*
 * Process buffers on the read queue; send up to owner.
 */
static void
logd_socket_dgram_io_read_sched_evcb(evutil_socket_t fd, short what, void *arg)
{
	struct logd_socket_dgram_io *ls = arg;
	struct logd_buf *m;

	while ((m = TAILQ_FIRST(&ls->write_msgs)) != NULL) {
		TAILQ_REMOVE(&ls->write_msgs, m, node);
		ls->num_write_msgs--;

		/* Hand to the owner; it's theirs now no matter what */
		if (ls->owner_cb.cb_read(ls, ls->owner_cb.cbdata, m) < 0) {
			break;
		}
	}
}

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
	ls->read_buf_size = 2048;

	evutil_make_socket_nonblocking(fd);

	TAILQ_INIT(&ls->read_msgs);
	TAILQ_INIT(&ls->write_msgs);

	/* Setup/start events */
	ls->read_sock_ev = event_new(ls->eb, ls->fd, EV_READ | EV_PERSIST,
	    logd_socket_dgram_io_read_evcb, ls);
	ls->read_sched_ev = event_new(ls->eb, -1, 0,
	    logd_socket_dgram_io_read_sched_evcb, ls);

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
