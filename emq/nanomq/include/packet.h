//
// Copyright 2020 Staysail Systems, Inc. <info@staysail.tech>
// Copyright 2018 Capitar IT Group BV <info@capitar.com>
// Copyright 2019 Devolutions <info@devolutions.net>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//
// The Struct to store mqtt_packet. 

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* fixed header
struct mqtt_packet_header {
    bool                    dup;
    Packet_qos              qos;
    bool                    retain;
    uint32_t                remain_len;
};
*/

struct mqtt_string {
	char *  str;
	int     len;
};
typedef struct mqtt_string mqtt_string;

struct mqtt_string_node {
	struct mqtt_string_node *  next;
	mqtt_string *       it;
};
typedef struct mqtt_string_node mqtt_string_node;

struct mqtt_binary {
	unsigned char * str;
	int             len;
};
typedef struct mqtt_binary mqtt_binary;

struct mqtt_str_pair {
	char *	str1; // key
	int 	len1;
	char *	str2; // value
	int 	len2;
};
typedef struct mqtt_str_pair mqtt_str_pair;

union Property_type{
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint32_t varint;
	mqtt_binary binary;
	mqtt_string str;
	mqtt_str_pair strpair;
};

struct property {
	uint8_t 			id;
	union Property_type	value;
	struct property * 	next;
};
typedef struct property property;

struct mqtt_property {
	uint32_t            len;
	uint32_t			count;
	struct property *   property;
	struct property *	property_end;
};
typedef struct mqtt_property mqtt_property;

// variable header in mqtt_packet_connect
struct mqtt_packet_connect {
	mqtt_string *       proto_name;
	uint8_t             proto_ver;
	bool                is_bridge;
	bool                clean_start;
	bool                will_flag;
	uint8_t             will_qos;
	bool                will_retain;
	uint16_t            keep_alive;
	mqtt_property *     property;
	bool                has_username;
	bool                has_password;
};

struct mqtt_payload_connect {
	mqtt_string *       client_id; // 1-23 bytes[0-9a-zA-Z]
	mqtt_property *     will_property; //app msg included
	mqtt_string *       will_topic;
	mqtt_binary *       will_payload;
	mqtt_string *       username;
	mqtt_binary *       password; //
};

//variable header in mqtt_packet_connack
struct mqtt_packet_connack {
	bool                session_present;
	uint8_t             reason_code;
	struct mqtt_property * property;
};

//struct mqtt_payload_connack {} = NULL;

//variable header in mqtt_packet_publish
struct mqtt_packet_publish {
	mqtt_string *   topic;
	uint16_t        packet_id;
	mqtt_property * property;
};

struct mqtt_payload_publish {
	mqtt_binary *   msg;
};

//variable header in mqtt_apcket_puback
struct mqtt_packet_puback {
	uint16_t        packet_id;
	uint8_t         reason_code;
	struct mqtt_property * property;
};

//struct mqtt_payload_puback {} = NULL;

//variable header in mqtt_packet_subscribe
struct topic_with_option {
	mqtt_string *   topic_filter;
	bool            no_local; //bit2
	bool            retain; //bit3
	uint8_t         retain_option; //bit45
};
typedef struct topic_with_option topic_with_option;

struct topic_node {
	topic_with_option * it;
	struct topic_node * next;
};	
typedef struct topic_node topic_node;

struct packet_subscribe {
	uint16_t packet_id;
	struct mqtt_property * property;
	topic_node * node;
};
typedef struct packet_subscribe packet_subscribe;

//variable header in mqtt_packet_unsubscribe
struct mqtt_packet_unsubscribe {
	uint16_t        packet_id;
	struct mqtt_property * property;
};

struct mqtt_payload_unsubscribe {
	struct mqtt_string_node * topic_filter;
	int count;
};

//variable header in mqtt_packet_unsuback
struct mqtt_packet_unsuback {
	uint16_t        packet_id;
	struct mqtt_property * property;
};

struct mqtt_payload_unsuback {
	mqtt_binary *   reason_code_list; //each byte->topic_filter in order
};

// variable header in mqtt_packet_disconnect 
struct mqtt_packet_disconnect {
	uint8_t                 reason_code;
	struct mqtt_property *  property;
};

// struct mqtt_payload_disconnect {} = NULL;

//variable header in mqtt_packet_auth
struct mqtt_packet_auth {
	uint8_t                 auth_reason_code;
	struct mqtt_property *  property;
};

// struct mqtt_payload_auth {} = NULL;

// typedef struct mqtt_packet_header mqtt_packet_header;

typedef struct mqtt_packet_connect mqtt_packet_connect;
typedef struct mqtt_packet_connack mqtt_packet_connack;
typedef struct mqtt_packet_publish mqtt_packet_publish;
typedef struct mqtt_packet_puback mqtt_packet_puback;
typedef struct mqtt_packet_unsubscribe mqtt_packet_unsubscribe;
typedef struct mqtt_packet_unsuback mqtt_packet_unsuback;
typedef struct mqtt_packet_disconnect mqtt_packet_disconnect;
typedef struct mqtt_packet_auth mqtt_packet_auth;

typedef struct mqtt_payload_connect mqtt_payload_connect;
typedef struct mqtt_payload_publish mqtt_payload_publish;
typedef struct mqtt_payload_unsubscribe mqtt_payload_unsubscribe;
typedef struct mqtt_payload_unsuback mqtt_payload_unsuback;

// ctx of subscribe
struct Ctx_sub {
	uint32_t	id; // client id
	struct mqtt_property * variable_property;
	struct topic_with_option * topic_with_option;

	// connect info
	// struct ctx_connect * ctx_con;
};
typedef struct Ctx_sub Ctx_sub;


uint32_t bin_parse_varint(uint8_t * bin_pos, int * pos){
	*pos = 0;
	uint32_t res = 0;
	uint32_t multiplier = 1;
	uint8_t byte;
	do {
		byte = *bin_pos++;
		res += (byte & 127) * multiplier;
		multiplier *= 128;
		(*pos)++;
	} while (*pos < 4 && (byte & 128));
	return res;
}

int len_of_varint(uint32_t num){
	int tmp = num, res = 0;
	while(tmp>0){
		res ++;
		tmp >> 8;
	}
	return res;
}

