PROG=	logd
SRCS=	logd_app.c main.c wait.c
SRCS+=	logd_source.c
SRCS+=	logd_source_list.c
SRCS+=	logd_source_klog.c
SRCS+=	logd_sink_file.c
SRCS+=	logd_msg.c
SRCS+=	logd_buf.c
SRCS+=	logd_collection.c
SRCS+=	logd_socket_dgram_io.c
SRCS+=	logd_filter.c
SRCS+=	logd_filter_true.c

# XXX this is so we can compile using the internal libevent for now
# .. and it's totally wrong, and also using CFLAGS like this is also wrong.
#CFLAGS=	-I/usr/include/private
#LDADD=	-lutil -lprivateevent

CFLAGS=		-I/usr/local/include -Wall -Werror
LDADD=-lutil -L/usr/local/lib -levent_core

MK_MAN=no

.include <bsd.prog.mk>

