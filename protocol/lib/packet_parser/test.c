#include "packet_parser.h"
#include <stdio.h>
#include <stdlib.h>

/* =======================================================================
 * TEST
 * =======================================================================*/

static void uint8_write_read(
		uint8_t input,
		int remaining_length)
{
	struct mqtt_packet packet;

	memset(&packet, 0, sizeof(struct mqtt_packet));

	packet.remaining_length = remaining_length;
    packet.payload = (uint8_t *)malloc(remaining_length*sizeof(uint8_t));
    packet_write_uint8(&packet, input);
    packet.pos = 0;

    uint8_t res = packet_parse_uint8(&packet);
    if (input == res) {
        printf("Bingle, uint8  %#X is OK!\n", input);
    }
    free(packet.payload);
    packet.payload = NULL;

}

static void uint16_write_read(uint16_t input, uint32_t remaining_length) 
{
    struct mqtt_packet packet;
    memset(&packet, 0, sizeof(struct mqtt_packet));
    packet.remaining_length = remaining_length;
    packet.payload = (uint8_t *)malloc(remaining_length*sizeof(uint8_t));
    packet_write_uint16(&packet, input);
    packet.pos = 0;
    uint16_t res = packet_parse_uint16(&packet);
    if (input == res) {
        printf("Bingle, uint16 %#X is OK!\n", input);
    }

    free(packet.payload);
    packet.payload = NULL;

}

static void uint32_write_read(uint32_t input, uint32_t remaining_length) 
{
    struct mqtt_packet packet;
    memset(&packet, 0, sizeof(struct mqtt_packet));
    packet.remaining_length = remaining_length;
    packet.payload = (uint8_t *)malloc(remaining_length*sizeof(uint8_t));
    packet_write_uint32(&packet, input);
    packet.pos = 0;

    uint32_t res = packet_parse_uint32(&packet);
    if (input == res) {
        printf("Bingle, uint32 %#X is OK!\n", input);
    }
    free(packet.payload);
    packet.payload = NULL;

}

static void string_write_read(char *input, uint32_t remaining_length)
{
    struct mqtt_packet packet;
    memset(&packet, 0, sizeof(struct mqtt_packet));
    packet.remaining_length = remaining_length;
    packet.payload = (char*)malloc(sizeof(char)*remaining_length);
    packet_write_string(&packet, input, remaining_length); 

    packet.pos = 0;
    char str[remaining_length];
    packet_parse_string(&packet, str, remaining_length);
	printf("\npacket_parse_string output: %s\n",
            str);

    free(packet.payload);
    packet.payload = NULL;

}


static void TEST_uint8_write_read(void)
{
	/* Empty packet */
	// byte_write_read(NULL, 0);

	uint8_t payload = 0;

	/* 0 value */
	// memset(payload, 0, sizeof(payload));
	payload = 0x00;
	uint8_write_read(payload, 1);


	/* Middle */
	//  memset(payload, 0, sizeof(payload));
	payload = 0x1F;
	uint8_write_read(payload, 1);

	/* 255 value */
	// memset(payload, 0, sizeof(payload));
	payload = 0xFF;
	uint8_write_read(payload, 1);

}


static void TEST_uint16_write_read(void)
{

	/* Empty packet */
	// uint16_write_read(NULL, 0);
	uint16_t payload = 0;

	/* 0 value */
	// memset(payload, 0, sizeof(payload));
	payload = 0x0000;
	uint16_write_read(payload, 2);

	/* Endian check */
	// memset(payload, 0, sizeof(payload));
	payload = 0x38F3;
	uint16_write_read(payload, 2);

	/* 65,535 value */
	// memset(payload, 0, sizeof(payload));
	payload = 0xFFFF;
	uint16_write_read(payload, 2);

}

static void TEST_uint32_write_read(void)
{
	/* Empty packet */
	// uint32_write_read(NULL, 0);
	uint32_t payload = 0;

	/* 0 value */
	payload = 0x00000000;
	uint32_write_read(payload, 4);

	/* Endian check */
	payload = 0x12345678;
	uint32_write_read(payload, 4);

	/* Biggest value */
	payload = 0xFFFFFFFF;
	uint32_write_read(payload, 4);

}

static void TEST_string_write_read(void)
{
    // char *payload = "This is a string!";
    char payload[18]; 
    memset(payload, 0, sizeof(payload));
    payload[0] = 'T';
    payload[1] = 'h';
    payload[2] = 'i';
    payload[3] = 's';
    payload[4] = ' ';
    payload[5] = 'i';
    payload[6] = 's';
    payload[7] = ' ';
    payload[8] = 'a';
    payload[9] = ' ';
    payload[10] = 's';
    payload[11] = 't';
    payload[12] = 'r';
    payload[13] = 'i';
    payload[14] = 'n';
    payload[15] = 'g';
    payload[16] = '!';
    payload[17] = '\0';
    string_write_read(payload, 18);
}


void test(void) 
{
    TEST_uint8_write_read();
    TEST_uint16_write_read();
    TEST_uint32_write_read();
    TEST_string_write_read();
}
