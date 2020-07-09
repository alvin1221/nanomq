#ifndef __CRC_H

#define __CRC_H


#include <string.h>
#include <stdio.h>

extern unsigned char result[2];

//void Getccitt16(uint8_t buffer[],uint32_t start,uint32_t length);

unsigned short getCRC(unsigned char *buffer,int length);
unsigned char * getccitt16(unsigned char* buffer, int start, int length);
unsigned short crc16(unsigned char *buf,unsigned short length);
unsigned short update_crc_ccitt( unsigned short crc, char c );

#endif

 
