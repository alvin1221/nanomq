#include "include/apps.h"

#include "include/version.h"
#include "include/process.h"
#include "include/cmd.h"
#include "include/const_strings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define NANO_APP_NAME "nanomq"
#define NANO_BRAND "EMQ X Edge Computing Kit"

static void print_version(void)
{
	printf("%s v.01-%s\n", NANO_BRAND, FW_EV_VER_ID_SHORT);
	printf("Copyright (C) 2012-2020 EMQ X Jaylin.\n");
	printf("\n");
}

static int print_avail_apps(void)
{
#if defined(DASH_DEBUG)
	const struct nanomq_app **dash_app;

	printf("available applications:\n");

	for (dash_app = dash_apps; *dash_app; ++dash_app)
		printf("   * %s\n", (*dash_app)->name);
#endif
	print_version();
	return 1;
}

#if !defined(DEBUG_TRACE)
static int check_trace(char *name)
{
	int pid, traced;

	switch(pid = fork()) {
	case  0:
		pid = getppid();
		traced = ptrace(PTRACE_ATTACH, pid, 0, 0);

		if (!traced) {
			process_send_signal(pid, SIGCONT);
			_exit(EXIT_SUCCESS);
		}

		perror(name);
		process_send_signal(pid, SIGKILL);
		goto err;
	case -1:
		break;
	default:
		if (pid == waitpid(pid, 0, 0))
			return EXIT_SUCCESS;

		break;
	}

	perror(name);
err:
	return -1;
}
#else
static int check_trace(char DASH_UNUSED(*name))
{
	return 0;
}
#endif

static int handle_app(int res)
{
	cmd_cleanup();
	return res;
}

int main(int argc, char **argv)
{
	const struct nanomq_app **dash_app;
	char *app_name;
	int ret;

	ret = check_trace(argv[0]);
	if (ret < 0)
		return EXIT_FAILURE;

	if ((argc > 1) && (strlen(argv[1]) > 1) &&
	    (argv[1][0] == '-') && (argv[1][1] == 'v')) {
		print_version();
		return EXIT_SUCCESS;
	}

	app_name = strrchr(argv[0], '/');
	app_name = (app_name ? app_name + 1 : argv[0]);

	if (strncmp(app_name, NANO_APP_NAME, APP_NAME_MAX) == 0) {
		if (argc == 1)
			return print_avail_apps();

		app_name = argv[1];
		argv++;
		argc--;
	}

	for (dash_app = edge_apps; *dash_app; ++dash_app)
		if (strncmp(app_name, (*dash_app)->name, APP_NAME_MAX) == 0)
			break;

	if (!(*dash_app)) {
		printf("Error - the application '%s' was not found\n", app_name);
		return EXIT_FAILURE;
	}

	if (argc < 2) {
		if ((*dash_app)->dflt)
			return handle_app((*dash_app)->dflt(argc - 1, argv + 1));

		printf("Error - not enough arguments to run %s\n",
		       app_name);
		goto err_param;
	}

	if ((strcmp(argv[1], "start") == 0) && (*dash_app)->start)
		return handle_app((*dash_app)->start(argc - 2, argv + 2));

	if ((strcmp(argv[1], "stop") == 0) && (*dash_app)->stop)
		return handle_app((*dash_app)->stop(argc - 2, argv + 2));

	if ((*dash_app)->dflt)
		return handle_app((*dash_app)->dflt(argc - 1, argv + 1));

	printf("Error - unknown parameter: %s\n", argv[1]);

err_param:
	printf("Use one of the following parameters:\n");
	if ((*dash_app)->start)
		printf("   * start\n");
	if ((*dash_app)->stop)
		printf("   * stop\n");
	return EXIT_FAILURE;
}
