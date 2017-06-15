#ifndef	__LOGD_BUF_H__
#define	__LOGD_BUF_H__

struct logd_buf;
struct logd_buf {
	TAILQ_ENTRY(logd_buf) node;
	char *buf;
	int size;
	int len;
};

static inline const char * logd_buf_get_offset(struct logd_buf *b,
    int offset)
{

	if (offset >= b->len) {
		return (NULL);
	}
	return (&b->buf[offset]);
}

static inline const char *logd_buf_get_buf(struct logd_buf *b)
{

	return (b->buf);
}

static inline int logd_buf_get_size(struct logd_buf *b)
{

	return (b->size);
}

static inline int logd_buf_get_len(struct logd_buf *b)
{

	return (b->len);
}

static inline int logd_buf_get_freespace(struct logd_buf *b)
{

	return (b->size - b->len);
}


extern	int logd_buf_init(struct logd_buf *b, int size);
extern	void logd_buf_done(struct logd_buf *b);

extern	int logd_buf_consume(struct logd_buf *b, char *buf, int size);
extern	int logd_buf_append(struct logd_buf *b, const char *buf, int len,
	    int do_partial);
extern	int logd_buf_populate(struct logd_buf *b, const char *buf, int size);

extern	int logd_buf_read_append(struct logd_buf *b, int fd, int max_size);
extern	int logd_buf_write_consume(struct logd_buf *b, int fd, int max_size);

extern	int logd_trim_trailing_newline(struct logd_buf *b);

#endif	/* __LOGD_BUF_H__ */
