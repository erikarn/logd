#ifndef	__LOGD_SOURCE_H__
#define	__LOGD_SOURCE_H__

struct logd_source;

typedef int logd_source_read_cb(struct logd_source *, void *);

struct logd_source {
	TAILQ_ENTRY(logd_source) node;
	int fd;
	logd_source_read_cb *cb_read;
	void *cbdata;
};

extern	struct logd_source * logd_source_create(int fd, logd_source_read_cb *read_cb,
	    void *cbdata);
extern	void	logd_source_free(struct logd_source *lsrc);

#endif	/* __LOGD_SOURCE_H__ */
