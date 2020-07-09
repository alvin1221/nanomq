

#ifndef  __AES__H
#define  __AES__H



typedef enum {	FAILED = 0, PASSED = !FAILED} TestStatus;

typedef unsigned char u8;
typedef unsigned int u32;
//extern  AES_Key		KEY;
extern 	u8		IV[16];

extern  unsigned char KEYB[16];
extern  unsigned char KEYA[16];

void Padding(u8 *InputMessage,u32 InputMessageLength ,u8 *OutputMessage,u32 *OutputMessageLength);
int RePadding(u8 *InputMessage,u32 InputMessageLength ,u8 *OutputMessage,u32 *OutputMessageLength);
void  My_AES_CBC_Encrypt(u8 *key, u8 *InputMessage,  u32 InputMessageLength, u8 * OutputMessage);
void  My_AES_CBC_Decrypt(u8 * key, u8 *InputMessage,  u32 InputMessageLength, u8 *OutputMessage);

int my_aes_encrypt(char *key, char *in, int inlen, char *out);
int my_aes_decrypt(char *key, char *in, int inlen, char *out);





#endif  // AES.h


