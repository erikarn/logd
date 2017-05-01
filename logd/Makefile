PROG=	logd
SRCS=	logd_app.c main.c wait.c

# XXX this is so we can compile using the internal libevent for now
# .. and it's totally wrong, and also using CFLAGS like this is also wrong.
CFLAGS=	-I/usr/include/private
LDADD=	-lutil -lprivateevent

MK_MAN=no

.include <bsd.prog.mk>

