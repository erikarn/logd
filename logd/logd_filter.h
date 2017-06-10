#ifndef	__LOGD_FILTER_H__
#define	__LOGD_FILTER_H__

struct logd_source;
struct logd_msg;
struct logd_filter;

struct logd_filter {
	/* The list of logd_source instances */
	TAILQ_HEAD(, logd_source) source_list;

	/* We're on a list.. */
	TAILQ_ENTRY(logd_filter) node;

	struct {
		void *cbdata;
	} owner_cb;
};

#endif	/* __LOGD_FILTER_H__ */
