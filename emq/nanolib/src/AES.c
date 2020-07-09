/*
m :http://voip01.cublog.cn
    Author :Andy_yang
    E-mail :happyyangxu@163.com
    Date :2009/01/23
    Descriptions: AES CBC 128 加密算法。可以批量加密路径里面所以文件。其中不满16byte的用0x00填充。最好是加密txt文件，否则pad的位会出问题。如果加密二进制文件的 话重新写下 PadData（）函数。例如pad最后一位添上需要pad的长度，然后解密时依照最后一位的值来还原加密前的长度。
    Require : OpenSSL lib
    Compile CLI ：gcc -o aes_cbc_128 aes_cbc_128.c -lcrypto -static
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <openssl/aes.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "libevpower/AES.h"

#define  PLAINTEXT_LENGTH	19
#define  CRL_AES128_KEY		16
#define  CRL_AES_BLOCK		16

/* Private macro -------------------------------------------------------------*/
#define _AES_DEBUG_
#ifdef _AES_DEBUG_	//调试信息开关
#define	aes_dbg(format, ...)	printf(format, ##__VA_ARGS__)
#else	//_AES_DEBUG_
#define aes_dbg(format, ...)	
#endif	//_AES_DEBUG_
#define aes_err(format, ...)	printf(format, ##__VA_ARGS__)
/* Private variables ---------------------------------------------------------*/

//AES_Key  KEY = {
//	.KEYA = {0xc4, 0x8c, 0xae, 0x77, 0x18, 0x83, 0x94, 0xf3, 0x12, 0x4f, 0x1c, 0xf9, 0x12, 0x9a, 0x4b, 0x45, 0},
//	.KEYB = {0xc4, 0x8c, 0xae, 0x77, 0x18, 0x83, 0x94, 0xf3, 0x12, 0x4f, 0x1c, 0xf9, 0x12, 0x9a, 0x4b, 0x45, 0}
//};

unsigned char KEYA[16] = {0xc4, 0x8c, 0xae, 0x77, 0x18, 0x83, 0x94, 0xf3, 0x12, 0x4f, 0x1c, 0xf9, 0x12, 0x9a, 0x4b, 0x45};
unsigned char KEYB[16] = {0xc4, 0x8c, 0xae, 0x77, 0x18, 0x83, 0x94, 0xf3, 0x12, 0x4f, 0x1c, 0xf9, 0x12, 0x9a, 0x4b, 0x45};
/* Initialization Vector */
  u8 IV[CRL_AES_BLOCK] =
{  
	 /* 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    	 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,*/
	 0xc4, 0x8C, 0xAE, 0x77, 0x18, 0x83, 0x94, 0xF3,
	 0x12, 0x4F, 0x1C, 0xF9, 0x12, 0x9A, 0x4B, 0x45
 };


	
 u8 Plaintext[PLAINTEXT_LENGTH] =
  {
    /*0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
    0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
    0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
    0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
    0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
    0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,*/
		0xab, 0x23, 0x89, 0xff, 0x1f, 0x90, 0xef, 0x89, 
		0x83, 0xa8, 0x5d, 0x67, 0x31, 0xed, 0x00, 0x89,
		0x90, 0xff, 0x12
  };

/* Key to be used for AES encryption/decryption */
u8 Key[CRL_AES128_KEY] = 
  {
    /*0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
    0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
    0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b,*/
		0xc4, 0x8C, 0xAE, 0x77, 0x18, 0x83, 0x94,0xF3,
    0x12, 0x4F, 0x1C, 0xF9, 0x12, 0x9A, 0x4B, 0x45
  };

