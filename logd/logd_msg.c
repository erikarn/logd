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
logd_msg_create(int len)
{
	struct logd_msg *m;

	m = calloc(1, sizeof(*m));

	if (m == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	if (logd_buf_init(&m->buf, len) < 0) {
		free(m);
		return (NULL);
	}

	return (m);
}

void
logd_msg_free(struct logd_msg *lm)
{

	logd_buf_done(&lm->buf);
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
	int r;

	r = logd_buf_populate(&lm->buf, buf, len);
	if (r != len)
		return (-1);

	return (0);
}
