#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libutil.h>
#include <strings.h>
#include <string.h>
#include <err.h>

#include <sys/queue.h>
#include <sys/param.h>	/* MIN */

#include <event2/event.h>

#include "config.h"

#include "logd_util.h"
#include "logd_buf.h"


int
logd_buf_init(struct logd_buf *b, int size)
{

	b->buf = malloc(size);
	if (b->buf == NULL) {
		warn("%s: malloc(%d)", __func__, size);
		return (-1);
	}

	b->size = size;
	b->len = 0;

	return (0);
}

void
logd_buf_done(struct logd_buf *b)
{

	free(b->buf);
	b->buf = NULL;
	b->size = 0;
	b->len = 0;

}

int
logd_buf_consume(struct logd_buf *b, char *buf, int size)
{
	int copy_size;

	copy_size = MIN(size, b->len);
	if (copy_size == 0)
		return (0);

	/* copy */
	if (buf)
		memcpy(buf, b->buf, copy_size);

	/* shuffle non-copied data back */
	memmove(b->buf, b->buf + copy_size, b->size - copy_size);
	b->len -= copy_size;

	return (copy_size);
}

int
logd_buf_append(struct logd_buf *b, const char *buf, int len,
    int do_partial)
{
	int copy_len;

	if (do_partial) {
		if (len + b->len >= b->size)
			return (0);
	}

	copy_len = MIN(len, (b->size - b->len));
	/* Optimisation */
	if (copy_len == 0)
		return (0);

	memcpy(b->buf + b->len, buf, copy_len);
	b->len += copy_len;
	return (copy_len);
}

/*
 * Read data from FD into logd_buf.
 *
 * If we're out of space, return -2.
 * If we've hit an error, return -1.
 * If we've hit EOF, return 0.
 * If we've read something, return > 0.
 */
int
logd_buf_read_append(struct logd_buf *b, int fd, int max_size)
{
	int r, read_len;

	if (b->len >= b->size)
		return (-2);

	read_len = MIN(max_size, b->size - b->len);

	r = read(fd, b->buf + b->len, read_len);

	/* EOF */
	if (r == 0)
		return (0);

	/* Read syscall error */
	if (r < 0)
		return (-1);

	/* Update len, return */
	b->len += r;
	return (r);
}

int
logd_buf_write_consume(struct logd_buf *b, int fd, int max_size)
{
	int r, write_len;

	if (b->len == 0)
		return (-2);

	write_len = MIN(max_size, b->len);

	r = write(fd, b->buf, write_len);
	if (r == 0)
		return (0);
	if (r < 0)
		return (-1);

	/* Note: we may write less than requested */

	/* Shuffle unread data back */
	memmove(b->buf, b->buf + r, b->size - r);
	b->len -= write_len;

	return (r);
}
