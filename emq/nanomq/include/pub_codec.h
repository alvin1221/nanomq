/**
  * Created by Alvin on 2020/7/25.
  */

#ifndef NNG_PUB_CODEC_H
#define NNG_PUB_CODEC_H


#include <stdint.h>

typedef uint32_t variable_length;

//MQTT Control Packet types
typedef enum {
    Reserved = 0,       // 禁止 	保留
    CONNECT = 1,        // 客户端到服务端 	客户端请求连接服务端
    CONNACK = 2,        // 服务端到客户端 	连接报文确认
    PUBLISH = 3,        // 两个方向都允许 	发布消息
    PUBACK = 4,         // 两个方向都允许 	QoS 1消息发布收到确认
    PUBREC = 5,         // 两个方向都允许 	发布收到（保证交付第一步）
    PUBREL = 6,         // 两个方向都允许 	发布释放（保证交付第二步）
    PUBCOMP = 7,        // 两个方向都允许 	QoS 2消息发布完成（保证交互第三步）
    SUBSCRIBE = 8,      // 客户端到服务端 	客户端订阅请求
    SUBACK = 9,         // 服务端到客户端 	订阅请求报文确认
    UNSUBSCRIBE = 10,   // 客户端到服务端 	客户端取消订阅请求
    UNSUBACK = 11,      // 服务端到客户端 	取消订阅报文确认
    PINGREQ = 12,       // 客户端到服务端 	心跳请求
    PINGRESP = 13,      // 服务端到客户端 	心跳响应
    DISCONNECT = 14,    // 两个方向都允许 	断开连接通知
    AUTH = 15           // 两个方向都允许 	认证信息交换
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
    PAYLOAD_FORMAT_INDICATOR = 1,   // 0x01 	载荷格式说明 	字节 	PUBLISH, Will Properties
    MESSAGE_EXPIRY_INTERVAL = 2,   // 0x02 	消息过期时间 	四字节整数 	PUBLISH, Will Properties
    CONTENT_TYPE = 3,   // 0x03 	内容类型 	UTF-8编码字符串 	PUBLISH, Will Properties
    RESPONSE_TOPIC = 8,   // 0x08 	响应主题 	UTF-8编码字符串 	PUBLISH, Will Properties
    CORRELATION_DATA = 9,   // 0x09 	相关数据 	二进制数据 	PUBLISH, Will Properties
    SUBSCRIPTION_IDENTIFIER = 11,   // 0x0B 	定义标识符 	变长字节整数 	PUBLISH, SUBSCRIBE
    SESSION_EXPIRY_INTERVAL = 17,   // 0x11 	会话过期间隔 	四字节整数 	CONNECT, CONNACK, DISCONNECT
    ASSIGNED_CLIENT_IDENTIFIER = 18,   // 0x12 	分配客户标识符 	UTF-8编码字符串 	CONNACK
    SERVER_KEEP_ALIVE = 19,   // 0x13 	服务端保活时间 	双字节整数 	CONNACK
    AUTHENTICATION_METHOD = 21,   // 0x15 	认证方法 	UTF-8编码字符串 	CONNECT, CONNACK, AUTH
    AUTHENTICATION_DATA = 22,   // 0x16 	认证数据 	二进制数据 	CONNECT, CONNACK, AUTH
    REQUEST_PROBLEM_INFORMATION = 23,   // 0x17 	请求问题信息 	字节 	CONNECT
    WILL_DELAY_INTERVAL = 24,   // 0x18 	遗嘱延时间隔 	四字节整数 	Will Properties
    REQUEST_RESPONSE_INFORMATION = 25,   // 0x19 	请求响应信息 	字节CONNECT
    RESPONSE_INFORMATION = 26,   // 0x1A 	请求信息 	UTF-8编码字符串 CONNACK
    SERVER_REFERENCE = 28,   // 0x1C 	服务端参考 	UTF-8编码字符串 	CONNACK, DISCONNECT
    REASON_STRING = 31,   // 0x1F 	原因字符串 	UTF-8编码字符串 	CONNACK, PUBACK, PUBREC, PUBREL, PUBCOMP, SUBACK, UNSUBACK, DISCONNECT, AUTH
    RECEIVE_MAXIMUM = 33,   // 0x21 	接收最大数量 	双字节整数 	CONNECT, CONNACK
    TOPIC_ALIAS_MAXIMUM = 34,   // 0x22 	主题别名最大长度 	双字节整数 	CONNECT, CONNACK
    TOPIC_ALIAS = 35,   // 0x23 	主题别名 	双字节整数
    PUBLISH_MAXIMUM_QOS = 36,   // 0x24 	最大QoS 	字节 	CONNACK
    RETAIN_AVAILABLE = 37,   // 0x25 	保留属性可用性 	字节 CONNACK
    USER_PROPERTY = 38,   // 0x26 	用户属性 	UTF-8字符串对 	CONNECT, CONNACK, PUBLISH, Will Properties, PUBACK, PUBREC, PUBREL, PUBCOMP, SUBSCRIBE, SUBACK, UNSUBSCRIBE, UNSUBACK, DISCONNECT, AUTH
    MAXIMUM_PACKET_SIZE = 39,   // 0x27 	最大报文长度 	四字节整数 	CONNECT,CONNACK
    WILDCARD_SUBSCRIPTION_AVAILABLE = 40,   // 0x28 	通配符订阅可用性 	字节  CONNACK
    SUBSCRIPTION_IDENTIFIER_AVAILABLE = 41,   // 0x29 	订阅标识符可用性 	字节 CONNACK
    SHARED_SUBSCRIPTION_AVAILABLE = 42    // 0x2A 	共享订阅可用性 	字节 	CONNACK


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
    SUCCESS = 0,  // 0x00 	成功 	CONNACK, PUBACK, PUBREC, PUBREL, PUBCOMP, UNSUBACK, AUTH
    NORMAL_DISCONNECTION = 0,  // 0x00 	正常断开 	DISCONNECT
    GRANTED_QOS_0 = 0,  // 0x00 	授权的QoS 0 	SUBACK
    GRANTED_QOS_1 = 1,  // 0x01 	授权的QoS 1 	SUBACK
    GRANTED_QOS_2 = 2,  // 0x02 	授权的QoS 2 	SUBACK
    DISCONNECT_WITH_WILL_MESSAGE = 4,  // 0x04 	包含遗嘱的断开 	DISCONNECT
    NO_MATCHING_SUBSCRIBERS = 16, //	0x10 	无匹配订阅 	PUBACK, PUBREC
    NO_SUBSCRIPTION_EXISTED = 17, //	0x11 	订阅不存在 	UNSUBACK
    CONTINUE_AUTHENTICATION = 24, //	0x18 	继续认证 	AUTH
    RE_AUTHENTICATE = 25, //	0x19 	重新认证 	AUTH
    UNSPECIFIED_ERROR = 128, //	0x80 	未指明的错误 	CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT
    MALFORMED_PACKET = 129, //	0x81 	无效报文 	CONNACK, DISCONNECT
    PROTOCOL_ERROR = 130, //	0x82 	协议错误 	CONNACK, DISCONNECT
    IMPLEMENTATION_SPECIFIC_ERROR = 131, //	0x83 	实现错误 	CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT
    UNSUPPORTED_PROTOCOL_VERSION = 132, //	0x84 	协议版本不支持 	CONNACK
    CLIENT_IDENTIFIER_NOT_VALID = 133, //	0x85 	客户标识符无效 	CONNACK
    BAD_USER_NAME_OR_PASSWORD = 134, //	0x86 	用户名密码错误 	CONNACK
    NOT_AUTHORIZED = 135, //	0x87 	未授权 	CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT
    SERVER_UNAVAILABLE = 136, //	0x88 	服务端不可用 	CONNACK
    SERVER_BUSY = 137, //	0x89 	服务端正忙 	CONNACK, DISCONNECT
    BANNED = 138, //	0x8A 	禁止 	CONNACK
    SERVER_SHUTTING_DOWN = 139, //	0x8B 	服务端关闭中 	DISCONNECT
    BAD_AUTHENTICATION_METHOD = 140, //	0x8C 	无效的认证方法 	CONNACK, DISCONNECT
    KEEP_ALIVE_TIMEOUT = 141, //	0x8D 	保活超时 	DISCONNECT
    SESSION_TAKEN_OVER = 142, //	0x8E 	会话被接管 	DISCONNECT
    TOPIC_FILTER_INVALID = 143, //	0x8F 	主题过滤器无效 	SUBACK, UNSUBACK, DISCONNECT
    TOPIC_NAME_INVALID = 144, //	0x90 	主题名无效 	CONNACK, PUBACK, PUBREC, DISCONNECT
    PACKET_IDENTIFIER_IN_USE = 145, //	0x91 	报文标识符已被占用 	PUBACK, PUBREC, SUBACK, UNSUBACK
    PACKET_IDENTIFIER_NOT_FOUND = 146, //	0x92 	报文标识符无效 	PUBREL, PUBCOMP
    RECEIVE_MAXIMUM_EXCEEDED = 147, //	0x93 	接收超出最大数量 	DISCONNECT
    TOPIC_ALIAS_INVALID = 148, //	0x94 	主题别名无效 	DISCONNECT
    PACKET_TOO_LARGE = 149, //	0x95 	报文过长 	CONNACK, DISCONNECT
    MESSAGE_RATE_TOO_HIGH = 150, //	0x96 	消息太过频繁 	DISCONNECT
    QUOTA_EXCEEDED = 151, //	0x97 	超出配额 	CONNACK, PUBACK, PUBREC, SUBACK, DISCONNECT
    ADMINISTRATIVE_ACTION = 152, //	0x98 	管理行为 	DISCONNECT
    PAYLOAD_FORMAT_INVALID = 153, //	0x99 	载荷格式无效 	CONNACK, PUBACK, PUBREC, DISCONNECT
    RETAIN_NOT_SUPPORTED = 154, //	0x9A 	不支持保留 	CONNACK, DISCONNECT
    QOS_NOT_SUPPORTED = 155, //	0x9B 	不支持的QoS等级 	CONNACK, DISCONNECT
    USE_ANOTHER_SERVER = 156, //	0x9C 	（临时）使用其他服务端 	CONNACK, DISCONNECT
    SERVER_MOVED = 157, //	0x9D 	服务端已（永久）移动 	CONNACK, DISCONNECT
    SHARED_SUBSCRIPTIONS_NOT_SUPPORTED = 158, //	0x9E 	不支持共享订阅 	SUBACK, DISCONNECT
    CONNECTION_RATE_EXCEEDED = 159, //	0x9F 	超出连接速率限制 	CONNACK, DISCONNECT
    MAXIMUM_CONNECT_TIME = 160, //	0xA0 	最大连接时间 	DISCONNECT
    SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED = 161, //	0xA1 	不支持订阅标识符 	SUBACK, DISCONNECT
    WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED = 162  //	0xA2 	不支持通配符订阅 	SUBACK, DISCONNECT

} reason_code;

struct pub_packet_struct {
    struct fixed_header fixed_header;
    struct variable_header variable_header;
    uint8_t *payload;
};

#endif //NNG_PUB_CODEC_H
