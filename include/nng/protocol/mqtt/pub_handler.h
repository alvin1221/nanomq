/**
  * Created by Alvin on 2020/7/25.
  */

#ifndef NNG_PUB_HANDLER_H
#define NNG_PUB_HANDLER_H

#include <nng/nng.h>
#include "nng/protocol/mqtt/mqtt.h"

typedef uint32_t variable_integer;


struct variable_string {
	uint32_t str_len;
	char *str_body;
};

struct variable_binary {
	uint32_t data_len;
	uint8_t *data;
};



//MQTT Fixed header
struct fixed_header {
	//flag_bits
	uint8_t retain: 1;
	uint8_t qos: 2;
	uint8_t dup: 1;
	//packet_types
	mqtt_control_packet_types packet_type: 4;
	//remaining length
	uint32_t remain_len;
};

//Special for publish message data structure
union property_content {
    struct {
        uint8_t payload_fmt_indicator;
        uint32_t msg_expiry_interval;
        uint16_t topic_alias;
        struct variable_string response_topic;
        struct variable_binary correlation_data;
        struct variable_string user_property;
        variable_integer subscription_identifier;
        struct variable_string content_type;
    } publish;
    struct {
        struct variable_string reason_string;
        struct variable_string user_property;
    } pub_arrc, puback, pubrec, pubrel, pubcomp;
};

//Properties
struct properties {
	uint32_t len; //property length, exclude itself,variable byte integer;
//    struct property prop_content[PUBLISH_PROPERTIES_TOTAL];
	union property_content content;
};


//MQTT Variable header
union variable_header {
	struct {
		uint16_t packet_identifier;
		struct variable_string topic_name;
		struct properties properties;
	} publish;

	struct {
		uint16_t packet_identifier;
		reason_code reason_code: 8;
		struct properties properties;
	} pub_arrc, puback, pubrec, pubrel, pubcomp;
};


struct mqtt_payload {
	uint8_t *payload;
	uint32_t payload_len;
};

struct pub_packet_struct {
	struct fixed_header fixed_header;
	union variable_header variable_header;
	struct mqtt_payload payload_body;

};

void pub_handler(nng_msg *msg);

bool encode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet);

bool decode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet);

#endif //NNG_PUB_HANDLER_H
