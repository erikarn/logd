#ifndef	__LOGD_MSG_H__
#define	__LOGD_MSG_H__

struct logd_msg;

struct logd_msg {
	TAILQ_ENTRY(logd_msg) node;
	char *buf;
	int len;
	int size;
	struct timespec ts_recv;
	struct timespec ts_msg;
};

extern	struct logd_msg * logd_msg_create(void);
extern	void logd_msg_free(struct logd_msg *);
extern	void logd_set_timestamps(struct logd_msg *, struct timespec *tr,
	    struct timespec *tm);
extern	int logd_msg_set_str(struct logd_msg *, const char *, int);

#endif	/* __LOGD_MSG_H__ */
