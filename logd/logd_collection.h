#ifndef	__LOGD_COLLECTION_H__
#define	__LOGD_COLLECTION_H__

struct logd_source;
struct logd_msg;
struct logd_collection;
struct logd_filter;

typedef	int logd_collection_filter_cb_t(struct logd_collection *, void *,
	    struct logd_filter *, struct logd_msg *);

struct logd_collection_entry {
	struct logd_collection *parent;
	struct logd_filter *filt;
	TAILQ_ENTRY(logd_collection_entry) node;
	TAILQ_HEAD(, logd_source) src;
};

struct logd_collection {
	/* collection entries */
	TAILQ_HEAD(, logd_collection_entry) entries;

	struct {
		void *cbdata;
		logd_collection_filter_cb_t *filter_cb;
	} owner_cb;
};

#endif	/* __LOGD_COLLECTION_H__ */
