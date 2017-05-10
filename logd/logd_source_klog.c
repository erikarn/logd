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
#if 0
	char *p, *line;

	/*
	 * Loop over, consuming lines until we can't find
	 * any more complete logging lines.
	 */

                for (p = line; (q = strchr(p, '\n')) != NULL; p = q + 1) {
                        *q = '\0';
                        printsys(p);
                }
#endif


	return (0);
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
