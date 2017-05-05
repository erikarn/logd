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

#include "logd_source.h"

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

	/* libevent setup */

	return (ls);
}

void
logd_source_free(struct logd_source *ls)
{

	/* libevent teardown */

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
