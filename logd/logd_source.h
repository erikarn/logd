#ifndef	__LOGD_SOURCE_H__
#define	__LOGD_SOURCE_H__

struct logd_source;

typedef int logd_source_read_cb(struct logd_source *, void *);
typedef int logd_source_free_cb(struct logd_source *, void *);

typedef int logd_source_logmsg_read_cb(struct logd_source *, void *,
	    struct logd_msg *m);

typedef int logd_source_error_cb(struct logd_source *, void *, int);
typedef int logd_source_open_cb(struct logd_source *, void *);
typedef int logd_source_close_cb(struct logd_source *, void *);

#define	LOGD_SOURCE_ERROR_READ_EOF		1
#define	LOGD_SOURCE_ERROR_READ_FULL		2
#define	LOGD_SOURCE_ERROR_READ_ERROR		3

struct logd_source {
	TAILQ_ENTRY(logd_source) node;
	int fd;
	int is_closing;

#if 1
	/*
	 * List of messages that we've read that the owner should
	 * consume.
	 */
	TAILQ_HEAD(, logd_msg) read_msgs;
#endif

#if 0
	/*
	 * List of messages that we've been asked to send.
	 */
	TAILQ_HEAD(, logd_msg) write_msgs;
#endif

	/* Callbacks - us to child */
	struct {
		logd_source_read_cb *cb_read;
		logd_source_error_cb *cb_error;
		logd_source_free_cb *cb_free;
		logd_source_free_cb *cb_open;
		logd_source_free_cb *cb_close;
		void *cbdata;
	} child_cb;

	/* Callbacks - us to owner */
	struct {
		logd_source_logmsg_read_cb *cb_logmsg_read;
		logd_source_error_cb *cb_error;
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

/*
 * Global methods
 */
extern	struct logd_source * logd_source_create(struct event_base *eb);
extern	void	logd_source_free(struct logd_source *lsrc);

/*
 * Attach the owner callbacks.
 */
extern	void logd_source_set_owner_callbacks(struct logd_source *ls,
	    logd_source_logmsg_read_cb *cb_logmsg_read,
	    logd_source_error_cb *cb_error,
	    void *cbdata);

/*
 * Child related methods.
 */

/*
 * Attach the child methods.
 */
extern	void logd_source_set_child_callbacks(struct logd_source *ls,
	    logd_source_read_cb *cb_read,
	    logd_source_error_cb *cb_error,
	    logd_source_free_cb *cb_free,
	    logd_source_open_cb *cb_open,
	    logd_source_close_cb *cb_close,
	    void *cbdata);

/*
 * Start reading from the end-point.
 */
extern	void logd_source_read_start(struct logd_source *ls);

/*
 * Stop reading from the end-point.
 */
extern	void logd_source_read_stop(struct logd_source *ls);

/*
 * Called from the child to push a parsed message to the owner.
 */
extern	int logd_source_add_read_msg(struct logd_source *ls,
	    struct logd_msg *m);

/*
 * Open the configured end-point (file, socket, etc).
 */
extern	int logd_source_open(struct logd_source *ls);

/*
 * Close the configured end-point (file, socket, etc.)
 */
extern	int logd_source_close(struct logd_source *ls);

/*
 * Flush pending items to the end-point.
 */
extern	int logd_source_flush(struct logd_source *ls);


/*
 * Global setup/teardown.
 */
extern	void logd_source_init(void);
extern	void logd_source_shutdown(void);

#endif	/* __LOGD_SOURCE_H__ */
