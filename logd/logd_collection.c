#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libutil.h>
#include <strings.h>
#include <string.h>
#include <err.h>

#include <sys/queue.h>

#include <event2/event.h>

#include "config.h"

#include "logd_util.h"
#include "logd_buf.h"
#include "logd_msg.h"
#include "logd_source.h"
#include "logd_filter.h"
#include "logd_collection.h"

/*
 * This is a collection of logd sources/sinks.
 *
 * Each sink/source is "owned" by this collection; any messages received
 * will be sent through to all the other sources/sinks after a filter is
 * called to determine whether to send them.
 */