u8 Expected_Ciphertext[] =
  {
 /*   0x4f, 0x02, 0x1d, 0xb2, 0x43, 0xbc, 0x63, 0x3d,
    0x71, 0x78, 0x18, 0x3a, 0x9f, 0xa0, 0x71, 0xe8,
    0xb4, 0xd9, 0xad, 0xa9, 0xad, 0x7d, 0xed, 0xf4,
    0xe5, 0xe7, 0x38, 0x76, 0x3f, 0x69, 0x14, 0x5a,
    0x57, 0x1b, 0x24, 0x20, 0x12, 0xfb, 0x7a, 0xe0,
    0x7f, 0xa9, 0xba, 0xac, 0x3d, 0xf1, 0x02, 0xe0,
    0x08, 0xb0, 0xe2, 0x79, 0x88, 0x59, 0x88, 0x81,
    0xd9, 0x20, 0xa9, 0xe6, 0x4f, 0x56, 0x15, 0xcd,*/ 
		0xf0, 0x41, 0x47, 0x1d, 0xa4, 0x3e, 0x3c, 0x36,
		0xba, 0x70, 0x69, 0x6f, 0x62, 0xdb, 0xa1, 0x75, 
		0xa0, 0x1f, 0x37, 0x99, 0x2e, 0x92, 0x5c, 0x49, 
		0xe3, 0x0a, 0x42, 0xa4, 0x21, 0x5d, 0x12, 0xfb 
  };


void Padding(u8 *InputMessage,u32 InputMessageLength ,u8 *OutputMessage,u32 *OutputMessageLength)
{
		u32  Remainder = 0;
		u32 len;	
		u32 Modulu;

		for(len = 0;len < InputMessageLength;len++)
		{
			OutputMessage[len] = InputMessage[len];
		}

		if(InputMessageLength >= 16)
		{
			Remainder = InputMessageLength%16;
			if(Remainder == 0)
			{
				*OutputMessageLength = InputMessageLength + 16;
				for(len = (*OutputMessageLength) - 1;len >= *OutputMessageLength - 16; len--)
				{
					OutputMessage[len] = 16;
				}
			}
			else 
			{
				 Modulu = (16 - Remainder);
				 *OutputMessageLength = InputMessageLength + Modulu;
				len = *OutputMessageLength - 1;
//				for(len = (*OutputMessageLength) - 1; len >= (*OutputMessageLength) - Remainder; len--)
				//{
//					OutputMessage[len] = Modulu;
//				}
				while(Modulu--)
				{
					OutputMessage[len--] = (16 - Remainder);
				}
		        }
		}
		else
		{
			Remainder = 16 - InputMessageLength;
			*OutputMessageLength = InputMessageLength + Remainder;
			for(len = (*OutputMessageLength) - 1; len >= (*OutputMessageLength) - Remainder; len--)
			{
				OutputMessage[len] = Remainder;
			}
		}
}		


int RePadding(u8 *InputMessage,u32 InputMessageLength ,u8 *OutputMessage,u32 *OutputMessageLength)
{
	 u8 Lastbyte = 0;		
	 u32  len;

	Lastbyte = InputMessage[InputMessageLength - 1];
	if(Lastbyte<=InputMessageLength)
	{
		*OutputMessageLength = InputMessageLength - Lastbyte;
		for(len = 0; len < *OutputMessageLength; len++)
		{
			OutputMessage[len] = InputMessage[len];
		}
		return 0;
	}
	else
	{
		aes_dbg("lastbyte:%u <= InputMessageLength[%u]:%u\n", Lastbyte, InputMessageLength, InputMessage[InputMessageLength - 1]);
		aes_dbg("wrong padding length\r\n");
		return -1;
	}
}


int my_aes_encrypt(char *key, char *in, int inlen, char *out)//, int olen)可能会设置buf长度
{
	int en_len=0;  
    	if(!in || !key || !out) return 0;
    
   	 AES_KEY aes;
    	if(AES_set_encrypt_key((unsigned char*)key, 128, &aes) < 0)
   	 {
       		 return 0;
    	}
//    int len=strlen(in);//这里的长度是char*in的长度，但是如果in中间包含'\0'字符的话
   	while(en_len < inlen)
	{
	   	AES_encrypt((unsigned char*)in, (unsigned char*)out, &aes);
		in += AES_BLOCK_SIZE;
		out +=AES_BLOCK_SIZE;
		en_len +=AES_BLOCK_SIZE;
	}
	 return 1;
}

