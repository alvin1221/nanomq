/**
  * Created by Alvin on 2020/7/25.
  */


#include "nng/protocol/mqtt/pub_handler.h"
#include <stdio.h>
#include <string.h>
#include <nng/nng.h>
#include "core/nng_impl.h"

uint32_t get_var_integer(const uint8_t *buf, int *pos);

static bool encode_pub_message(uint8_t *dest, struct pub_packet_struct *pub_packet);

static bool decode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet);

static int32_t get_utf8_str(char *dest, const uint8_t *src, int *pos);

static int utf8_check(const char *str, size_t length);

uint32_t get_uint16(const uint8_t *buf);

uint32_t get_uint32(const uint8_t *buf);

uint64_t get_uint64(const uint8_t *buf);

uint16_t get_variable_binary(uint8_t *dest, const uint8_t *src);


/**
 * publish logic
 *
 * @param msg
 * @param pub_packet
 */
void pub_handler(nng_msg *msg)
{
    struct pub_packet_struct pub_packet;

    if (decode_pub_message(msg, &pub_packet)) {
        //TODO save some useful publish message info and properties to global mqtt context while decode succeed
        //TODO search clients' pipe from topic_name
        //TODO do publish actions, eq: send payload to clients dependent on QoS ,topic alias if exists
    }
}

static bool encode_pub_message(uint8_t *dest, struct pub_packet_struct *pub_packet)
{

    return true;
}

static bool decode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet)
{
    uint8_t *msg_body = nng_msg_body(msg);
    size_t msg_len = nng_msg_len(msg);

    int pos = 0;
    int temp_pos = 0;

    memcpy((uint8_t *) &pub_packet->fixed_header, msg_body, 1);
    ++pos;
    pub_packet->fixed_header.remain_len = get_var_integer(msg_body, &pos);

    if (pub_packet->fixed_header.remain_len <= msg_len - pos) {

        if (pub_packet->fixed_header.packet_type == PUBLISH) {

            //variable header
            //TODO deal with topic alias
            int topic_len = get_utf8_str(pub_packet->variable_header.topic_name.str_body, msg_body + pos, &pos);
            if (topic_len < 0 ||
                strstr(pub_packet->variable_header.topic_name.str_body, "+") ||
                strstr(pub_packet->variable_header.topic_name.str_body, "#")) {
                //protocol error
                return false;
            }
            pub_packet->variable_header.topic_name.str_len = topic_len;

            if (pub_packet->fixed_header.qos > 0) { //extract packet_identifier while qos > 0
                pub_packet->variable_header.packet_identifier = get_uint16(msg_body + pos);
                pos += 2;
            }

            pub_packet->variable_header.properties.len = get_var_integer(msg_body, &pos);
            int used_pos = pos;
            if (pub_packet->variable_header.properties.len > 0) {
                for (uint32_t i = 0; i < pub_packet->variable_header.properties.len;) {
                    properties_type prop_type = get_var_integer(msg_body, &pos);
                    switch (prop_type) {
                        case PAYLOAD_FORMAT_INDICATOR:
                            pub_packet->variable_header.properties.content.payload_fmt_indicator = *(msg_body + pos);
                            ++pos;
                            ++i;
                            break;

                        case MESSAGE_EXPIRY_INTERVAL:
                            pub_packet->variable_header.properties.content.msg_expiry_interval = get_uint32(
                                    msg_body + pos);
                            pos += 4;
                            i += 4;
                            break;

                        case CONTENT_TYPE:
                            pub_packet->variable_header.properties.content.content_type.str_len = get_utf8_str(
                                    pub_packet->variable_header.properties.content.content_type.str_body, msg_body,
                                    &pos);
                            i = i + pub_packet->variable_header.properties.content.content_type.str_len + 2;
                            break;

                        case TOPIC_ALIAS:
                            pub_packet->variable_header.properties.content.topic_alias = get_uint16(msg_body + pos);
                            pos += 2;
                            i += 2;

                            if (pub_packet->variable_header.properties.content.topic_alias == 0) {
                                //TODO protocol error, free memory and return ?
//                                return false;
                            }

                            break;

                        case RESPONSE_TOPIC:
                            pub_packet->variable_header.properties.content.response_topic.str_len = get_utf8_str(
                                    pub_packet->variable_header.properties.content.response_topic.str_body, msg_body,
                                    &pos);
                            i = i + pub_packet->variable_header.properties.content.content_type.str_len + 2;

                            break;

                        case CORRELATION_DATA:
                            pub_packet->variable_header.properties.content.correlation_data.data_len = get_variable_binary(
                                    pub_packet->variable_header.properties.content.correlation_data.data,
                                    msg_body + pos);
                            pos += pub_packet->variable_header.properties.content.correlation_data.data_len + 2;
                            i += pub_packet->variable_header.properties.content.correlation_data.data_len + 2;

                            break;

                        case USER_PROPERTY:
                            pub_packet->variable_header.properties.content.response_topic.str_len = get_utf8_str(
                                    pub_packet->variable_header.properties.content.user_property.str_body, msg_body,
                                    &pos);
                            i = i + pub_packet->variable_header.properties.content.user_property.str_len + 2;
                            break;

                        case SUBSCRIPTION_IDENTIFIER:
                            temp_pos = pos;
                            pub_packet->variable_header.properties.content.subscription_identifier = get_var_integer(
                                    msg_body, &pos);
                            i += (pos - temp_pos);
                            break;

                        default:
                            i++;
                            break;
                    }
                }
            }

            //payload
            pub_packet->payload_body.payload_len = msg_len - used_pos;
            pub_packet->payload_body.payload = nni_alloc(pub_packet->payload_body.payload_len);
            memcpy(pub_packet->payload_body.payload, msg_body + pos, pub_packet->payload_body.payload_len);

            return true;
        } else if (pub_packet->fixed_header.packet_type == PUBACK) {
            //TODO handle PUCACK

        } else if (pub_packet->fixed_header.packet_type == PUBREC) {

        } else if (pub_packet->fixed_header.packet_type == PUBREL) {

        } else if (pub_packet->fixed_header.packet_type == PUBCOMP) {

        }
    }


    return false;
}

