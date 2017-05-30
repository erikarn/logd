#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <time.h>

#include <sys/queue.h>

#include "logd_buf.h"
#include "logd_msg.h"
#include "logd_source.h"

#include "logd_source_list.h"


struct logd_source_list *
logd_source_list_new(void)
{
	struct logd_source_list *sl;

	sl = calloc(1, sizeof(*sl));
	if (sl == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}
	TAILQ_INIT(&sl->list);
	return (sl);
}

int
logd_source_list_add(struct logd_source_list *sl, struct logd_source *ls)
{
	/* XXX TODO Set owner cb */
	TAILQ_INSERT_TAIL(&sl->list, ls, node);

	return (0);
}

int
logd_source_list_remove(struct logd_source_list *sl, struct logd_source *ls)
{

	/* XXX TODO Clear owner cb */
	TAILQ_REMOVE(&sl->list, ls, node);

	return (0);
}

void logd_source_list_set_owner_callbacks(struct logd_source_list *sl,
	    logd_source_list_logmsg_read_cb *logmsg_cb,
	    logd_source_list_error_cb *error_cb,
	    void *cbdata)
{

	sl->owner_cb.logmsg_cb = logmsg_cb;
	sl->owner_cb.error_cb = error_cb;
	sl->owner_cb.cbdata = cbdata;
}

void
logd_source_list_flush(struct logd_source_list *sl)
{

}

void
logd_source_list_close(struct logd_source_list *sl)
{

}

void
logd_source_list_open(struct logd_source_list *sl)
{

}

void
logd_source_list_free(struct logd_source_list *sl)
{

}

void
logd_source_list_write(struct logd_source_list *sl,
    struct logd_msg *lm)
{

}

void
logd_source_list_init(void)
{

}

void
logd_source_list_shutdown(void)
{

}
