/**
  * Created by Alvin on 2020/7/25.
  */

#ifndef NNG_PUB_HANDLER_H
#define NNG_PUB_HANDLER_H

#include <nng/nng.h>

typedef uint32_t variable_integer;

struct variable_string {
	uint32_t str_len;
	char *str_body;
};

struct variable_binary {
	uint32_t data_len;
	uint8_t *data;
};

//MQTT Control Packet types
typedef enum {
	RESERVED = 0,
	CONNECT = 1,
	CONNACK = 2,
	PUBLISH = 3,
	PUBACK = 4,
	PUBREC = 5,
	PUBREL = 6,
	PUBCOMP = 7,
	SUBSCRIBE = 8,
	SUBACK = 9,
	UNSUBSCRIBE = 10,
	UNSUBACK = 11,
	PINGREQ = 12,
	PINGRESP = 13,
	DISCONNECT = 14,
	AUTH = 15
} mqtt_control_packet_types;


//MQTT Fixed header
struct fixed_header {
	//flag_bits
	union {
		uint8_t bit0: 1;
		uint8_t retain: 1;
	};
	union {
		uint8_t bit1: 1;
		uint8_t bit2: 1;
		uint8_t qos: 2;
	};
	union {
		uint8_t bit3: 1;
		uint8_t dup: 1;
	};
	//packet_types
	mqtt_control_packet_types packet_type: 4;
	//remaining length
	uint32_t remain_len;
};

typedef enum {
	SUCCESS = 0,
	NORMAL_DISCONNECTION = 0,
	GRANTED_QOS_0 = 0,
	GRANTED_QOS_1 = 1,
	GRANTED_QOS_2 = 2,
	DISCONNECT_WITH_WILL_MESSAGE = 4,
	NO_MATCHING_SUBSCRIBERS = 16,
	NO_SUBSCRIPTION_EXISTED = 17,
	CONTINUE_AUTHENTICATION = 24,
	RE_AUTHENTICATE = 25,
	UNSPECIFIED_ERROR = 128,
	MALFORMED_PACKET = 129,
	PROTOCOL_ERROR = 130,
	IMPLEMENTATION_SPECIFIC_ERROR = 131,
	UNSUPPORTED_PROTOCOL_VERSION = 132,
	CLIENT_IDENTIFIER_NOT_VALID = 133,
	BAD_USER_NAME_OR_PASSWORD = 134,
	NOT_AUTHORIZED = 135,
	SERVER_UNAVAILABLE = 136,
	SERVER_BUSY = 137,
	BANNED = 138,
	SERVER_SHUTTING_DOWN = 139,
	BAD_AUTHENTICATION_METHOD = 140,
	KEEP_ALIVE_TIMEOUT = 141,
	SESSION_TAKEN_OVER = 142,
	TOPIC_FILTER_INVALID = 143,
	TOPIC_NAME_INVALID = 144,
	PACKET_IDENTIFIER_IN_USE = 145,
	PACKET_IDENTIFIER_NOT_FOUND = 146,
	RECEIVE_MAXIMUM_EXCEEDED = 147,
	TOPIC_ALIAS_INVALID = 148,
	PACKET_TOO_LARGE = 149,
	MESSAGE_RATE_TOO_HIGH = 150,
	QUOTA_EXCEEDED = 151,
	ADMINISTRATIVE_ACTION = 152,
	PAYLOAD_FORMAT_INVALID = 153,
	RETAIN_NOT_SUPPORTED = 154,
	QOS_NOT_SUPPORTED = 155,
	USE_ANOTHER_SERVER = 156,
	SERVER_MOVED = 157,
	SHARED_SUBSCRIPTIONS_NOT_SUPPORTED = 158,
	CONNECTION_RATE_EXCEEDED = 159,
	MAXIMUM_CONNECT_TIME = 160,
	SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED = 161,
	WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED = 162

} reason_code;

typedef enum {
	PAYLOAD_FORMAT_INDICATOR = 1,
	MESSAGE_EXPIRY_INTERVAL = 2,
	CONTENT_TYPE = 3,
	RESPONSE_TOPIC = 8,
	CORRELATION_DATA = 9,
	SUBSCRIPTION_IDENTIFIER = 11,
	SESSION_EXPIRY_INTERVAL = 17,
	ASSIGNED_CLIENT_IDENTIFIER = 18,
	SERVER_KEEP_ALIVE = 19,
	AUTHENTICATION_METHOD = 21,
	AUTHENTICATION_DATA = 22,
	REQUEST_PROBLEM_INFORMATION = 23,
	WILL_DELAY_INTERVAL = 24,
	REQUEST_RESPONSE_INFORMATION = 25,
	RESPONSE_INFORMATION = 26,
	SERVER_REFERENCE = 28,
	REASON_STRING = 31,
	RECEIVE_MAXIMUM = 33,
	TOPIC_ALIAS_MAXIMUM = 34,
	TOPIC_ALIAS = 35,
	PUBLISH_MAXIMUM_QOS = 36,
	RETAIN_AVAILABLE = 37,
	USER_PROPERTY = 38,
	MAXIMUM_PACKET_SIZE = 39,
	WILDCARD_SUBSCRIPTION_AVAILABLE = 40,
	SUBSCRIPTION_IDENTIFIER_AVAILABLE = 41,
	SHARED_SUBSCRIPTION_AVAILABLE = 42
} properties_type;

struct property {
	properties_type type: 32;
	uint32_t len;
	void *data; //Dynamic alloc memory, need to call the function "nng_free(void *buf, size_t sz)" to free;
};

//Special for publish message data structure
struct property_content {
	uint8_t payload_fmt_indicator;
	uint32_t msg_expiry_interval;
	uint16_t topic_alias;
	struct variable_string response_topic;
	struct variable_binary correlation_data;
	struct variable_string user_property;
	variable_integer subscription_identifier;
	struct variable_string content_type;
};

//#define PUBLISH_PROPERTIES_TOTAL 10

//Properties
struct properties {
	uint32_t len; //property length, exclude itself,variable byte integer;
//    struct property prop_content[PUBLISH_PROPERTIES_TOTAL];
	struct property_content content;
};


//MQTT Variable header
struct variable_header {
	union {
		uint16_t packet_identifier;
		struct variable_string topic_name;
		struct properties properties;
	} publish;

	union {
		uint16_t packet_identifier;
		reason_code reason_code;
		struct properties properties;
	} puback;

	union {

	}pubrec;

	union {

	}pubrel;

	union {

	} pubcomp ;
};


struct mqtt_payload {
	uint8_t *payload;
	uint32_t payload_len;
};

struct pub_packet_struct {
	struct fixed_header fixed_header;
	struct variable_header variable_header;
	struct mqtt_payload payload_body;

};

void pub_handler(nng_msg *msg);

#endif //NNG_PUB_HANDLER_H