/**
 * Get variable integer value
 *
 * @param buf Byte array
 * @param pos
 * @return Integer value
 */
uint32_t get_var_integer(const uint8_t *buf, int *pos)
{
    uint8_t temp;
    uint32_t result = 0;
    uint8_t loop_times = 4;
    int p = *pos;

    do {
        temp = *(buf + (p++));
        result = (temp & 0x7F) | (result * 128);
    }
    while ((temp & 0x80) && --loop_times);
    *pos = p;
    return result;
}

uint32_t get_uint32(const uint8_t *buf)
{
    return (((uint32_t) ((uint8_t) (buf)[0])) >> 24u) +
           (((uint32_t) ((uint8_t) (buf)[+1])) >> 16u) +
           (((uint32_t) ((uint8_t) (buf)[+2])) >> 8u) +
           (((uint32_t) (uint8_t) (buf)[+3]));
}

uint64_t get_uint64(const uint8_t *buf)
{
    return (((uint64_t) ((uint8_t) (buf)[0])) >> 56u) +
           (((uint64_t) ((uint8_t) (buf)[1])) >> 48u) +
           (((uint64_t) ((uint8_t) (buf)[2])) >> 40u) +
           (((uint64_t) ((uint8_t) (buf)[3])) >> 32u) +
           (((uint64_t) ((uint8_t) (buf)[4])) >> 24u) +
           (((uint64_t) ((uint8_t) (buf)[5])) >> 16u) +
           (((uint64_t) ((uint8_t) (buf)[6])) >> 8u) +
           (((uint64_t) (uint8_t) (buf)[7]));
}

uint32_t get_uint16(const uint8_t *buf)
{
    return (((uint32_t) ((uint8_t) (buf)[0])) >> 8u) +
           (((uint32_t) (uint8_t) (buf)[1]));
}

uint16_t get_variable_binary(uint8_t *dest, const uint8_t *src)
{
    uint16_t len = get_uint16(src);
    dest = nni_alloc(len);

    memcpy(dest, src + 2, len);
    return len;
}

/**
 * Get utf-8 string
 *
 * @param dest output string
 * @param src input bytes
 * @param pos
 * @return string length -1: not utf-8, 0: empty string, >0 : normal utf-8 string
 */
static int32_t get_utf8_str(char *dest, const uint8_t *src, int *pos)
{
    int32_t str_len = get_uint16(src + *pos);
    dest = nng_alloc(str_len); //Do not forget to free
    *pos = (*pos) + 2;
    if (str_len > 0) {
        if (utf8_check((const char *) (src + *pos), str_len)) {
            strncpy(dest, (char *) (src + *pos), str_len);
            *pos = (*pos) + str_len;
        } else {
            str_len = -1;
        }
    }
    return str_len;
}

static int utf8_check(const char *str, size_t length)
{
    size_t i = 0;
    int nBytes = 0;////UTF8可用1-6个字节编码,ASCII用一个字节
    unsigned char ch = 0;
    bool bAllAscii = true;//如果全部都是ASCII,说明不是UTF-8
    while (i < length) {
        ch = *(str + i);
        if ((ch & 0x80) != 0)
            bAllAscii = false;
        if (nBytes == 0) {
            if ((ch & 0x80) != 0) {
                while ((ch & 0x80) != 0) {
                    ch <<= 1;
                    nBytes++;
                }
                if ((nBytes < 2) || (nBytes > 6)) {
                    return 0;
                }
                nBytes--;
            }
        } else {
            if ((ch & 0xc0) != 0x80) {
                return 0;
            }
            nBytes--;
        }
        i++;
    }
    if (bAllAscii)
        return false;
    return (nBytes == 0);
}