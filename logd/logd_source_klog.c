#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libutil.h>
#include <strings.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <event2/event.h>

#include "config.h"

#include "logd_buf.h"
#include "logd_msg.h"
#include "logd_source.h"
#include "logd_source_klog.h"

#define	KLOG_FD_TYPE_NONE	0
#define	KLOG_FD_TYPE_FIFO	1
#define	KLOG_FD_TYPE_UDP_SOCKET	2
#define	KLOG_FD_TYPE_TCP_SOCKET	3
#define	KLOG_FD_TYPE_FILE	4
#define	KLOG_FD_TYPE_KLOG	5

/*
 * Implement a simple klog source.  This consumes messages
 * and add timestamping, logging level, etc.
 */

struct logd_source_klog {
	int fd;
	char *path;
	int is_readonly;
	int fd_type;
	int do_unlink;
};

/*
 * Parse the facility field of a klog/syslog message.
 * It's an integer value formatted with < and > around it.
 *
 * Returns 0 if it wasn't found, or >0 indiciated it was
 * found, complete with the facility and priority values
 * parsed out and the facility value stripped.
 */
static int
logd_source_klog_parse_facility(struct logd_msg *m)
{
	char numbuf[16];
	const char *b, *n;
	char *e;
	int i, f, len;

	if (logd_buf_get_len(&m->buf) < 2)
		return (0);

	b = logd_buf_get_buf(&m->buf);

	/* First < */
	if (b[0] != '<')
		return (0);

	/* Find > */
	n = memchr(b + 1, '>', logd_buf_get_len(&m->buf) - 1);
	if (n == NULL)
		return (0);

	/*
	 * n points to the first >;
	 * n - b + 1 is the length of the buffer to remove.
	 */
	len = (n - b) + 1;
	if (len < 2)
		return (0);

	/* Copy out string minus the < and > */
	memcpy(numbuf, b + 1, len - 2);
	numbuf[len - 2] = '\0';

	i = strtoul(numbuf, &e, 0);
	if (b == e)
		return (0);

#if 0
	fprintf(stderr, "%s: parsed %d (len %d)\n", __func__, i, len);
#endif

	m->msg_orig_prifac = i;
	m->msg_priority = i & 0x7;
	m->msg_facility = i >> 3;

	logd_buf_consume(&m->buf, NULL, len);

	/* Return the number of bytes consumed */
	/* XXX TODO: this is actually also really wrong, haven't updated len */
	return (len);
}

/*
 * Attempt to parse a date and process name from the "normal" BSD
 * formatted syslog message.
 *
 * syslogd.c will attempt to do parse things:
 *
 * + is the first 16 characters (MAXDATELEN) matches (badly) a timestamp
 *   format;
 * + then it trims whitespace
 * + then it looks for :, [, /, or ' ' as the process name delimiter;
 * + .. and that's the end of the program name.
 *
 * For now, don't modify the log message - just parse out the timestamp
 * and process name for logging.
 *
 * Returns 0 if it didn't parse anything out, > 0 for how many characters
 * in the beginning of the string that contained the timestamp + process name.
 *
 * XXX TODO:
 * + everything in this routine needs refactoring
 * + all the string/buffer stuff should be turned into logd_buf APIs
 * + return values are all wrong
 */
static int
logd_source_klog_parse_timestamp_procname(struct logd_msg *m)
{
#define	MAXDATELEN	16
#define	NAME_MAX	255
	int len = 0;
	int found_date = 0;
	const char *msg;
	int msglen;
	int i, j;

	msg = logd_buf_get_buf(&m->buf);
	msglen = logd_buf_get_len(&m->buf);

	/* Look for a date, syslog style */
	if (msglen < MAXDATELEN)
		return (0);

	found_date = 1;
	if (msg[3] != ' ' || msg[6] != ' ' ||
	    msg[9] != ':' || msg[12] != ':' || msg[15] != ' ') {
		found_date = 0;
	}

	/* If we found a date, skip over it */
	if (found_date == 1) {
		msg += MAXDATELEN;
		msglen -= MAXDATELEN;
	}

	/* skip leading blanks */
	while (isspace(*msg) && msglen < 0) {
		msg++;
		msglen--;
	}

	/* XXX TODO incorrect return value - should just be the date parsed */
	if (msglen == 0) {
		return (0);
	}

	/* Look for a programname in the string */
	for (i = 0; i < NAME_MAX; i++) {
		if (!isprint(msg[i]) || msg[i] == ':' || msg[i] == '[' ||
		    msg[i] == '/' || isspace(msg[i]))
			break;
	}

	/* If we hit the limit, don't copy */
	/* XXX TODO incorrect return value - should just be the date parsed */
	if (i == NAME_MAX) {
		return (0);
	}

	/* Ok, msg[0] -> msg[i] is the string */
	logd_msg_set_src_name(m, msg, i);

	return (len);
#undef	NAME_MAX
#undef	MAXDATELEN
}

