#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libutil.h>
#include <strings.h>
#include <string.h>
#include <err.h>

#define SYSLOG_NAMES    1
#include <sys/syslog.h>

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

	m->msg_orig_prifac = -1;
	m->msg_priority = -1;
	m->msg_facility = -1;

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

static const char *
logd_msg_get_syslog_facility(struct logd_msg *m)
{
	const CODE *c;

	if (m->msg_facility == -1)
		return ("none");

	for (c = facilitynames; c->c_name; c++) {
		/*
		 * Shifted because the facility levels in facilitynames
		 * are shifted..
		 */
		if (c->c_val == (m->msg_facility << 3))
			return (c->c_name);
	}

	return ("unknown");
}

static const char *
logd_msg_get_syslog_priority(struct logd_msg *m)
{
	const CODE *c;

	if (m->msg_priority == -1)
		return ("none");
	for (c = prioritynames; c->c_name; c++) {
		if (c->c_val == m->msg_priority)
			return (c->c_name);
	}

	return ("unknown");
}

int
logd_msg_print(FILE *fp, struct logd_msg *m)
{

	fprintf(fp, "%s: called; m=%p; (%d) (%d:%s/%d:%s) msg=%.*s\n",
	    __func__,
	    m,
	    m->msg_orig_prifac,
	    m->msg_facility,
	    logd_msg_get_syslog_facility(m),
	    m->msg_priority,
	    logd_msg_get_syslog_priority(m),
	    logd_buf_get_len(&m->buf),
	    logd_buf_get_buf(&m->buf));

	return (0);
}
