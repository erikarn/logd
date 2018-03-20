
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
#include "logd_filter.h"
#include "logd_filter_true.h"

/*
 * Always return true (don't match.)
 */
static int
logd_filter_true_check_fast(struct logd_filter *lf, void *arg,
    struct logd_source *src, struct logd_source *dst,
    struct logd_msg *m)
{

	return (1);
}

static int
logd_filter_true_free(struct logd_filter *lf, void *arg)
{

	return (0);
}

struct logd_filter *
logd_filter_true_create(struct event_base *eb)
{
	struct logd_filter *lf;

	lf = logd_filter_create(eb);
	if (lf == NULL)
		return (NULL);

	/* Override methods */
	lf->child_cb.filter_check_fast_cb = logd_filter_true_check_fast;
	lf->child_cb.filter_free_cb = logd_filter_true_free;
	lf->child_cb.cbdata = NULL;

	return (lf);
}