/*
 * Loop over and attempt to consume a message.
 * Note that in syslog format things are terminated by a \n, and we
 * may end up reading a partial buffer - so yes we have to also handle
 * that.
 *
 * If the read buffer is full then we also have to handle that and
 * potentially flush the buffer.
 */
static int
logd_source_klog_read_cb(struct logd_source *ls, void *arg)
{
	struct logd_msg *m;
	const char *p, *q;
	int l, ret = 0;

	//fprintf(stderr, "%s: called\n", __func__);

	while (logd_buf_get_len(&ls->rbuf) != 0) {
		/* start of line */
		p = logd_buf_get_buf(&ls->rbuf);

		/* look for a \n */
		q = memchr(p, '\n', logd_buf_get_len(&ls->rbuf));
		if (q == NULL) {
			/* We don't have a complete line, so bail */
			fprintf(stderr, "%s: still waiting for EOL\n", __func__);
			break;
		}

		/* we have a line, so create a buf for it, and consume */
		l = q - p + 1;
		if (l < 0) {
			/* Error, shouldn't happen */
			return (-1);
		}

		/* Now, we have a string of len 'l', so populate */
		m = logd_msg_create(l);
		if (m == NULL)
			return (-1);
		logd_msg_set_str(m, p, l);

		/* XXX rest of the parsing, etc to do here */
		logd_trim_trailing_newline(&m->buf);

		/* Consume the buffer - XXX TODO: l or l+1? */
		logd_buf_consume(&ls->rbuf, NULL, l);

		/* Parse the facility out */
		logd_source_klog_parse_facility(m);

		/* Logfile, procname */
		logd_source_klog_parse_timestamp_procname(m);

		/* Add the message to the source */
		if (logd_source_add_read_msg(ls, m) < 0) {
			/*
			 * XXX TODO should notify caller that we couldn't add
			 * the message
			 */
			logd_msg_free(m);
			continue;
		}

		ret++;
	}

	return (ret);
}

/*
 * UNIX domain socket / UDP socket version of the above.
 *
 * If the read buffer is full then we also have to handle that and
 * potentially flush the buffer.
 */
static int
logd_source_klog_read_dgram_cb(struct logd_source *ls, void *arg)
{
	struct logd_msg *m;
	const char *p, *q;
	int l, ret = 0;

	//fprintf(stderr, "%s: called\n", __func__);

	if (logd_buf_get_len(&ls->rbuf) == 0) {
		return (0);

	}

	p = logd_buf_get_buf(&ls->rbuf);
	l = logd_buf_get_len(&ls->rbuf);

	/* Now, we have a string of len 'l', so populate */
	m = logd_msg_create(l);
	if (m == NULL)
		return (-1);
	logd_msg_set_str(m, p, l);

	/* XXX rest of the parsing, etc to do here */
	logd_trim_trailing_newline(&m->buf);

	/* Consume the buffer - XXX TODO: l or l+1? */
	logd_buf_consume(&ls->rbuf, NULL, l);

	/* Parse the facility out */
	logd_source_klog_parse_facility(m);

	/* Logfile, procname */
	logd_source_klog_parse_timestamp_procname(m);

	/* Add the message to the source */
	if (logd_source_add_read_msg(ls, m) < 0) {
		/*
		 * XXX TODO should notify caller that we couldn't add
		 * the message
		 */
		logd_msg_free(m);
		return (0);
	}

	return (1);
}

static int
logd_source_klog_error_cb(struct logd_source *ls, void *arg, int error)
{

	fprintf(stderr, "%s: called; error=%d\n", __func__, error);

	return (0);
}

static int
logd_source_klog_free_cb(struct logd_source *ls, void *arg)
{
	struct logd_source_klog *kl = arg;

	fprintf(stderr, "%s: called\n", __func__);
	/* Do teardown */

	free(kl->path);
	return (0);
}

