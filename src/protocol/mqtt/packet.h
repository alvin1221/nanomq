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

typedef enum Packet_qos Packet_qos;
typedef enum Control_packet_type Control_packet_type;
typedef enum Protocol_version Protocol_version;

typedef struct mqtt_packet mqtt_packet;
typedef struct mqtt_packet_header mqtt_packet_header;
typedef struct mqtt_string mqtt_string;
typedef struct mqtt_string_list mqtt_string_list;
typedef struct mqtt_binary mqtt_binary;
typedef struct mqtt_property mqtt_property;
typedef struct property property;

typedef struct mqtt_packet_connect mqtt_packet_connect;
typedef struct mqtt_packet_connack mqtt_packet_connack;
typedef struct mqtt_packet_publish mqtt_packet_publish;
typedef struct mqtt_packet_puback mqtt_packet_puback;
typedef struct mqtt_packet_subscribe mqtt_packet_subscribe;
typedef struct mqtt_packet_suback mqtt_packet_suback;
typedef struct mqtt_packet_unsubscribe mqtt_packet_unsubscribe;
typedef struct mqtt_packet_unsuback mqtt_packet_unsuback;
typedef struct mqtt_packet_disconnect mqtt_packet_disconnect;
typedef struct mqtt_packet_auth mqtt_packet_auth;

typedef struct mqtt_payload_connect mqtt_payload_connect;
typedef struct mqtt_payload_publish mqtt_payload_publish;
typedef struct mqtt_payload_subscribe mqtt_payload_subscribe;
typedef struct mqtt_payload_suback mqtt_payload_suback;
typedef struct mqtt_payload_unsubscribe mqtt_payload_unsubscribe;
typedef struct mqtt_payload_unsuback mqtt_payload_unsuback;

enum Control_packet_type {
    Reserved = 0,   CONNECT,    CONNACK,    PUBLISH, 
    PUBACK,         PUBREC,     PUBREL,     PUBCOMP, 
    SUBSCRIBE,      SUBACK,     UNSUBSCRIBE,UNSUBACK, 
    PINGREQ,        PINGRESP,   DISCONNECT, Reserved
};

enum Packet_qos {
    QOS_0 = 0, QOS_1, QOS_2
};

enum Protocol_version {
    MQTT_PROTO_V4 = 4, // 3.1.1
    MQTT_PROTO_V5
};

// fixed header
struct mqtt_packet_header {
    Control_packet_type     type;
    bool                    dup;
    Packet_qos              qos;
    bool                    retain;
    uint32_t                remain_len;
};

struct mqtt_string {
    char * str;
    int len;
};

struct mqtt_string_list {
    mqtt_string * next;
    mqtt_string * it;
};

struct mqtt_binary {
    unsigned char * str;
    int len;
};

struct property {
    uint32_t id;
    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint32_t varint;
        mqtt_binary * binary;
        mqtt_string * str;
    }value;
    struct property * next;
};

struct mqtt_property {
    uint32_t            len;
    struct property *   property;
};

// variable header in mqtt_packet_connect
struct mqtt_packet_connect {
    mqtt_string *       proto_name;
    Protocol_version    proto_ver;
    bool                is_bridge;
    bool                clean_start;
    bool                will_flag;
    Packet_qos          will_qos;
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
struct mqtt_packet_subscribe {
    uint16_t        packet_id;
    struct mqtt_property * property;
};

struct mqtt_payload_subscribe {
    mqtt_string *   topic_filter;
    bool            no_local; //bit2
    bool            retain; //bit3
    uint8_t         retain_option; //bit45
};

//variable header in mqtt_apcket_suback
struct mqtt_packet_suback {
    uint16_t        packet_id;
    struct mqtt_property * property;
};

struct mqtt_payload_suback {
    mqtt_binary *   reason_code_list; //each byte->topic_filter in order
};

//variable header in mqtt_packet_unsubscribe
struct mqtt_packet_unsubscribe {
    uint16_t        packet_id;
    struct mqtt_property * property;
};

struct mqtt_payload_unsubscribe {
    mqtt_string_list * topic_filter;
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

// packet
struct mqtt_packet {
    mqtt_header *   header;
    union {
        mqtt_packet_connect;
        mqtt_packet_connack;
        mqtt_packet_publish;
        mqtt_packet_puback;
        mqtt_packet_subscribe;
        mqtt_packet_suback;
        mqtt_packet_unsubscribe;
        mqtt_packet_unsuback;
        mqtt_packet_disconnect;
        mqtt_packet_auth;
    }variable;
    union {
        mqtt_payload_connect;
        mqtt_payload_publish;
        mqtt_payload_subscribe;
        mqtt_payload_suback;
        mqtt_payload_unsubscribe;
        mqtt_payload_unsuback;
    }payload;
};

