#ifndef	__LOGD_SOURCE_KLOG_H__
#define	__LOGD_SOURCE_KLOG_H__

extern	struct logd_source *
logd_source_klog_create_read_dev(struct event_base *eb, const char *path);
extern	struct logd_source *
logd_source_klog_create_unix_fifo(struct event_base *eb, const char *path);

extern	void logd_source_klog_init(void);
extern	void logd_source_klog_shutdown(void);

#endif	/* __LOGD_SOURCE_KLOG_H__ */
