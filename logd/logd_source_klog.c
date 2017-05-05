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
#include "logd_source.h"

/*
 * Implement a simple klog source.  This consumes messages
 * and add timestamping, logging level, etc.
 */

static int
logd_source_klog_read_cb(struct logd_source *ls, void *arg)
{

	return (0);
}

struct logd_source *
logd_source_klog_create(int fd, struct event_base *eb)
{
	struct logd_source *ls;

	ls = logd_source_create(fd, eb, logd_source_klog_read_cb, NULL);
	if (ls == NULL)
		return (NULL);

	/* Do other setup */

	return (ls);
}

void
logd_source_klog_free(struct logd_source *ls)
{

	/* Do other teardown */
	logd_source_free(ls);
}

void
logd_source_klog_init(void)
{

}

void
logd_source_klog_shutdown(void)
{

}
