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
logd_collection_create(void)
{
	struct logd_collection *lc;

	lc = calloc(1, sizeof(*lc));
	if (lc == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}
	TAILQ_INIT(&lc->entries);

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

struct logd_collection_entry *
logd_collection_entry_add(struct logd_collection *lc, struct logd_source *ls)
{
	struct logd_collection_entry *le;

	le = logd_collection_entry_alloc();
	if (le == NULL) {
		return (NULL);
	}

	le->parent = lc;
	le->filt = NULL;
	le->src = ls;

	/* Now we're the owner, so channel reads/writes to us */

	TAILQ_INSERT_TAIL(&lc->entries, le, node);

	return (le);
}