int my_aes_decrypt(char* key, char *in, int inlen, char* out)  
{  
    	if(!in || !key || !out) return 0;  
    	AES_KEY aes; 
   	 if(AES_set_decrypt_key((unsigned char*)key, 128, &aes) < 0)  
   	 {  
        	return 0;  
    	}  
//    int len=strlen(in), en_len=0;  
	int en_len = 0;  
     	while(en_len<inlen)  
     	{  
        	AES_decrypt((unsigned char*)in, (unsigned char*)out, &aes);  
        	in+=AES_BLOCK_SIZE;
		aes_dbg("AES_BLOCK_SIZE:%d\n", AES_BLOCK_SIZE);  
        	out+=AES_BLOCK_SIZE;  
        	en_len+=AES_BLOCK_SIZE;  
   	 }  
   	 return 1;  
}  



void  My_AES_CBC_Encrypt(u8 *key, u8 *InputMessage,  u32 InputMessageLength, u8 * OutputMessage)
{
	AES_KEY   ass_key;
	memset(&ass_key, 0, sizeof(ass_key)); 
	u8 iv[CRL_AES_BLOCK] = {0};
//	aes_dbg("key:%s\n", key);
	int i;
//	aes_dbg("IV:");
	for(i = 0; i < CRL_AES_BLOCK; i++){
		iv[i] = key[i];
//		aes_dbg("%d ", iv[i]);
	}
//	aes_dbg("\n");
//	aes_dbg("KEYB:");
	for(i = 0; i < CRL_AES_BLOCK; i++){
//		aes_dbg("%d ", KEY.KEYB[i]);
	}
//	aes_dbg("\n");
	memset(&ass_key, 0, sizeof(AES_KEY));
	if(AES_set_encrypt_key(key, 128, &ass_key)<0){
		aes_dbg("AES set key error...\n");
		return ;
	}
	
	AES_cbc_encrypt(InputMessage, OutputMessage, InputMessageLength, &ass_key, iv, AES_ENCRYPT);
}


void  My_AES_CBC_Decrypt(u8 * key, u8 *InputMessage,  u32 InputMessageLength, u8 *OutputMessage)
{
	AES_KEY  ass_key;
	u8  iv[CRL_AES_BLOCK] = {0};
	int i;
	for(i = 0; i < CRL_AES_BLOCK; i++){
		iv[i] = key[i];
	}
	
	memset(&ass_key, 0, sizeof(AES_KEY));
	if(AES_set_decrypt_key(key, 128, &ass_key) < 0){
		aes_dbg("AES_Dec set key error...\n");
		return ;
	}
	AES_cbc_encrypt(InputMessage, OutputMessage, InputMessageLength, &ass_key, iv, AES_DECRYPT);
}



/*
int main(void)
{
	
	u8 	 output_buff[14000] = {0};
	u8	send_buff[16000] = {0};
	u32  length;
	u32  len;
	Padding(Plaintext, 19, output_buff,  &length);
	aes_dbg("leng1:%d\n", length);
//	Padding("abcdefghijklmnopqrstuvwxyz", 26, output_buff,  &length);
	My_AES_CBC_Encrypt(Key, output_buff, length, send_buff);
	
	int i = 0;
	while(i !=length)
	{
		aes_dbg("%#x ", send_buff[i++]);
	}
	aes_dbg("\n");
	aes_dbg("enc %d\n", length);
	unsigned char  re_buff[10000] = {0};
	int  re_len = 0;	
	memset(output_buff, 0, sizeof(output_buff));
	My_AES_CBC_Decrypt(Key, send_buff, length, output_buff);
	aes_dbg("dec %d\n", length);
//	aes_dbg("output_buff:%s\n", output_buff);
	for(i = 0; i< length; i++){
		aes_dbg("%#x ", output_buff[i]);
	}	
	aes_dbg("\n");


	RePadding(output_buff, length, re_buff, &re_len);

	aes_dbg("re_len:%d\n",re_len);
	for(i = 0; i<re_len; i++){
		aes_dbg("%#x ", re_buff[i] );	
	}
	aes_dbg("\n");
	

}


*/

