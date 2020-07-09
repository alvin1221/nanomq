
#ifndef __NANO_DASH_H__
#define __NANO_DASH_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/nl80211.h>

#define CHECKIN_NAME "emq"
#define DEBUG_FILE_PATH "/tmp/debug_nanomq.log"
#define API_SERVER_IP "localhost"
#define API_SERVER_PORT "1883"
#define CHECKIN_FLAG_MAX 20
#define FW_EV_VER_ID_SHORT 0

#define EMQ_MAX_PACKET_LEN 20
#define EMQ_CONNECT_PACKET_LEN 200

// later on makefile
#define DEBUG_CONSOLE
#define DEBUG_FILE
#define DEBUG_SYSLOG

#undef DASH_DEBUG
#if defined(DEBUG_CONSOLE) || defined(DEBUG_FILE) || defined(DEBUG_SYSLOG)
#define DASH_DEBUG 1

#include <unistd.h>
#include <sys/types.h>
#include <syslog.h>
#include <stdarg.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>



extern char *getlocalip(void);

static inline char *dash_time_str()
{
	char *buff;
	time_t now;

	now = time(NULL);
	buff = ctime(&now);
	if (!buff)
		return NULL;

	if (buff[strlen(buff) - 1] == '\n')
		buff[strlen(buff) - 1] = '\0';

	return buff;
}

#endif

#if defined(DEBUG_CONSOLE)
#define debug_console(fmt, arg...) do {\
	char *_t = dash_time_str();\
	fprintf(stderr, "%s %s: " fmt "\n", _t, __FUNCTION__, ## arg);\
} while (0)
#else
#define debug_console(fmt, arg...) do { } while (0)
#endif

#if defined(DEBUG_FILE)
#define debug_file(fmt, arg...) do {\
	char *_t = dash_time_str();\
	FILE *file = fopen(DEBUG_FILE_PATH, "a");\
	fprintf(file, "%s [%i] %s: " fmt "\n", _t, getpid(), __FUNCTION__, ## arg);\
	fclose(file);\
} while(0)
#else
#define debug_file(fmt, arg...) do { } while (0)
#endif

#if defined(DEBUG_SYSLOG)
#define debug_syslog(fmt, arg...) do {\
	openlog("dashboard-ev", LOG_PID, LOG_DAEMON | LOG_EMERG);\
	syslog(0, "%s: " fmt, __FUNCTION__, ## arg);\
	closelog();\
} while (0)
#else
#define debug_syslog(fmt, arg...) do { } while (0)
#endif


#if defined(DASH_DEBUG)
#define debug_msg(fmt, arg...) do {		\
	debug_console(fmt, ## arg);			\
	debug_file(fmt, ## arg);			\
	debug_syslog(fmt, ## arg);			\
} while (0)
#else
#define debug_msg(fmt, arg...) do { } while (0)
#endif

#define DASH_UNUSED(x) (x)__attribute__((unused))


enum emq_api_ver {
	API_unknown = 0,
	APIv2 = 2,
};

struct checkin_state {
	FILE *fd;
	int checkin_generated;
	// TO DO
};

struct client_traffic_data {
	uint8_t mac[ETH_ALEN];
	uint64_t up;
	uint64_t down;
};



#endif /* __NANO_DASH_H__ */
