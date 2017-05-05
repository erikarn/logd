#ifndef	__LOGD_SOURCE_H__
#define	__LOGD_SOURCE_H__

struct logd_source;

typedef int logd_source_read_cb(struct logd_source *, void *);

struct logd_source {
	TAILQ_ENTRY(logd_source) node;
	int fd;
	int is_closing;
	logd_source_read_cb *cb_read;
	void *cbdata;

	struct event_base *eb;
	struct event *read_ev;

	/* Yeah yeah, I should have a buf abstraction */
	struct logd_buf rbuf;
};

extern	struct logd_source * logd_source_create(int fd, struct event_base *eb,
	    logd_source_read_cb *read_cb, void *cbdata);
extern	void	logd_source_free(struct logd_source *lsrc);

extern	void logd_source_init(void);
extern	void logd_source_shutdown(void);

#endif	/* __LOGD_SOURCE_H__ */
