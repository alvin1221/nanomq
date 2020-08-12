#ifndef MQTT_SUBSCRIBE_HANDLE_H
#define MQTT_SUBSCRIBE_HANDLE_H

#include <nng/nng.h>
#include "include/packet.h"
#include "apps/broker.h"

uint8_t decode_sub_message(nng_msg *, packet_subscribe *);
uint8_t encode_suback_message(nng_msg *, packet_subscribe *);
void sub_ctx_handle(emq_work *);
uint8_t subscribe_handle(nng_msg *);

#endif
