#ifndef	__LOGD_APP_H__
#define	__LOGD_APP_H__

struct logd_app {
	struct event_base *eb;

	/* daemon settings */
	int do_background;

	/* pidfile state */
	pid_t pid;
	struct pidfh *pid_fh;
	char *pidfile_path;

};

extern	void logd_app_init(struct logd_app *la);
extern	void logd_app_set_pidfile(struct logd_app *la, const char *pidfile);
extern	int logd_app_open_pidfile(struct logd_app *la);
extern	int logd_app_write_pidfile(struct logd_app *la);
extern	int logd_app_close_pidfile(struct logd_app *la);
extern	int logd_app_remove_pidfile(struct logd_app *la);
extern	int logd_app_set_background(struct logd_app *la, int do_background);

#endif	/* __LOGD_APP_H__ */
