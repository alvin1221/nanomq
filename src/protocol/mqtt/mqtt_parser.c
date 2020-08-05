/***
 * Jaylin
 * EMQ
 * 
 * 
 * 
 * 
 * MIT 
 *
 **/

#include <stdio.h>
#include <nng/nng.h>
#include <string.h>
#include "core/nng_impl.h"
#include "nng/protocol/mqtt/mqtt_parser.h"
#include "nng/protocol/mqtt/mqtt.h"
#include "include/nng_debug.h"

static uint32_t power(uint32_t x, uint32_t n);

static uint32_t power(uint32_t x, uint32_t n)
{

	uint32_t val = 1;

	for (uint32_t i = 0; i <= n; ++i) {
		val = x * val;
	}

	return val / x;
}

/**
 * put a value to variable byte array
 * @param dest
 * @param value
 * @return data length
 */
uint8_t put_var_integer(uint8_t *dest, uint32_t value)
{
	uint8_t len = 0;
	uint32_t init_val = 0x7F;

	for (uint32_t i = 0; i < sizeof(value); ++i) {

		if (i > 0) {
			init_val = (init_val * 0x80) | 0xFF;
		}
		dest[i] = value / (uint32_t) power(0x80, i);
		if (value > init_val) {
			dest[i] |= 0x80;
		}
		len++;
	}
	return len;
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
	int p = *pos;
	int i = 0;

	do {
		temp = *(buf + p);
		result = result + (uint32_t) (temp & 0x7f) * (power(0x80, i));
		p++;
	} while ((temp & 0x80) > 0 && i++ < 4);
	*pos = p;
	return result;
}

/**
 * Get utf-8 string
 *
 * @param dest output string
 * @param src input bytes
 * @param pos
 * @return string length -1: not utf-8, 0: empty string, >0 : normal utf-8 string
 */
int32_t get_utf8_str(char *dest, const uint8_t *src, int *pos)
{
	int32_t str_len = 0;
	NNI_GET16(src + (*pos), str_len);

	*pos = (*pos) + 2;
	if (str_len > 0) {
		if (utf8_check((const char *) (src + *pos), str_len) == ERR_SUCCESS) {
			dest = (char *) (src + (*pos));
			*pos = (*pos) + str_len;
		} else {
			str_len = -1;
		}
	}
	return str_len;
}

int utf8_check(const char *str, size_t len)
{
	int i;
	int j;
	int codelen;
	int codepoint;
	const unsigned char *ustr = (const unsigned char *) str;

	if (!str) return ERR_INVAL;
	if (len < 0 || len > 65536) return ERR_INVAL;

	for (i = 0; i < len; i++) {
		if (ustr[i] == 0) {
			return ERR_MALFORMED_UTF8;
		} else if (ustr[i] <= 0x7f) {
			codelen = 1;
			codepoint = ustr[i];
		} else if ((ustr[i] & 0xE0) == 0xC0) {
			/* 110xxxxx - 2 byte sequence */
			if (ustr[i] == 0xC0 || ustr[i] == 0xC1) {
				/* Invalid bytes */
				return ERR_MALFORMED_UTF8;
			}
			codelen = 2;
			codepoint = (ustr[i] & 0x1F);
		} else if ((ustr[i] & 0xF0) == 0xE0) {
			/* 1110xxxx - 3 byte sequence */
			codelen = 3;
			codepoint = (ustr[i] & 0x0F);
		} else if ((ustr[i] & 0xF8) == 0xF0) {
			/* 11110xxx - 4 byte sequence */
			if (ustr[i] > 0xF4) {
				/* Invalid, this would produce values > 0x10FFFF. */
				return ERR_MALFORMED_UTF8;
			}
			codelen = 4;
			codepoint = (ustr[i] & 0x07);
		} else {
			/* Unexpected continuation byte. */
			return ERR_MALFORMED_UTF8;
		}

		/* Reconstruct full code point */
		if (i == len - codelen + 1) {
			/* Not enough data */
			return ERR_MALFORMED_UTF8;
		}
		for (j = 0; j < codelen - 1; j++) {
			if ((ustr[++i] & 0xC0) != 0x80) {
				/* Not a continuation byte */
				return ERR_MALFORMED_UTF8;
			}
			codepoint = (codepoint << 6) | (ustr[i] & 0x3F);
		}

		/* Check for UTF-16 high/low surrogates */
		if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
			return ERR_MALFORMED_UTF8;
		}

		/* Check for overlong or out of range encodings */
		/* Checking codelen == 2 isn't necessary here, because it is already
		 * covered above in the C0 and C1 checks.
		 * if(codelen == 2 && codepoint < 0x0080){
		 *	 return ERR_MALFORMED_UTF8;
		 * }else
		*/
		if (codelen == 3 && codepoint < 0x0800) {
			return ERR_MALFORMED_UTF8;
		} else if (codelen == 4 && (codepoint < 0x10000 || codepoint > 0x10FFFF)) {
			return ERR_MALFORMED_UTF8;
		}

		/* Check for non-characters */
		if (codepoint >= 0xFDD0 && codepoint <= 0xFDEF) {
			return ERR_MALFORMED_UTF8;
		}
		if ((codepoint & 0xFFFF) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF) {
			return ERR_MALFORMED_UTF8;
		}
		/* Check for control characters */
		if (codepoint <= 0x001F || (codepoint >= 0x007F && codepoint <= 0x009F)) {
			return ERR_MALFORMED_UTF8;
		}
	}
	return ERR_SUCCESS;
}

