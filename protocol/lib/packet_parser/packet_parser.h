#ifndef PACKET_PAESER
#define PACKET_PARSER

#include <string.h>



typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef signed char int8_t;


struct header{
    
}


struct mqtt_packet {
    uint8_t *binary;
    uint32_t pos;
    uint32_t len;
};

struct mqtt_packet a;
a.binary = variable;
a.pos = 0;
a.len = 20;

string t = paclet_parse_uint8(&packet);
int y = packet_parse_uint8(&packet);

uint8_t packet_parse_uint8(struct mqtt_packet *packet); 
uint16_t packet_parse_uint16(struct mqtt_packet *packet); 
uint32_t packet_parse_uint32(struct mqtt_packet *packet); 
void packet_parse_string(struct mqtt_packet *packet, char str[], uint32_t length);


void packet_write_uint8(struct mqtt_packet *packet, uint8_t input);
void packet_write_uint16(struct mqtt_packet *packet, uint16_t input);
void packet_write_uint32(struct mqtt_packet * packet, uint32_t intput);
void packet_write_string(struct mqtt_packet *packet, char str[], uint32_t length);

#endif
