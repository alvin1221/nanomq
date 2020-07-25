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
#include "nng/protocol/mqtt/connect_parser.h"
#include "nng/protocol/mqtt/mqtt.h"
#include "include/nng_debug.h"

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

uint8_t fixed_header_adaptor(uint8_t *packet)
{
	nni_msg *m;
	uint8_t cmd; 

	//m = mqtt_msg;
	//cmd = *((char *)nng_msg_body(m));
	//debug_msg("fixed_header_adaptor %x %02x %x", 0x10, (char *)nng_msg_body(m), (char *)nng_msg_body(m));
	//if()
	return 0;
}


static char *client_id_gen(int *idlen, const char *auto_id_prefix, int auto_id_prefix_len)
{
	char *client_id;

	return client_id;
}