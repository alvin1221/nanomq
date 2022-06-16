#ifndef ZMQ_PROXY_H
#define ZMQ_PROXY_H

#include <nng/mqtt/mqtt_client.h>
#include <nng/supplemental/util/platform.h>
#include <nng/nng.h>
#include "conf.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#ifdef NANO_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>
#endif


int  zmq_gateway(zmq_gateway_conf *conf);

extern int gateway_start(int argc, char **argv);
extern int gateway_dflt(int argc, char **argv);
#endif