static int
logd_source_klog_open_cb(struct logd_source *ls, void *arg)
{
	struct logd_source_klog *kl = arg;
	int fd;

	fprintf(stderr, "%s: called\n", __func__);

	switch (kl->fd_type) {
	case KLOG_FD_TYPE_KLOG:
		/* /dev/klog - always open read only for now */
		fd = open(kl->path, O_RDONLY | O_NONBLOCK | O_CLOEXEC, 0);
		if (fd < 0) {
			warn("%s: open(%s)", __func__, kl->path);
			return (-1);
		}

		/* Make socket non-blocking */
		evutil_make_socket_nonblocking(fd);
		break;
	case KLOG_FD_TYPE_FIFO:
		{
			struct sockaddr_un sa;

			/* Remove socket before we re-create it */
			unlink(kl->path);

			/* Create socket */
			fd = socket(AF_UNIX, SOCK_DGRAM, 0);
			if (fd < 0) {
				warn("%s: open(%s)", __func__, kl->path);
				return (-1);
			}


			/* Bind */
			bzero(&sa, sizeof(sa));
			sa.sun_family = AF_UNIX;
			strncpy(sa.sun_path, kl->path, sizeof(sa.sun_path) - 1);
			if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
				warn("%s: bind(%s)", __func__, kl->path);
				close(fd);
				return (-1);
			}

			/* chmod - default mode for now */
			if (chmod(kl->path, DEFFILEMODE) < 0) {
				warn("%s: chmod(%s, 0666)", __func__, kl->path);
				close(fd);
				return (-1);
			}

			/* Make socket non-blocking */
			evutil_make_socket_nonblocking(fd);
		}
		break;
	default:
		fprintf(stderr, "%s: unknown fd type (%d)\n", __func__,
		    kl->fd_type);
		return (-1);
	}

	/* XXX should be a method */
	ls->fd = fd;

	return (0);
}

static int
logd_source_klog_close_cb(struct logd_source *ls, void *arg)
{
	struct logd_source_klog *kl = arg;

	fprintf(stderr, "%s: called\n", __func__);

	/* Note: for now, main class has called close() already */

	if (kl->do_unlink) {
		unlink(kl->path);
	}
	return (0);
}

static int
logd_source_klog_write_cb(struct logd_source *l, void *arg)
{

	fprintf(stderr, "%s: called\n", __func__);
	return (-1);
}

static int
logd_source_klog_reopen_cb(struct logd_source *l, void *arg)
{

	fprintf(stderr, "%s: called\n", __func__);
	return (-1);
}

static int
logd_source_klog_sync_cb(struct logd_source *l, void *arg)
{

	fprintf(stderr, "%s: called\n", __func__);
	return (-1);
}

static int
logd_source_klog_flush_cb(struct logd_source *l, void *arg)
{

	fprintf(stderr, "%s: called\n", __func__);
	return (-1);
}

struct logd_source *
logd_source_klog_create_read_dev(struct event_base *eb,
    const char *path)
{
	struct logd_source *ls;
	struct logd_source_klog *kl;

	kl = calloc(1, sizeof(*kl));
	if (kl == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	ls = logd_source_create(eb);
	if (ls == NULL) {
		free(kl);
		return (NULL);
	}

	/* Do other setup */
	logd_source_set_child_callbacks(ls,
	    logd_source_klog_read_cb,
	    logd_source_klog_write_cb,
	    logd_source_klog_error_cb,
	    logd_source_klog_free_cb,
	    logd_source_klog_open_cb,
	    logd_source_klog_close_cb,
	    logd_source_klog_sync_cb,
	    logd_source_klog_reopen_cb,
	    logd_source_klog_flush_cb,
	    kl);

	kl->path = strdup(path);
	kl->is_readonly = 1;
	kl->fd_type = KLOG_FD_TYPE_KLOG;

	return (ls);
}

struct logd_source *
logd_source_klog_create_unix_fifo(struct event_base *eb,
    const char *path)
{
	struct logd_source *ls;
	struct logd_source_klog *kl;

	kl = calloc(1, sizeof(*kl));
	if (kl == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	ls = logd_source_create(eb);
	if (ls == NULL) {
		free(kl);
		return (NULL);
	}

	/* Do other setup */
	logd_source_set_child_callbacks(ls,
	    logd_source_klog_read_dgram_cb,
	    logd_source_klog_write_cb,
	    logd_source_klog_error_cb,
	    logd_source_klog_free_cb,
	    logd_source_klog_open_cb,
	    logd_source_klog_close_cb,
	    logd_source_klog_sync_cb,
	    logd_source_klog_reopen_cb,
	    logd_source_klog_flush_cb,
	    kl);

	kl->path = strdup(path);
	kl->do_unlink = 1;
	kl->fd_type = KLOG_FD_TYPE_FIFO;

	return (ls);
}

void
logd_source_klog_init(void)
{

}

void
logd_source_klog_shutdown(void)
{

}
