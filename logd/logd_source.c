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
#include "logd_source.h"
#include "logd_msg.h"

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
	char buf[1024];	/* XXX ew static */
	struct logd_source *ls = arg;
	int r;

	r = read(fd, buf, sizeof(buf));

	/* Don't fail temporary errors */
	if ((r < 0 && errno_sockio_fatal(errno)) || r == 0) {
		fprintf(stderr, "%s: FD %d; failed; errno=%d\n",
		    __func__,
		    fd,
		    errno);

		event_del(ls->read_ev);

		/* XXX TODO: notify owner */

		return;
	}

	fprintf(stderr, "%s: called; r=%d\n", __func__, r);

	/* XXX TODO: do something with the data */
	buf[r] = 0;
	fprintf(stderr, "  %s\n", buf);
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
	ls->fd = fd;
	ls->cb_read = cb_read;
	ls->cbdata = cbdata;
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
