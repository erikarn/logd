#ifndef	__LOGD_FILTER_H__
#define	__LOGD_FILTER_H__

struct logd_msg;
struct logd_filter;

struct logd_filter {
	struct {
		void *cbdata;
	} owner_cb;
};

#endif	/* __LOGD_FILTER_H__ */
