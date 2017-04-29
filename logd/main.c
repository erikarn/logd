#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libutil.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <err.h>

#include <sys/mman.h>

#include "config.h"
#include "logd_app.h"
#include "wait.h"

static void
usage(void)
{

	printf("Usage: logd (-p <pidfile>)\n");
	printf("  -f			Run in foreground\n");
	printf("  -p <pidfile>		Set pidfile (default is %s)\n",
	    LOGD_PID_FILE);
	exit(127);
}

int
main(int argc, char *argv[])
{
	struct logd_app la;
	pid_t ppid;
	int opt;

	if (madvise(NULL, 0, MADV_PROTECT) != 0)
		fprintf(stderr, "madvise() failed: %s\n", strerror(errno));

	logd_app_init(&la);
	fprintf(stderr, "%s: starting\n", argv[0]);

	/* Command line handling */

	while ((opt = getopt(argc, argv, "fhp:")) != -1) {
		switch (opt) {
		case 'f':
			logd_app_set_background(&la, 0);
			break;
		case 'p':
			logd_app_set_pidfile(&la, optarg);
			break;
		case '?':
		case 'h':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/* Create pidfile */
	(void) logd_app_open_pidfile(&la);

	/* Turn into a daemon if required */
	if (la.do_background == 1) {
		fprintf(stderr, "%s: forking child\n", __func__);
		ppid = wait_daemon(5);
		if (ppid < 0) {
			warn("could not become daemon");
			logd_app_remove_pidfile(&la);
			exit(1);
		}
		fprintf(stderr, "%s: child ppid=%d\n", __func__, ppid);
	}

	/* Need to actually do some work here .. */

	logd_app_remove_pidfile(&la);
	fprintf(stderr, "%s: exiting\n", __func__);

	exit(0);
}
