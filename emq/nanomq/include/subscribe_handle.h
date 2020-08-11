#ifndef MQTT_SUBSCRIBE_HANDLE_H
#define MQTT_SUBSCRIBE_HANDLE_H

#include <nng/nng.h>
#include "include/packet.h"
#include "apps/broker.h"
/*
struct Ctx_sub {
	uint32_t	id;
	struct mqtt_property * variable_property;
	struct topic_with_option * topic_with_option;

// connect info
//	struct ctx_connect * ctx_con;
};
typedef struct Ctx_sub Ctx_sub;
*/

uint8_t decode_sub_message(nng_msg * msg, packet_subscribe * sub_pkt);
uint8_t encode_suback_message(nng_msg * msg, packet_subscribe * sub_pkt);
void sub_ctx_handle(nng_msg * msg, packet_subscribe * sub_pkt);
uint8_t subscribe_handle(nng_msg *msg);

#endif
