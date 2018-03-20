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
#include "logd_collection.h"

/*
 * This is a collection of logd sources/sinks.
 *
 * Each sink/source is "owned" by this collection; any messages received
 * will be sent through to all the other sources/sinks after a filter is
 * called to determine whether to send them.
 */
struct logd_collection *
logd_collection_create(struct event_base *eb)
{
	struct logd_collection *lc;

	lc = calloc(1, sizeof(*lc));
	if (lc == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}
	TAILQ_INIT(&lc->entries);
	lc->eb = eb;

	return (lc);
}

static struct logd_collection_entry *
logd_collection_entry_alloc(void)
{
	struct logd_collection_entry *le;

	le = calloc(1, sizeof(*le));
	if (le == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}
	return (le);
}

static int
logd_collection_source_read_cb(struct logd_source *ls, void *arg,
    struct logd_msg *m)
{
	struct logd_collection_entry *le = arg;
	struct logd_collection *lc = le->parent;
	struct logd_collection_entry *ln, *lnn;

	logd_msg_print(stderr, m);

	/*
	 * Walk the collection list, and call the filter routine on each
	 * to see if we should send this to others.
	 */
	TAILQ_FOREACH_SAFE(ln, &lc->entries, node, lnn) {
		struct logd_msg *mn;
		/* Don't send to self */
		if (ln->src == ls)
			continue;

		/* If there's no filter set, then default deny */
		if (lc->filter == NULL)
			continue;

		/* Do a fast check */
		if (logd_filter_check_fast(lc->filter,
		    ls, ln->src, m) != 1)
			continue;

		/* Ok, relaying - call xmit method */
		mn = logd_msg_dup(m);
		/* XXX TODO: counter */
		if (mn == NULL)
			continue;
		if (logd_source_write(ln->src, mn) < 0) {
			/* XXX TODO: counter */
			logd_msg_free(mn);
			continue;
		}
	}

	/*
	 * And now we're done; free the original log message.
	 */
	logd_msg_free(m);
	return (0);
}

static int
logd_collection_source_err_cb(struct logd_source *ls, void *arg, int err)
{

	fprintf(stderr, "%s: called; err=%d\n", __func__, err);
	return (0);
}


struct logd_collection_entry *
logd_collection_add(struct logd_collection *lc, struct logd_source *ls)
{
	struct logd_collection_entry *le;

	le = logd_collection_entry_alloc();
	if (le == NULL) {
		return (NULL);
	}

	le->parent = lc;
	/* TBD: no filter for now */
	le->filt = NULL;
	le->src = ls;

	/* Now we're the owner, so channel reads/writes to us */
	logd_source_set_owner_callbacks(ls, logd_collection_source_read_cb,
	    logd_collection_source_err_cb, le);

	TAILQ_INSERT_TAIL(&lc->entries, le, node);

	return (le);
}

void
logd_collection_free(struct logd_collection *lc)
{

	printf("%s: XXX TODO!\n", __func__);

	/* go through the entries and free them */

	/* free the filter if it exists */
	logd_filter_free(lc->filter);

	/* done */
}

int
logd_collection_assign_filter(struct logd_collection *lc,
    struct logd_filter *lf)
{

	/* If there's a previous filter then free it first */
	if (lc->filter != NULL) {
		logd_filter_free(lc->filter);
		lc->filter = NULL;
	}

	lc->filter = lf;

	return (0);
}
