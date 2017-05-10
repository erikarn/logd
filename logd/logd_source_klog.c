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
#include "logd_msg.h"
#include "logd_source.h"
#include "logd_source_klog.h"

/*
 * Implement a simple klog source.  This consumes messages
 * and add timestamping, logging level, etc.
 */

/*
 * Loop over and attempt to consume a message.
 * Note that in syslog format things are terminated by a \n, and we
 * may end up reading a partial buffer - so yes we have to also handle
 * that.
 *
 * If the read buffer is full then we also have to handle that and
 * potentially flush the buffer.
 */
static int
logd_source_klog_read_cb(struct logd_source *ls, void *arg)
{
	struct logd_msg *m;
	const char *p, *q;
	int l, ret = 0;

	while (logd_buf_get_len(&ls->rbuf) != 0) {
		/* start of line */
		p = logd_buf_get_buf(&ls->rbuf);

		/* look for a \n */
		q = memchr(p, '\n', logd_buf_get_len(&ls->rbuf));
		if (q == NULL) {
			/* We don't have a complete line, so bail */
			break;
		}

		/* we have a line, so create a buf for it, and consume */
		l = q - p + 1;
		if (l < 0) {
			/* Error, shouldn't happen */
			return (-1);
		}

		/* Now, we have a string of len 'l', so populate */
		m = logd_msg_create();
		if (m == NULL)
			return (-1);
		logd_msg_set_str(m, p, l);

		/* XXX rest of the parsing, etc to do here */

		/* Consume the buffer - XXX TODO: l or l+1? */
		logd_buf_consume(&ls->rbuf, NULL, l);

		/* Add the message to the source */
		if (logd_source_add_read_msg(ls, m) < 0) {
			/*
			 * XXX TODO should notify caller that we couldn't add
			 * the message
			 */
			logd_msg_free(m);
			continue;
		}

		ret++;
	}

	return (ret);
}

static int
logd_source_klog_error_cb(struct logd_source *ls, void *arg, int error)
{

	fprintf(stderr, "%s: called; error=%d\n", __func__, error);

	return (0);
}

static int
logd_source_klog_close_cb(struct logd_source *ls, void *arG)
{

	fprintf(stderr, "%s: called\n", __func__);
	/* Do teardown */
	return (0);
}

struct logd_source *
logd_source_klog_create(int fd, struct event_base *eb)
{
	struct logd_source *ls;

	ls = logd_source_create(fd, eb);
	if (ls == NULL)
		return (NULL);

	/* Do other setup */
	logd_source_set_child_callbacks(ls,
	    logd_source_klog_read_cb,
	    logd_source_klog_error_cb,
	    logd_source_klog_close_cb,
	    NULL);

	return (ls);
}

void
logd_source_klog_init(void)
{

}

void
logd_source_klog_shutdown(void)
{

}
