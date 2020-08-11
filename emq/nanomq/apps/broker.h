#ifndef NANOMQ_BROKER_H
#define NANOMQ_BROKER_H
#define MQTT_VER 5

#include <nng/nng.h>
#include <nng/protocol/mqtt/emq_tcp.h>
#include <nng/supplemental/util/platform.h>
#include <nng/protocol/mqtt/mqtt.h>


struct work {
	enum { INIT, RECV, WAIT, SEND } state;
	nng_aio *aio;
	nng_msg *msg;
	nng_ctx  ctx;
	struct db_tree *db;
	conn_param *cparam;
	struct pub_packet_struct *pub_packet;
};


typedef struct work emq_work;

int broker_start(int argc, char **argv);

int broker_dflt(int argc, char **argv);

#endif