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

#endif	/* __LOGD_MSG_H__ */
