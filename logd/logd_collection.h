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
	struct logd_source *src;
	TAILQ_ENTRY(logd_collection_entry) node;
};

struct logd_collection {
	struct event_base *eb;

	/* collection entries */
	TAILQ_HEAD(, logd_collection_entry) entries;

	struct logd_filter *filter;

	struct {
		void *cbdata;
		logd_collection_filter_cb_t *filter_cb;
	} owner_cb;
};

extern	struct logd_collection * logd_collection_create(struct event_base *eb);
extern	int logd_collection_assign_filter(struct logd_collection *,
	    struct logd_filter *);
extern	void logd_collection_free(struct logd_collection *);

extern	struct logd_collection_entry * logd_collection_add(struct logd_collection *,
	    struct logd_source *);

#endif	/* __LOGD_COLLECTION_H__ */
