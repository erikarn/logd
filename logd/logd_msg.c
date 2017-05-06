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


struct logd_msg *
logd_msg_create(void)
{
	struct logd_msg *m;

	m = calloc(1, sizeof(*m));
	if (m == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}
	return (m);
}

void
logd_msg_free(struct logd_msg *lm)
{
	if (lm->buf != NULL)
		free(lm->buf);
	free(lm);
}

void
logd_set_timestamps(struct logd_msg *lm, struct timespec *tr,
    struct timespec *tm)
{

	if (tr != NULL)
		lm->ts_recv = *tr;
	if (tm != NULL)
		lm->ts_msg = *tm;
}

int
logd_msg_set_str(struct logd_msg *lm, const char *buf, int len)
{

	/* Free existing buf */
	if (lm->buf != NULL)
		free(lm->buf);

	/* Allocate new buffer */
	lm->buf = malloc(len + 1);
	if (lm->buf == NULL) {
		warn("%s: malloc(%d)", __func__, len + 1);
		return (-1);
	}

	/* Copy; NUL terminate */
	memcpy(lm->buf, buf, len);
	lm->buf[len] = '\0';
	lm->len = len;
	lm->size = len + 1;

	return (0);
}
