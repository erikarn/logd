#ifndef	__LOGD_COLLECTION_H__
#define	__LOGD_COLLECTION_H__

struct logd_source;
struct logd_msg;
struct logd_collection;
struct logd_filter;

typedef	int logd_collection_filter_cb_t(struct logd_collection *, void *,
	    struct logd_filter *, struct logd_msg *);

struct logd_collection {
	/* The list of logd_source instances */
	TAILQ_HEAD(, logd_filter) logd_list;

	struct {
		void *cbdata;
		logd_collection_filter_cb_t *filter_cb;
	} owner_cb;
};

#endif	/* __LOGD_COLLECTION_H__ */
