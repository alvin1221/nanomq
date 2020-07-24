#include "packet_parser.h"
#include <assert.h>
#include <stdio.h>


uint8_t packet_parse_uint8(struct mqtt_packet *packet) 
{
    assert(packet && packet->remaining_length >= packet->pos+1);
    return packet->payload[packet->pos++];
}

uint16_t packet_parse_uint16(struct mqtt_packet *packet) 
{
    assert(packet && packet->remaining_length >= packet->pos+2);
    uint16_t res = packet->payload[packet->pos++];
    return (res << 8) + packet->payload[packet->pos++];
}

uint32_t packet_parse_uint32(struct mqtt_packet *packet) 
{
    assert(packet && packet->remaining_length >= packet->pos+4);
    uint32_t res = packet->payload[packet->pos++];

    if (res) {
        while ((0xFF000000 & res) == 0) {
            res = (res << 8) + packet->payload[packet->pos++];
        }
    } else {
        int i = 4;
        while (--i > 0) {
            res = (res << 8) + packet->payload[packet->pos++];
        }
    }

    return res;
}

void packet_parse_string(struct mqtt_packet *packet, char str[], uint32_t length)
{
    assert(packet && packet->remaining_length >= packet->pos+length);
    memcpy(str, &(packet->payload[packet->pos]), length);
    packet->pos += length;
}


void packet_write_uint8(struct mqtt_packet *packet, uint8_t input)
{
    assert(packet && packet->remaining_length >= packet->pos+1);
    packet->payload[packet->pos++] = input;
}

void packet_write_uint16(struct mqtt_packet *packet, uint16_t input)
{
    assert(packet && packet->remaining_length >= packet->pos+2);
    packet->payload[packet->pos++] = (input & 0xFF00) >> 8;
    packet->payload[packet->pos++] = input & 0x00FF;
}

void packet_write_uint32(struct mqtt_packet *packet, uint32_t input)
{
    assert(packet && packet->remaining_length >= packet->pos+4);
    packet->payload[packet->pos++] = (input & 0xFF000000) >> 24;
    packet->payload[packet->pos++] = (input & 0x00FF0000) >> 16;
    packet->payload[packet->pos++] = (input & 0x0000FF00) >> 8;
    packet->payload[packet->pos++] = (input & 0x000000FF);
}

void packet_write_string(struct mqtt_packet *packet, char str[], uint32_t length) 
{
    assert(packet && packet->remaining_length >= packet->pos+length);
    memcpy(&(packet->payload[packet->pos]), str, length);
    packet->pos += length;
}





