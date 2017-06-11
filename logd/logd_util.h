#ifndef	__LOGD_UTIL_H__
#define	__LOGD_UTIL_H__

static inline int
errno_sockio_fatal(int x)
{
	switch (x) {
	case EWOULDBLOCK:
	case EINTR:
		return (0);
	default:
		return (1);
	}
}


#endif	/* __LOGD_UTIL_H__ */
