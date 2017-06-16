#ifndef	__LOGD_SOCKET_DGRAM_IO_H__
#define	__LOGD_SOCKET_DGRAM_IO_H__

struct logd_socket_dgram_io;

/* XXX TODO: turn into enum */
#define	LOGD_SOCK_DGRAM_IO_ERR_SOCKET_READ		1
#define	LOGD_SOCK_DGRAM_IO_ERR_SOCKET_WRITE		2
#define	LOGD_SOCK_DGRAM_IO_ERR_SOCKET_READ_ALLOC	3

typedef int logd_socket_dgram_io_error_cb(struct logd_socket_dgram_io *,
	    void *, int, int);
typedef int logd_socket_dgram_io_read_cb(struct logd_socket_dgram_io *,
	    void *, struct logd_buf *);
typedef int logd_socket_dgram_io_write_cb(struct logd_socket_dgram_io *,
	    void *);

/*
 * This implements a datagram buffer event IO mechanism for
 * streaming data to/from a datagram socket.
 *
 * UDP sockets and klog are datagram sockets, not streaming
 * sockets.  Their behaviour is different to bufferevents and
 * not supported by buffer events.
 *
 * Log messages are a list of logd_bufs, rather than a single
 * buffer being appended to.
 */

struct logd_socket_dgram_io {
	int fd;
	int is_closing;
	int is_running;

	/*
	 * List of messages that we've read that the owner should
	 * consume.
	 */
	TAILQ_HEAD(, logd_buf) read_msgs;

	/*
	 * List of messages that we've been asked to send.
	 */
	TAILQ_HEAD(, logd_buf) write_msgs;

	/*
	 * Maximum number of messages to queue for TX / RX before
	 * we overflow.
	 */
	int max_read_msgs;
	int num_read_msgs;
	int num_read_msgs_overflow;

	int max_write_msgs;
	int num_write_msgs;
	int num_write_msgs_overflow;

	int read_buf_size;

	/* Callbacks - us to owner */
	struct {
		logd_socket_dgram_io_read_cb *cb_read;
		logd_socket_dgram_io_error_cb *cb_error;
		logd_socket_dgram_io_write_cb *cb_write;
		void *cbdata;
	} owner_cb;

	struct event_base *eb;

	struct event *read_sock_ev;
	struct event *write_sock_ev;
	struct event *read_sched_ev;
	struct event *write_sched_ev;
};

/*
 * Global methods
 */
extern	struct logd_socket_dgram_io * logd_socket_dgram_io_create(struct event_base *eb,
	    int fd);
extern	void logd_socket_dgram_io_free(struct logd_socket_dgram_io *lsrc);

/*
 * Attach the owner callbacks.
 */
extern	void logd_socket_dgram_io_set_owner_callbacks(struct logd_socket_dgram_io *ls,
	    logd_socket_dgram_io_read_cb *cb_read,
	    logd_socket_dgram_io_error_cb *cb_error,
	    logd_socket_dgram_io_write_cb *cb_write,
	    void *cbdata);

/*
 * Start reading from the end-point.
 */
extern	void logd_socket_dgram_io_read_start(struct logd_socket_dgram_io *ls);

/*
 * Stop reading from the end-point.
 */
extern	void logd_socket_dgram_io_read_stop(struct logd_socket_dgram_io *ls);

/*
 * Sync pending items to/from the end-point (eg disk sync.)
 */
extern	int logd_socket_dgram_io_sync(struct logd_socket_dgram_io *ls);

/*
 * Flush (remove) pending items to/from the end-point.
 */
extern	int logd_socket_dgram_io_flush(struct logd_socket_dgram_io *ls);

/*
 * Write a message.
 */
extern	int logd_socket_dgram_io_write(struct logd_socket_dgram_io *ls,
	    struct logd_buf *b);

/*
 * Global setup/teardown.
 */
extern	void logd_socket_dgram_io_init(void);
extern	void logd_socket_dgram_io_shutdown(void);

#endif	/* __LOGD_SOURCE_H__ */
