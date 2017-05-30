#ifndef	__LOGD_SOURCE_FILE_H__
#define	__LOGD_SOURCE_FILE_H__

extern	struct logd_source *
logd_sink_file_create_file(struct event_base *eb, const char *path);

extern	void logd_sink_file_init(void);
extern	void logd_sink_file_shutdown(void);

#endif	/* __LOGD_SOURCE_FILE_H__ */
