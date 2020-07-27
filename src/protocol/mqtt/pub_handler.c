/**
  * Created by Alvin on 2020/7/25.
  */


#include "nng/protocol/mqtt/pub_handler.h"
#include <stdio.h>
#include <string.h>
#include <nng/nng.h>
#include "core/nng_impl.h"

uint32_t get_var_integer(const uint8_t *buf, int *pos);

static bool decode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet);

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
        //TODO save some useful publish message info to global mqtt context
        //TODO search clients' pipe from topic_name
        //TODO send payload to clients


    }
}

static bool decode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet)
{

    uint8_t *msg_body = nng_msg_body(msg);
    size_t msg_len = nng_msg_len(msg);

    int pos = 0;

    memcpy((uint8_t *) &pub_packet->fixed_header, msg_body, 1);
    ++pos;
    pub_packet->fixed_header.remain_len = get_var_integer(msg_body, &pos);

    if (pub_packet->fixed_header.remain_len <= msg_len - pos) {
        if (pub_packet->fixed_header.packet_type == PUBLISH) {
            if (pub_packet->fixed_header.qos > 0) {
                //variable header
                //TODO extract Topic Name


                pub_packet->variable_header.packet_identifier = (uint16_t) (*(msg_body + pos) >> 8 |
                                                                            *(msg_body + pos + 1));
                //TODO need to save packet_identifier to mqtt context for PUBACK，PUBREC and PUBREL while QOS > 0
                pos += 2;

                pub_packet->variable_header.properties.property_len = get_var_integer(msg_body, &pos);
                int used_pos = pos;
                if (pub_packet->variable_header.properties.property_len > 0) {

                    pub_packet->variable_header.properties.type = get_var_integer(msg_body, &pos);

                    int property_data_len = pub_packet->variable_header.properties.property_len - (pos - used_pos);
                    //alloc memory for property data
                    pub_packet->variable_header.properties.property = nng_alloc(property_data_len);
                    memcpy(&pub_packet->variable_header.properties.property, msg_body, property_data_len);

                    used_pos += property_data_len;
                }
                //payload
                pub_packet->payload_body.payload = msg_body + pos;
                pub_packet->payload_body.payload_len = msg_len - used_pos;

            } else {
                //TODO handle for qos = 0
            }

            return true;
        } else if (pub_packet->fixed_header.packet_type == PUBACK) {
            //TODO handle PUCACK

        }
    }


    return false;
}


uint32_t get_var_integer(const uint8_t *buf, int *pos)
{
    uint8_t temp = 0;
    uint32_t result = 0;
    uint8_t loop_times = 4;
    do {
        temp = buf[*pos++] & 0x80;
        result = temp | (result >> 7);
    }
    while (temp && --loop_times);
    return result;

}

