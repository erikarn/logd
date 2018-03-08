#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <libutil.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <err.h>

#include <sys/mman.h>
#include <sys/queue.h>

#include <event2/event.h>
//#include <event/evsignal.h>

#include "config.h"
#include "logd_app.h"
#include "wait.h"

/* XXX for testing */
#include "logd_buf.h"
#include "logd_msg.h"
#include "logd_source.h"
#include "logd_source_list.h"
#include "logd_source_klog.h"
#include "logd_sink_file.h"
#include "logd_collection.h"

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

	/* XXX temp */
	struct logd_source *ls;
	struct logd_collection *lc;

	if (madvise(NULL, 0, MADV_PROTECT) != 0)
		fprintf(stderr, "madvise() failed: %s\n", strerror(errno));

	/* Module init */
	logd_source_init();
	logd_source_klog_init();
	logd_sink_file_init();
	logd_source_list_init();

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
	lc = logd_collection_create(la.eb);

	/* /dev/klog; only read from it */
	ls = logd_source_klog_create_read_dev(la.eb, "/dev/klog");
	logd_collection_add(lc, ls);
	logd_source_open(ls);

	/* /var/run/log - global rw */
	ls = logd_source_klog_create_unix_fifo(la.eb, "/var/run/log");
	logd_collection_add(lc, ls);
	logd_source_open(ls);

	/* /var/run/logpriv - root only rw */
	ls = logd_source_klog_create_unix_fifo(la.eb, "/var/run/logpriv");
	logd_collection_add(lc, ls);
	logd_source_open(ls);

	/* /tmp/log.txt - write/append output file */
	ls = logd_sink_file_create_file(la.eb, "/tmp/log.txt");
	logd_collection_add(lc, ls);
	logd_source_open(ls);

	logd_app_run(&la);

	/* XXX during shutdown, unlink the UNIX domain logd sources that need it */
	/* XXX and yes, close FDs, etc */

	logd_app_remove_pidfile(&la);
	fprintf(stderr, "%s: exiting\n", __func__);

	exit(0);
}
