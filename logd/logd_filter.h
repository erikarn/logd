#ifndef	__LOGD_FILTER_H__
#define	__LOGD_FILTER_H__

struct logd_msg;
struct logd_source;
struct logd_filter;

typedef int logd_filter_check_cb_t(struct logd_filter *, void *,
	    struct logd_source *src, struct logd_source *dst,
	    struct logd_msg *);
typedef	int logd_filter_free_cb_t(struct logd_filter *, void *);

struct logd_filter {
	struct event_base *eb;

	struct {
		void *cbdata;
	} owner_cb;

	struct {
		void *cbdata;
		logd_filter_check_cb_t * filter_check_cb;
		logd_filter_free_cb_t * filter_free_cb;
	} child_cb;
};

extern	struct logd_filter * logd_filter_create(struct event_base *eb);
extern	int logd_filter_free(struct logd_filter *);
extern	int logd_filter_check(struct logd_filter *, struct logd_source *,
	    struct logd_source *, struct logd_msg *);

#endif	/* __LOGD_FILTER_H__ */
