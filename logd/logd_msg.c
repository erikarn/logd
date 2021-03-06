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

	if (lm->msg_src_name != NULL)
		free(lm->msg_src_name);

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

/*
 * This is the daemon/process name, not the source
 * host name.  Yes, I may want to change that later.
 */
static const char *
logd_msg_get_src_name(struct logd_msg *m)
{
	if (m->msg_src_name == NULL)
		return ("unknown");
	return (m->msg_src_name);
}

int
logd_msg_print(FILE *fp, struct logd_msg *m)
{

	fprintf(fp, "%s: called; m=%p; (%d) (%d:%s/%d:%s) (%s) msg=%.*s\n",
	    __func__,
	    m,
	    m->msg_orig_prifac,
	    m->msg_facility,
	    logd_msg_get_syslog_facility(m),
	    m->msg_priority,
	    logd_msg_get_syslog_priority(m),
	    logd_msg_get_src_name(m),
	    logd_buf_get_len(&m->buf),
	    logd_buf_get_buf(&m->buf));

	return (0);
}

int
logd_msg_set_src_name(struct logd_msg *lm, const char *str, int len)
{

	if (lm->msg_src_name != NULL)
		free(lm->msg_src_name);

	lm->msg_src_name = strndup(str, len);
	if (lm->msg_src_name == NULL) {
		warn("%s: strndup(%d)", __func__, len);
		return (-1);
	}

	return (0);
}

struct logd_msg *
logd_msg_dup(const struct logd_msg *m)
{
	struct logd_msg *mn;

	mn = logd_msg_create(m->buf.size);
	if (mn == NULL) {
		return (NULL);
	}

	/* Duplicate content */
	logd_msg_set_str(mn, m->buf.buf, m->buf.len);
	mn->ts_recv = m->ts_recv;
	mn->ts_msg = m->ts_msg;
	mn->msg_orig_prifac = m->msg_orig_prifac;
	mn->msg_priority = m->msg_priority;
	mn->msg_facility = m->msg_facility;
	if (m->msg_src_name)
		mn->msg_src_name = strdup(m->msg_src_name);

	return (mn);
}
