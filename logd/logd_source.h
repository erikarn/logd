#ifndef	__LOGD_SOURCE_H__
#define	__LOGD_SOURCE_H__

struct logd_source;

typedef int logd_source_read_cb(struct logd_source *, void *);
typedef int logd_source_close_cb(struct logd_source *, void *, int);

typedef int logd_source_logmsg_read_cb(struct logd_source *, void *,
	    struct logd_msg *m);

struct logd_source {
	TAILQ_ENTRY(logd_source) node;
	int fd;
	int is_closing;

#if 0
	/*
	 * List of messages that we've read that the owner should
	 * consume.
	 */
	TAILQ_HEAD(, logd_msg) read_msgs;

	/*
	 * List of messages that we've been asked to send.
	 */
	TAILQ_HEAD(, logd_msg) write_msgs;
#endif

	/* Callbacks - us to child */
	struct {
		logd_source_read_cb *cb_read;
		logd_source_read_cb *cb_close;
		void *cbdata;
	} child_cb;

	/* Callbacks - us to owner */
	struct {
		logd_source_logmsg_read_cb *cb_logmsg_read;
		void *cbdata;
	} owner_cb;

	struct event_base *eb;
	struct event *read_ev;

	/*
	 * Incoming unstructured data (eg from a file, socket)
	 * that needs translating into log messages to log.
	 */
	struct logd_buf rbuf;
};

extern	struct logd_source * logd_source_create(int fd, struct event_base *eb,
	    logd_source_read_cb *read_cb, void *cbdata);
extern	void	logd_source_free(struct logd_source *lsrc);

extern	void logd_source_init(void);
extern	void logd_source_shutdown(void);

#endif	/* __LOGD_SOURCE_H__ */
