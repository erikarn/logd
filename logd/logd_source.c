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

/*
 * Implement a logd source/sink node that handles incoming, outgoing
 * log messages and some basic ownership about this particular node
 * on a list of nodes.
 */

static int
logd_source_flush_read_msgs(struct logd_source *ls)
{
	int ret = 0;
	struct logd_msg *m;

	while ((m = TAILQ_FIRST(&ls->read_msgs)) != NULL) {
		TAILQ_REMOVE(&ls->read_msgs, m, node);
		logd_msg_free(m);
	}

	return (ret);
}

/* XXX TODO: unify/abstract with above */
static int
logd_source_flush_write_msgs(struct logd_source *ls)
{
	int ret = 0;
#if 0
	struct logd_msg *m;

	while ((m = TAILQ_FIRST(&ls->write_msgs)) != NULL) {
		TAILQ_REMOVE(&ls->write_msgs, m, node);
		logd_msg_free(m);
	}
#endif

	return (ret);
}

int
logd_source_add_read_msg(struct logd_source *ls, struct logd_msg *m)
{

	TAILQ_INSERT_TAIL(&ls->read_msgs, m, node);
	return (0);
}

/*
 * Handle an error whilst doing read IO.
 *
 * This notifies the owner there was an error and it's up to the owner
 * to determine what to do.
 */
void
logd_source_read_error(struct logd_source *ls, int errcode, int xerror)
{

	ls->owner_cb.cb_error(ls, ls->owner_cb.cbdata, errcode);
}

/*
 * Child method: start read IO.
 */
void
logd_source_read_start(struct logd_source *ls)
{

	fprintf(stderr, "%s: TODO: actually implement\n", __func__);
}

/*
 * Child method: stop read IO.
 */
void
logd_source_read_stop(struct logd_source *ls)
{

	fprintf(stderr, "%s: TODO: actually implement\n", __func__);
}

void
logd_source_send_up_readmsgs(struct logd_source *ls)
{
	struct logd_msg *m;

	/*
	 * XXX TODO: defer this?
	 *
	 * Whilst we have logd_msg entries, pass them up.
	 */
	while ((m = TAILQ_FIRST(&ls->read_msgs)) != NULL) {
		TAILQ_REMOVE(&ls->read_msgs, m, node);
		ls->owner_cb.cb_logmsg_read(ls, ls->owner_cb.cbdata, m);
		/* owner owns logmsg now */
	}
}

struct logd_source *
logd_source_create(struct event_base *eb)
{
	struct logd_source *ls;

	ls = calloc(1, sizeof(*ls));
	if (ls == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	ls->eb = eb;

	TAILQ_INIT(&ls->read_msgs);
#if 0
	TAILQ_INIT(&ls->write_msgs);
#endif

	return (ls);
}

void
logd_source_free(struct logd_source *ls)
{

	logd_source_close(ls);

	/* Tell the child we're going away */
	if (ls->child_cb.cb_free != NULL) {
		ls->child_cb.cb_free(ls, ls->child_cb.cbdata);
	}

	/* Iterate through, free queued sources */
	logd_source_flush_read_msgs(ls);
	logd_source_flush_write_msgs(ls);

	free(ls);
}

void
logd_source_set_child_callbacks(struct logd_source *ls,
    logd_source_read_cb *cb_read,
    logd_source_write_cb *cb_write,
    logd_source_error_cb *cb_error,
    logd_source_free_cb *cb_free,
    logd_source_open_cb *cb_open,
    logd_source_close_cb *cb_close,
    logd_source_sync_cb *cb_sync,
    logd_source_reopen_cb *cb_reopen,
    logd_source_flush_cb *cb_flush,
    void *cbdata)
{

	ls->child_cb.cb_read = cb_read;
	ls->child_cb.cb_write = cb_write;
	ls->child_cb.cb_error = cb_error;
	ls->child_cb.cb_free = cb_free;
	ls->child_cb.cb_open = cb_open;
	ls->child_cb.cb_close = cb_close;
	ls->child_cb.cb_reopen = cb_reopen;
	ls->child_cb.cb_sync = cb_sync;
	ls->child_cb.cb_flush = cb_flush;
	ls->child_cb.cbdata = cbdata;
}

void
logd_source_set_owner_callbacks(struct logd_source *ls,
    logd_source_logmsg_read_cb *cb_logmsg_read,
    logd_source_error_cb *cb_error,
    void *cbdata)
{

	ls->owner_cb.cb_logmsg_read = cb_logmsg_read;
	ls->owner_cb.cb_error = cb_error;
	ls->owner_cb.cbdata = cbdata;
}

void
logd_source_init(void)
{
}

void
logd_source_shutdown(void)
{
}

int
logd_source_open(struct logd_source *ls)
{
	int ret;

	/*
	 * Initial setup - this will allocate a filedescriptor
	 * as appropriate.
	 */
	ret = ls->child_cb.cb_open(ls, ls->child_cb.cbdata);
	if (ret != 0)
		return (ret);

	return (0);
}

int
logd_source_close(struct logd_source *ls)
{

	/* XXX sync/flush read events? */

	/* XXX sync write events? */

	/* Leave the closing to the child */
	return (ls->child_cb.cb_close(ls, ls->child_cb.cbdata));
}

int
logd_source_reopen(struct logd_source *ls)
{

	if (ls->child_cb.cb_reopen == NULL)
		return (-1);

	return (ls->child_cb.cb_reopen(ls, ls->child_cb.cbdata));
}

/*
 * Write to the destination log source.
 *
 * TODO: rate limiting, maximum queue depth, etc is a later thing.
 */
int
logd_source_write(struct logd_source *ls, struct logd_msg *m)
{

	if (ls->child_cb.cb_write == NULL)
		return (-1);

	return (ls->child_cb.cb_write(ls, ls->child_cb.cbdata, m));
}

int
logd_source_sync(struct logd_source *ls)
{
	if (ls->child_cb.cb_sync == NULL)
		return (-1);

	return (ls->child_cb.cb_sync(ls, ls->child_cb.cbdata));
}

int
logd_source_flush(struct logd_source *ls)
{
	if (ls->child_cb.cb_flush == NULL)
		return (-1);

	/* XXX TODO: flush read/write */
	return (ls->child_cb.cb_flush(ls, ls->child_cb.cbdata));
}
