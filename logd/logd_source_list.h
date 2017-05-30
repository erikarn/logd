#ifndef	__LOGD_SOURCE_LIST__
#define	__LOGD_SOURCE_LIST__

struct logd_source_list;
struct logd_source;
struct logd_msg;

typedef	int logd_source_list_logmsg_read_cb(struct logd_source_list *,
	    struct logd_source *, void *arg, struct logd_msg *);
typedef	int logd_source_list_error_cb(struct logd_source_list *,
	    struct logd_source *, void *arg, int);

struct logd_source_list {

	TAILQ_HEAD(, logd_source) list;
	struct {
		logd_source_list_logmsg_read_cb *logmsg_cb;
		logd_source_list_error_cb *error_cb;
		void *cbdata;
	} owner_cb;
};

extern	struct logd_source_list * logd_source_list_new(void);

extern	int logd_source_list_add(struct logd_source_list *,
	    struct logd_source *);
extern	int logd_source_list_remove(struct logd_source_list *,
	    struct logd_source *);

extern	void logd_source_list_set_owner_callbacks(struct logd_source_list *,
	    logd_source_list_logmsg_read_cb *logmsg_cb,
	    logd_source_list_error_cb *error_cb,
	    void *cbdata);

extern	void logd_source_list_flush(struct logd_source_list *);
extern	void logd_source_list_close(struct logd_source_list *);
extern	void logd_source_list_open(struct logd_source_list *);
extern	void logd_source_list_free(struct logd_source_list *);
extern	void logd_source_list_write(struct logd_source_list *,
	    struct logd_msg *);

extern	void logd_source_list_init(void);
extern	void logd_source_list_shutdown(void);


#endif	/* __LOGD_SOURCE_LIST__ */
