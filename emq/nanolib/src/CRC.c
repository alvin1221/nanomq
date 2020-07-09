#include "libevpower/CRC.h"

#include <stdlib.h>

int poly = 0x1021;

unsigned char result[2]={0};

static int update_crc(int value, int initcrc)
 {
       int i, v, xor_flag;
        v = 0x80;
        for (i = 0; i < 8; i++)
        {
            if ((initcrc & 0x8000) != 0)
            {
                xor_flag = 1;
            } else
            {
                xor_flag = 0;
            }
            initcrc = initcrc << 1;
            if ((value & v) != 0)
            {
                initcrc = initcrc + 1;
            }
            if (xor_flag != 0)
            {
                initcrc = initcrc ^ poly;
            }
            v = v >> 1;
        }
        return initcrc;
}

static int augment_message_for_crc(int initcrc)
{

        int i, xor_flag;
        for (i = 0; i < 16; i++)
        {
            if ((initcrc & 0x8000) != 0)
            {
                xor_flag = 1;
            } else
            {
                xor_flag = 0;
            }
            initcrc = initcrc << 1;

            if (xor_flag != 0)
            {
                initcrc = initcrc ^ poly;
            }
        }
        return initcrc;
} 

unsigned short getCRC(unsigned char *buffer,int length)
{
	unsigned short crcValue = 0xffff;
	int i=0;
	for(i=0;i<length;i++)
	{
		crcValue = update_crc(buffer[i], crcValue);		
	}
	crcValue = augment_message_for_crc(crcValue);
	return crcValue;
}




unsigned char * getccitt162(unsigned char * buffer,int length)
{
		int initcrc1 = 0xffff;
		//msb is in 0
		int i = 0; 
//		unsigned char  result[2] = {0};

		for ( i = 0; i < length; i++)
		{
			initcrc1 = update_crc(buffer[i], initcrc1);
		}
		initcrc1 = augment_message_for_crc(initcrc1);
		result[0] = (unsigned char) (initcrc1 >> 8);
		result[1] = (unsigned char) initcrc1;
		return result;
}

unsigned char * getccitt16(unsigned char* buffer, int start, int length)
{
	 unsigned char* temp = (unsigned char*)malloc(length);
				//unsigned char byte[length]=malloc;
        memcpy(temp, buffer + start, length);
        return getccitt162(temp,length);
}

