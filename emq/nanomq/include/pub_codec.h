/**
  * Created by Alvin on 2020/7/25.
  */

#ifndef NNG_PUB_CODEC_H
#define NNG_PUB_CODEC_H


#include <stdint.h>

typedef uint32_t variable_length;

//MQTT Control Packet types
typedef enum {
    Reserved = 0,
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
    uint8_t flag_bit0: 1;
    uint8_t flag_bit1: 1;
    uint8_t flag_bit2: 1;
    uint8_t flag_bit3: 1;
    mqtt_control_packet_types packet_type: 4;
    uint32_t remain_len; //Remaining Length, Variable Byte Integer, Max field length: 4Bytes
};

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

//Properties
struct properties {
    uint32_t property_len; //property length, exclude itself,variable byte integer;
    properties_type type;
    void *property; //Unknown property data type;
};

//MQTT Variable header
struct variable_header {
    uint16_t packet_identifier;
    struct properties properties;
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

struct pub_packet_struct {
    struct fixed_header fixed_header;
    struct variable_header variable_header;
    uint8_t *payload;
};

#endif //NNG_PUB_CODEC_H
