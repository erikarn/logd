
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

/*
 * Always return false (don't match.)
 */
static int
logd_filter_null_check_fast(struct logd_filter *lf, void *arg,
    struct logd_source *src, struct logd_source *dst,
    struct logd_msg *m)
{

	return (0);
}

static int
logd_filter_null_free(struct logd_filter *lf, void *arg)
{

	return (0);
}

/*
 * Allocate a default logd filter.  This is a blank template
 * who will always fail filter requests.
 */
struct logd_filter *
logd_filter_create(struct event_base *eb)
{
	struct logd_filter *lf;

	lf = calloc(1, sizeof(*lf));
	if (lf == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}
	lf->eb = eb;
	lf->child_cb.filter_check_fast_cb = logd_filter_null_check_fast;
	lf->child_cb.filter_free_cb = logd_filter_null_free;
	lf->child_cb.cbdata = NULL;

	return (lf);
}

int
logd_filter_free(struct logd_filter *lf)
{

	lf->child_cb.filter_free_cb(lf, lf->child_cb.cbdata);

	free(lf);
	return (0);
}

int
logd_filter_check_fast(struct logd_filter *lf, struct logd_source *src,
    struct logd_source *dst, struct logd_msg *m)
{

	return (lf->child_cb.filter_check_fast_cb(lf, lf->child_cb.cbdata,
	    src, dst, m));
}