uint16_t get_variable_binary(uint8_t *dest, const uint8_t *src)
{
	uint16_t len = 0;
	NNI_GET16(src, len);
	dest = (uint8_t *) (src + 2);
	return len;
}

/*
uint32_t htoi(char *str)
{
	int hexdigit;//记录每个16进制数对应的十进制数
	int i;//工作指针
	int ishex;//是否有效的16进制数
	int n;//返回的10进制数

	i = 0;
	if('0' == str[i]){
		i++;
		if('x' == str[i] || 'X' == str[i]){
			i++;
		}
	}
	n = 0;
	ishex = 1;
	for(; 1 == ishex; i++)
	{
		if('0' <= str[i] && '9' >= str[i]){
			hexdigit = str[i] - '0';
		}
		else if('a' <= str[i] && 'f' >= str[i]){
			hexdigit = str[i] - 'a' + 10;
		}
		else if('A' <= str[i] && 'F' >= str[i]){
			hexdigit = str[i] - 'A' + 10;
		}
		else{
			ishex = 0;
		}
		if(1 == ishex){
			n = 16 * n + hexdigit;
		}
	}
	return n;
}

int hex_to_oct(char *str)
{
    char temp[4];    //temp string
    int j,i,length,flag=0,oct=0,num=0,t;
    length=strlen(str); //length of the binary string 
    //printf("%d",length);

    i=length-1;  //last index of a binary string 

    while(i>=0){

    j=2;    // as we want to divide it into grp of 3
    while(j>=0){

        if(i>=0){

        temp[j]=str[i--];  // take 3 characters in the temporary string 
        j--;
      }
      else{
        flag=1;   // if binary string length is not numtiple of 3
        break;
      }

    }
    if(flag==1){

    while(j>=0){
        temp[j]='0';  //add leading 0 if length is not multiple of 3 
        j--;
    }
     flag=0;
    }

    temp[3]='\0'; //add null character at the end of the tempory string 



    // use comparisons of tempory string with binary numbers 
    if(strcmp(temp,"000")==0){
        oct=oct*10+0;
    }
    else if(strcmp(temp,"001")==0){
        oct=oct*10+1;
    }
    else if(strcmp(temp,"010")==0){
        oct=oct*10+2;
    }
    else if(strcmp(temp,"011")==0){
        oct=oct*10+3;
    }
    else if(strcmp(temp,"100")==0){
        oct=oct*10+4;
    }
    else if(strcmp(temp,"101")==0){
        oct=oct*10+5;
    }
    else if(strcmp(temp,"110")==0){
        oct=oct*10+6;
    }
    else if(strcmp(temp,"111")==0){
        oct=oct*10+7;
    }

   //        printf("\n%s",temp);

 }

 //as we move from last first character reverse the number
 num=oct;
 flag=0;
 t=1;
 while(num>0){
    if(flag==0){

      t=num%10;
      flag=1;
     }
    else{
        t=t*10+num%10;
    }
     num=num/10;
     
     return t;
 }



 //print the octal number
 printf("\n Octal number is : %d",t);

 }
*/
int fixed_header_adaptor(uint8_t *packet, nni_msg *dst)
{
	nni_msg *m;
	int rv, pos = 1;
	uint32_t len;

	m = dst;
	len = get_var_integer(packet, &pos);

	rv = nni_msg_header_append(m, packet, pos);
	//cmd = *((char *)nng_msg_body(m));
	debug_msg("fixed_header_adaptor %d %d %x", pos, rv);
	//if()
	return rv;
}

int variable_header_adaptor(uint8_t *packet, nni_msg *dst)
{
	nni_msg *m;
	int pos = 0;
	uint32_t len;

	return 0;
}


static char *client_id_gen(int *idlen, const char *auto_id_prefix, int auto_id_prefix_len)
{
	char *client_id;

	return client_id;
}
