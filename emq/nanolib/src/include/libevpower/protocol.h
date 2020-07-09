#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__
#include <stdint.h>
#include <sys/types.h>
#include "AES.h"
#include "CRC.h"


#define UNIQUECODELEN	(32)
typedef char uniquecode_t[UNIQUECODELEN + 1];

//bit little (endian) swap 32bit
#define blsw32(dat)	((((uint32_t)(dat) & 0xff000000) >> 24)\
					|(((uint32_t)(dat) & 0x00ff0000) >>  8)\
					|(((uint32_t)(dat) & 0x0000ff00) <<  8)\
					|(((uint32_t)(dat) & 0x000000ff) << 24))

//bit little (endian) swap 16bit
#define blsw16(dat)	((((uint16_t)(dat) & 0xff00) >> 8)\
					|(((uint16_t)(dat) & 0x00ff) << 8))

//(u8* or char*)pointer to u32(uint32_t)
#define ptou32(ptr)	(*(uint32_t *)(ptr))

//(u8* or char*)pointer to u16(uint16_t)
#define ptou16(ptr)	(*(uint16_t *)(ptr))

//hex(string) to int
extern uint32_t htoi(char *str);
extern void genrateuniquecode(uniquecode_t uniquecode);


/***********************************************************************
  Protocol 1
原始报文
	|*****|**********|****|
	|head |   payload(crc)|
	head.len = sizeof(head) - P1LEN4PROMPT - P1LEN4LEN + sizeof(payload)
crc后报文
	|*****|***************|
	|head |   payload(crc)|

padding之后
	|*****|**********************|
	|head |   payload(crc)| null |

encrypt之后
	|*****|**********************|
	|head |encrypted payload(crc)|
***********************************************************************/
typedef enum{
	/* VERSION 1.9 */
	/*************** 电桩 -> 路由 ***************/
	P1CMD_CNNT_REQ	= 0x10,	//Connect Request 连接请求
	P1CMD_HR_BT	= 0x34,	//Heart Beat  心跳
	P1CMD_CHRG_REQ	= 0x54,	//charge request  充电请求
	P1CMD_UP_IN_CHRG	= 0x56,	//update in charging  充电中状态更新
	P1CMD_STOP_CHRG_REQ  =	0x58, //stop charge request 停止充电请求
	P1CMD_CTRL_CHRGR_RSP	= 0xa3,	//control charger respond 控制电桩回应
	P1174CMD_RD_METER_RSP  =	0xa5, //read meter respond 抄表回应(p1174)
	P1176CMD_RD_METER_RSP  =	0xae, //read meter respond 抄表回应(p1176)
	P1CMD_UP_RSP	= 0x91,	//update respond  更新指令回应
	P1CMD_UP_SW_RSP	= 0x95,	//update software respond 更新软件回应
	P1CMD_RSVD_CMD	= 0x42,	//reserved command	预约指令 /* 同名冲突 */
	P1CMD_PUSH_CFG_RSP	= 0x50,	//push config respond 推送配置回应
	P1CMD_START_CHRG_RSP	= 0x61,	//start charge respond 开始充电命令回应
	//P1CMD_STOP_CHRG_RSP =	0x63, //stop charge respond //停止充电命令回应	/* 没有用到 */
	P1CMD_CHRGR_EXCEPT_CODE	= 0xee,	//charge exception code 电庄异常码
	P1CMD_RUTR_UP_RSP  =	0xfe, //router update respond 路由升级命令回应	/* 同名冲突 */
	P1CMD_EXTERN_RSP	= 0xa7,	//extern respond 扩展回应
	P1CMD_IES_SRC_DATA =	0x88, //ies src data IES电桩原始数据
	/*************** 路由 -> 电桩 ***************/
	P1CMD_CNNT_CFM =	0x11, //connect comfirm 连接确认
	P1CMD_RUTR_HR_BT	= 0x35,	//Router Heart Beat	路由心跳
	P1CMD_CHRG_REQ_RSP	= 0x55,	//charge request respond 充电请求回应
	P1CMD_STOP_CHRG_REQ_RSP	= 0x57,	//stop charge request respond  停止充电请求回应
	P1CMD_CTRL_CHRGR	= 0xa2,	//control charger 控制电桩（指令）
	P1CMD_RD_METER =	0xa4, //read meter	抄表
	P1CMD_UP	= 0x90,	//update  更新指令
	P1CMD_UP_SW  =	0x94, //update software	更新软件
	P1CMD_RSVD_CMD1	= 0x41,	//reserved command 预约指令  /* 同名冲突 */
	P1CMD_OPERATE	= 0x21,	//Operate 操作命令
	P1CMD_PUSH_CFG	= 0x51,	//push config 推送配置
	P1CMD_START_CHRG =	0x60, //start charge 开始充电命令
	P1CMD_STOP_CHRG  =	0x62, //stop charge 停止充电命令
	P1CMD_RUTR_UP_RSP1  =	0xfa, //router update respond 路由升级命令回应	/* 同名冲突 */
	P1CMD_EXTERN =	0xa6, //extern	扩展
}p1cmd_t;

#define P1LEN4PROMPT	(3)
#define P1LEN4LEN		(1)
#define P1LEN4CMD		(1)
#define P1LEN4CID		(4)
typedef struct{
	uint8_t prompt[P1LEN4PROMPT];	//EV>
	uint8_t len[P1LEN4LEN];		//从命令到CRC的长度
	uint8_t cmd[P1LEN4CMD];
	uint8_t cid[P1LEN4CID];
}p1head_t;

#define P1LEN4ACCCHARGERFREQUENCE		(2)
#define P1LEN4ACCCHARGINGFREQUENCE	(2)
#define P1LEN4ACCNUM					(2)
#define P1LEN4ACCPOWER				(4)
#define P1LEN4CHARGERCODE				(2)
#define P1LEN4CHARGERDURATION			(2)
#define P1LEN4CHARGERNUM				(1)
#define P1LEN4CHARGERSTATUS			(1)
#define P1LEN4CHARGERTYPE				(1)
#define P1LEN4CHARGERVERSION			(2)/**/
#define P1LEN4CHARGERWAY				(1)
#define P1LEN4CHARGERYAOXIN			(4)
#define P1LEN4CHARGINGCODE			(2)
#define P1LEN4CHARGINGDURATION		(2)
//#define P1LEN4CHARGVERSION			(2)/**/
#define P1LEN4CONCENT					(1)
#define P1LEN4CRC						(2)
#define P1LEN4DIGITALINPUT			(2)
#define P1LEN4PWM						(2)
#define P1LEN4CPV						(2)
#define P1LEN4DIGITALOUTPUT			(2)
#define P1LEN4ENDTIMESTAMP			(4)
#define P1LEN4ERROR					(1)
#define P1LEN4ERRORCODE1				(4)
#define P1LEN4ERRORCODE2				(4)
#define P1LEN4ERRORCODE3				(4)
#define P1LEN4ERRORCODE4				(4)
#define P1LEN4ERRORCODE5				(4)
#define P1LEN4ERRORSTATUS				(1)
#define P1LEN4EVLINKID				(16)
#define P1LEN4FRAMECOMPELETE			(1)
#define P1LEN4FRAMESEQUENCE			(1)
#define P1LEN4FW_NAME					(12)
#define P1LEN4IP						(4)
#define P1LEN4MAC						(6)
#define P1LEN4MODEL					(10)
#define P1LEN4OUTPUTCURRENT			(2)
#define P1LEN4OUTPUTVOLTAGE			(2)
#define P1LEN4POWER					(2)
#define P1LEN4PRESENTMODE				(1)
#define P1LEN4PRESENTOUTPUTCURRENT	(2)
#define P1LEN4PROTOCOLVERSION			(2)
#define P1LEN4READCOMPLETE			(1)
/* Ver 19174 */
#define P1174LEN4PERRECORD			(64)	//每条充电记录的长度
#define P1174MAXPERRECORDDATA		(15)	//一个报文最多可存的记录数量
#define P1174LEN4RECORDDATA				(P1174LEN4PERRECORD * P1174MAXPERRECORDDATA)
/* Ver 19176 */
#define P1176LEN4PERRECORD			(76)	//每条充电记录的长度
#define P1176MAXPERRECORDDATA		(15)	//一个报文最多可存的记录数量
#define P1176LEN4RECORDDATA				(P1176LEN4PERRECORD * P1176MAXPERRECORDDATA)
#define P1LEN4RESERVEDUID				(16)
#define P1LEN4RESTART					(1)
#define P1LEN4RNG					(4)
#define P1LEN4SELECTCURRENT			(1)
#define P1LEN4SELECTSOCKET			(1)
#define P1LEN4SEQUENCE				(2)
#define P1LEN4SERIES					(10)
#define P1LEN4SOC						(1)
#define P1LEN4SOCKET					(1)
#define P1LEN4STARTTIMESTAMP			(4)
#define P1LEN4STATE					(1)
#define P1LEN4STOPUID					(32)/*(16)*/
#define P1LEN4SUBMODE					(2)
#define P1LEN4TARGETCURRENT			(1)
#define P1LEN4TILLTIME				(2)
#define P1LEN4UID						(16)
#define P1LEN4UNIXTIMESTAMP			(4)
#define P1LEN4USEPOWER				(4)
#define P1LEN4USETIME					(2)
#define P1LEN4STOPREASON				(1)

#define P1LEN4KEY						(16)
#define P1LEN4RNG				(4)
#define P1LEN4STARTTIME			(4)
#define P1LEN4STOPTIME				(4)
#define P1IESLEN4ERRCODE			(4)
#define P1IESLEN4RAWPOWER			(4)
#define P1IESLEN4VERSION			(16)

#define P1LEN4MAXCURRENTSUPPLY		(1)
#define P1LEN4TARGETMODE				(1)
#define P1LEN4SYSTEMMESSAGE			(1)
#define P1LEN4MAXCURRENT				(1)
#define P1LEN4RESET					(1)
#define P1LEN4UNLOCK					(1)
#define P1LEN4TARGETRECODEID			(2)
#define P1LEN4FRAMELENGTH				(4)
#define P1LEN4FRAMENUM				(1)
#define P1LEN4CURRENTFRAMELENGTH		(4)
#define P1LEN4FRAMEDATA				(1024)
#define P1LEN4SUBSCRIBETIME			(2)
#define P1LEN4SUBSCRIBEUID			(32)
#define P1LEN4CONFIGNUM				(1)
#define P1LEN4CONFIGLENGTH			(4)
#define P1LEN4CURRENTCONFIGLENGTH		(4)
#define P1LEN4OID						(25)
#define P1LEN4CHARGERCURRENT			(2)
#define P1LEN4PID						(15)
#define P1LEN4TARGETENEGY				(4)
#define P1LEN4STARTUID				(32)
#define P1LEN4STOPLIMIT				(1)
#define P1LEN4JAY						(1)

#define LEN4PERFRAGMENT	(1024)


typedef struct{ //桩->路:连接请求(0x10)
	p1head_t head;
	uint8_t ip[P1LEN4IP];
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t presentmode[P1LEN4PRESENTMODE];
	uint8_t model[P1LEN4MODEL];
	uint8_t series[P1LEN4SERIES];
	uint8_t protocolversion[P1LEN4PROTOCOLVERSION];
	uint8_t chargerversion[P1LEN4CHARGERVERSION];
	uint8_t mac[P1LEN4MAC];
	uint8_t chargertype[P1LEN4CHARGERTYPE];
	uint8_t fw_name[P1LEN4FW_NAME];
	uint8_t crc[P1LEN4CRC];
}p10x10_t; //桩->路:连接请求(0x10)

typedef struct{ //路->桩:连接响应(0x11)
	p1head_t head;
	uint8_t ip[P1LEN4IP];
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t key[P1LEN4KEY];
	uint8_t crc[P1LEN4CRC];
}p10x11_t; //路->桩:连接响应(0x11)


typedef struct{ //桩->路:心跳(0x34)
	p1head_t head;
	uint8_t presentmode[P1LEN4PRESENTMODE];
	uint8_t submode[P1LEN4SUBMODE];
	uint8_t sequence[P1LEN4SEQUENCE];
	uint8_t digitaloutput[P1LEN4DIGITALOUTPUT];
	uint8_t digitalinput[P1LEN4DIGITALINPUT];
	uint8_t crc[P1LEN4CRC];
}p10x34_t; //桩->路:心跳(0x34)

typedef struct{ //路->桩:心跳响应(0x35)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t maxcurrentsupply[P1LEN4MAXCURRENTSUPPLY];
	uint8_t crc[P1LEN4CRC];
}p10x35_t; //路->桩:心跳响应(0x35)


typedef struct{ //桩->路:充电请求(0x54)
	p1head_t head;
	uint8_t ip[P1LEN4IP];
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t presentmode[P1LEN4PRESENTMODE];
	uint8_t submode[P1LEN4SUBMODE];
	uint8_t sequence[P1LEN4SEQUENCE];
	uint8_t accpower[P1LEN4ACCPOWER];
	uint8_t accchargingfrequence[P1LEN4ACCCHARGINGFREQUENCE];
	uint8_t selectsocket[P1LEN4SELECTSOCKET];
	uint8_t selectcurrent[P1LEN4SELECTCURRENT];
	uint8_t presentoutputcurrent[P1LEN4PRESENTOUTPUTCURRENT];
	uint8_t chargingduration[P1LEN4CHARGINGDURATION];
	uint8_t cycleid[P1LEN4CHARGINGCODE];
	uint8_t evlinkid[P1LEN4EVLINKID];
	uint8_t digitaloutput[P1LEN4DIGITALOUTPUT];
	uint8_t digitalinput[P1LEN4DIGITALINPUT];
	uint8_t chargerway[P1LEN4CHARGERWAY];
	uint8_t rng[P1LEN4RNG];
	uint8_t crc[P1LEN4CRC];
}p10x54_t; //桩->路:充电请求(0x54)

typedef struct{ //路->桩:充电请求响应(0x55)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t targetmode[P1LEN4TARGETMODE];
	uint8_t systemmessage[P1LEN4SYSTEMMESSAGE];
	uint8_t chargingduration[P1LEN4CHARGINGDURATION];
	uint8_t chargingcode[P1LEN4CHARGINGCODE];
	uint8_t evlinkid[P1LEN4EVLINKID];
	uint8_t maxcurrent[P1LEN4MAXCURRENT];
	uint8_t crc[P1LEN4CRC];
}p10x55_t; //路->桩:充电请求响应(0x55)


typedef struct{ //桩->路:充电心跳(0x56)
	p1head_t head;
	uint8_t ip[P1LEN4IP];
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t presentmode[P1LEN4PRESENTMODE];
	uint8_t submode[P1LEN4SUBMODE];
	uint8_t sequence[P1LEN4SEQUENCE];
	uint8_t targetcurrent[P1LEN4TARGETCURRENT];
	uint8_t outputcurrent[P1LEN4OUTPUTCURRENT];
	uint8_t outputvoltage[P1LEN4OUTPUTVOLTAGE];
	uint8_t soc[P1LEN4SOC];
	uint8_t tilltime[P1LEN4TILLTIME];
	uint8_t usepower[P1LEN4USEPOWER];
	uint8_t usetime[P1LEN4USETIME];
	uint8_t evlinkid[P1LEN4EVLINKID];
	uint8_t chargerway[P1LEN4CHARGERWAY];
	uint8_t chargercode[P1LEN4CHARGERCODE];
	uint8_t digitaloutput[P1LEN4DIGITALOUTPUT];
	uint8_t digitalinput[P1LEN4DIGITALINPUT];
	uint8_t pwm[P1LEN4PWM];
	uint8_t cpv[P1LEN4CPV];
	uint8_t rng[P1LEN4RNG];
	uint8_t starttime[P1LEN4STARTTIME];
	uint8_t stoptime[P1LEN4STOPTIME];
	uint8_t ies_errcode[P1IESLEN4ERRCODE];
	uint8_t ies_rawpower[P1IESLEN4RAWPOWER];
	uint8_t ies_version[P1IESLEN4VERSION];
	uint8_t crc[P1LEN4CRC];
}p10x56_t; //桩->路:充电心跳(0x56)


typedef struct{ //桩->路:停止告知(0x58)
	p1head_t head;
	uint8_t ip[P1LEN4IP];
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t presentmode[P1LEN4PRESENTMODE];
	uint8_t submode[P1LEN4SUBMODE];
	uint8_t sequence[P1LEN4SEQUENCE];
	uint8_t accpower[P1LEN4ACCPOWER];
	uint8_t accchargingfrequence[P1LEN4ACCCHARGINGFREQUENCE];
	uint8_t selectsocket[P1LEN4SELECTSOCKET];
	uint8_t targetcurrent[P1LEN4TARGETCURRENT];
	uint8_t outputcurrent[P1LEN4OUTPUTCURRENT];
	uint8_t outputvoltage[P1LEN4OUTPUTVOLTAGE];
	uint8_t soc[P1LEN4SOC];
	uint8_t tilltime[P1LEN4TILLTIME];
	uint8_t usepower[P1LEN4USEPOWER];
	uint8_t usetime[P1LEN4USETIME];
	uint8_t evlinkid[P1LEN4EVLINKID];
	uint8_t chargerway[P1LEN4CHARGERWAY];
	uint8_t chargercode[P1LEN4CHARGERCODE];
	uint8_t digitaloutput[P1LEN4DIGITALOUTPUT];
	uint8_t digitalinput[P1LEN4DIGITALINPUT];
	uint8_t stopreason[P1LEN4STOPREASON];
	uint8_t rng[P1LEN4RNG];
}p10x58_t; //桩->路:停止告知(0x58)

typedef struct{ //路->桩:停止告知响应(0x57)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t targetmode[P1LEN4TARGETMODE];
	uint8_t systemmessage[P1LEN4SYSTEMMESSAGE];
	uint8_t evlinkid[P1LEN4EVLINKID];
	uint8_t crc[P1LEN4CRC];
}p10x57_t; //路->桩:停止告知响应(0x57)


typedef struct{ //路->桩:控制电桩(0xa2)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t reset[P1LEN4RESET];
	uint8_t unlock[P1LEN4UNLOCK];
	uint8_t crc[P1LEN4CRC];
}p10xa2_t; //路->桩:控制电桩(0xa2)

typedef struct{ //桩->路:控制电桩响应(0xa3)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t concent[P1LEN4CONCENT];
	uint8_t crc[P1LEN4CRC];
}p10xa3_t; //桩->路:控制电桩响应(0xa3)


typedef struct{ //路->桩:抄表(0xa4)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t targetrecodeid[P1LEN4TARGETRECODEID];
	uint8_t starttimestamp[P1LEN4STARTTIMESTAMP];
	uint8_t endtimestamp[P1LEN4ENDTIMESTAMP];
	uint8_t chargercode[P1LEN4CHARGERCODE];
	uint8_t crc[P1LEN4CRC];
}p10xa4_t; //路->桩:抄表(0xa4)

typedef struct{ //桩->路:抄表19176响应(0xae)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t starttimestamp[P1LEN4STARTTIMESTAMP];
	uint8_t endtimestamp[P1LEN4ENDTIMESTAMP];
	uint8_t recorddata[P1176LEN4RECORDDATA];
	uint8_t chargernum[P1LEN4CHARGERNUM];
	uint8_t chargercode[P1LEN4CHARGERCODE];
	uint8_t accnum[P1LEN4ACCNUM];
	uint8_t readcomplete[P1LEN4READCOMPLETE];
	uint8_t crc[P1LEN4CRC];
}p10xae_t; //桩->路:抄表19176响应(0xae)

typedef struct{ //桩->路:抄表19174响应(0xa5)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t starttimestamp[P1LEN4STARTTIMESTAMP];
	uint8_t endtimestamp[P1LEN4ENDTIMESTAMP];
	uint8_t recorddata[P1174LEN4RECORDDATA];
	uint8_t chargernum[P1LEN4CHARGERNUM];
	uint8_t chargercode[P1LEN4CHARGERCODE];
	uint8_t accnum[P1LEN4ACCNUM];
	uint8_t readcomplete[P1LEN4READCOMPLETE];
	uint8_t crc[P1LEN4CRC];
}p10xa5_t; //桩->路:抄表19174响应(0xa5)


typedef struct{ //路->桩:更新指令(0x90)
	p1head_t head;
	uint8_t chargerversion[P1LEN4CHARGERVERSION];
	uint8_t framelength[P1LEN4FRAMELENGTH];
	uint8_t framenum[P1LEN4FRAMENUM];
	uint8_t crc[P1LEN4CRC];
}p10x90_t; //路->桩:更新指令(0x90)

/*
typedef struct{ //桩->路:更新指令响应(0x91)
	p1head_t head;
	uint8_t chargerversion[P1LEN4CHARGERVERSION];
	uint8_t chargerstatus[P1LEN4CHARGERSTATUS];
	uint8_t crc[P1LEN4CRC];
}p10x91_t; //桩->路:更新指令响应(0x91)
*/

typedef struct{ //桩->路:索要更新碎片(0x95)
	p1head_t head;
	//uint8_t chargversion[P1LEN4CHARGVERSION];
	uint8_t chargerversion[P1LEN4CHARGERVERSION];
	uint8_t framesequence[P1LEN4FRAMESEQUENCE];
	uint8_t framecompelete[P1LEN4FRAMECOMPELETE];
	uint8_t crc[P1LEN4CRC];
}p10x95_t; //桩->路:索要更新碎片(0x95)

typedef struct{ //路->桩:发送更新碎片(0x94)
	p1head_t head;
	uint8_t chargerversion[P1LEN4CHARGERVERSION];
	uint8_t currentframelength[P1LEN4CURRENTFRAMELENGTH];
	uint8_t framesequence[P1LEN4FRAMESEQUENCE];
	uint8_t framedata[P1LEN4FRAMEDATA];
	uint8_t crc[P1LEN4CRC];
}p10x94_t; //路->桩:发送更新碎片(0x94)


/*
typedef struct{ //路->桩:预约(0x41)
	p1head_t head;
	uint8_t subscribetime[P1LEN4SUBSCRIBETIME];
	uint8_t subscribeuid[P1LEN4SUBSCRIBEUID];
	uint8_t crc[P1LEN4CRC];
}p10x41_t; //路->桩:预约(0x41)

typedef struct{ //桩->路:预约响应(0x42)
	p1head_t head;
	uint8_t reserveduid[P1LEN4RESERVEDUID];
	uint8_t state[P1LEN4STATE];
	uint8_t crc[P1LEN4CRC];
}p10x42_t; //桩->路:预约响应(0x42)
*/


typedef struct{ //路->桩:推送配置(0x51)
	p1head_t head;
	uint8_t confignum[P1LEN4CONFIGNUM];
	uint8_t configlength[P1LEN4CONFIGLENGTH];
	uint8_t currentconfiglength[P1LEN4CURRENTCONFIGLENGTH];
	uint8_t framesequence[P1LEN4FRAMESEQUENCE];
	uint8_t framedata[P1LEN4FRAMEDATA];
	uint8_t crc[P1LEN4CRC];
}p10x51_t; //路->桩:推送配置(0x51)

typedef struct{ //桩->路:推送配置响应(0x50)
	p1head_t head;
	uint8_t framesequence[P1LEN4FRAMESEQUENCE];
	uint8_t framecompelete[P1LEN4FRAMECOMPELETE];
	uint8_t crc[P1LEN4CRC];
}p10x50_t; //桩->路:推送配置响应(0x50)


typedef struct{ //路->桩:远程启动充电(0x60)
	p1head_t head;
	uint8_t oid[P1LEN4OID];
	uint8_t chargercurrent[P1LEN4CHARGERCURRENT];
	uint8_t pid[P1LEN4PID];
	uint8_t targetenegy[P1LEN4TARGETENEGY];
	uint8_t startuid[P1LEN4STARTUID];
	uint8_t crc[P1LEN4CRC];
}p10x60_t; //路->桩:远程启动充电(0x60)

typedef struct{ //桩->路:远程启动充电响应(0x61)
	p1head_t head;
	uint8_t error[P1LEN4ERROR];
	uint8_t uid[P1LEN4UID];
	uint8_t crc[P1LEN4CRC];
}p10x61_t; //桩->路:远程启动充电响应(0x61)


typedef struct{ //路->桩:远程停止充电(0x62)
	p1head_t head;
	uint8_t stopuid[P1LEN4STOPUID];
	uint8_t stoplimit[P1LEN4STOPLIMIT];
	uint8_t crc[P1LEN4CRC];
}p10x62_t; //路->桩:远程停止充电(0x62)

/*
typedef struct{ //桩->路:远程停止充电响应(0x63)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t errorstatus[P1LEN4ERRORSTATUS];
	uint8_t presentmode[P1LEN4PRESENTMODE];
	uint8_t submode[P1LEN4SUBMODE];
	uint8_t sequence[P1LEN4SEQUENCE];
	uint8_t accpower[P1LEN4ACCPOWER];
	uint8_t accchargerfrequence[P1LEN4ACCCHARGERFREQUENCE];
	uint8_t selectsocket[P1LEN4SELECTSOCKET];
	uint8_t selectcurrent[P1LEN4SELECTCURRENT];
	uint8_t presentoutputcurrent[P1LEN4PRESENTOUTPUTCURRENT];
	uint8_t chargerduration[P1LEN4CHARGERDURATION];
	uint8_t chargingcode[P1LEN4CHARGINGCODE];
	uint8_t digitaloutput[P1LEN4DIGITALOUTPUT];
	uint8_t digitalinput[P1LEN4DIGITALINPUT];
	uint8_t chargerway[P1LEN4CHARGERWAY];
	uint8_t power[P1LEN4POWER];
	uint8_t stopuid[P1LEN4STOPUID];
	uint8_t crc[P1LEN4CRC];
}p10x63_t; //桩->路:远程停止充电响应(0x63)
*/


typedef struct{ //桩->路:电桩异常码(0xee)
	p1head_t head;
	uint8_t unixtimestamp[P1LEN4UNIXTIMESTAMP];
	uint8_t presentmode[P1LEN4PRESENTMODE];
	uint8_t sequence[P1LEN4SEQUENCE];
	uint8_t accpower[P1LEN4ACCPOWER];
	uint8_t accchargerfrequence[P1LEN4ACCCHARGERFREQUENCE];
	uint8_t socket[P1LEN4SOCKET];
	uint8_t outputcurrent[P1LEN4OUTPUTCURRENT];
	uint8_t outputvoltage[P1LEN4OUTPUTVOLTAGE];
	uint8_t soc[P1LEN4SOC];
	uint8_t tilltime[P1LEN4TILLTIME];
	uint8_t usepower[P1LEN4USEPOWER];
	uint8_t usetime[P1LEN4USETIME];
	uint8_t evlinkid[P1LEN4EVLINKID];
	uint8_t chargerway[P1LEN4CHARGERWAY];
	uint8_t chargingcode[P1LEN4CHARGINGCODE];
	uint8_t errorcode1[P1LEN4ERRORCODE1];
	uint8_t errorcode2[P1LEN4ERRORCODE2];
	uint8_t errorcode3[P1LEN4ERRORCODE3];
	uint8_t errorcode4[P1LEN4ERRORCODE4];
	uint8_t errorcode5[P1LEN4ERRORCODE5];
	uint8_t chargeryaoxin[P1LEN4CHARGERYAOXIN];
	uint8_t crc[P1LEN4CRC];
}p10xee_t; //桩->路:电桩异常码(0xee)


/*
typedef struct{ //路->桩:抄总电量(0xa6)
	p1head_t head;
	uint8_t jay[P1LEN4JAY];	//I donot know how to named it, so...
	uint8_t crc[P1LEN4CRC];
}p1extern_t;//extern 扩展

typedef struct{ //桩->路:抄总电量响应(0xa7)
	p1head_t head;
	uint8_t accpower[P1LEN4ACCPOWER];
	uint8_t crc[P1LEN4CRC];
}p1extern_rsp_t;//extern respond 扩展回应
*/


extern void p1rcpack_head(void *dptr, void *prompt, uint8_t cmd, void *cid);
extern size_t p1decrypt_unpadding(uint8_t *recv_buff, size_t pkt_len, uint8_t
        *key);
extern int32_t p1crc_padding_encrypt_writen(int32_t fd, uint8_t *send_buff,
        size_t pkt_len, uint8_t *key);

extern size_t p1rcpack0x11(void *dptr, void *ip, time_t unixtimestamp, void
        *key); //路->桩:连接响应(0x11)
extern size_t p1rcpack0x35(void *dptr, time_t unixtimestamp, uint8_t
        maxcurrentsupply); //路->桩:心跳响应(0x35)
extern size_t p1rcpack0x55(void *dptr, time_t unixtimestamp, uint8_t targetmode,
        uint8_t systemmessage, uint16_t chargingduration, uint16_t chargingcode,
        void *evlinkid, uint8_t maxcurrent); //路->桩:充电请求响应(0x55)
extern size_t p1rcpack0x57(void *dptr, time_t unixtimestamp, uint8_t targetmode,
        uint8_t systemmessage, void *evlinkid); //路->桩:停止告知响应(0x57)
extern size_t p1rcpack0xa2(void *dptr, time_t unixtimestamp, uint8_t reset,
        uint8_t unlock); //路->桩:控制电桩(0xa2)
extern size_t p1rcpack0xa4(void *dptr, time_t unixtimestamp, uint16_t
        targetrecodeid, time_t starttimestamp, time_t endtimestamp, uint16_t
        chargercode); //路->桩:抄表(0xa4)
extern size_t p1rcpack0x90(void *dptr, void *chargerversion, uint32_t
        framelength, uint8_t framenum); //路->桩:更新指令(0x90)
extern size_t p1rcpack0x94(void *dptr, void *chargerversion, uint32_t
        currentframelength, uint8_t framesequence, void *framedata); //路->桩:发送更新碎片(0x94)
extern size_t p1rcpack0x51(void *dptr, uint8_t confignum, uint32_t configlength,
        uint32_t currentconfiglength, uint8_t framesequence, void *framedata); //路->桩:推送配置(0x51)
extern size_t p1rcpack0x60(void *dptr, void *oid, uint16_t chargercurrent, void
        *pid, uint32_t targetenegy, void *startuid); //路->桩:远程启动充电(0x60)
extern size_t p1rcpack0x62(void *dptr, void *stopuid, uint8_t stoplimit); //路->桩:远程停止充电(0x62)



/***********************************************************************
  Protocol 2
原始报文
	|*****|***************|
	|head |  payload      |

padding之后
	|*****|*********************|
	|head |  payload      |null |

encrypt之后
	|*****|*********************|
	|head |  encrypted payload  |
	head.len = sizeof(head)-P2LEN4PROMPT-P2LEN4LEN + sizeof(encrypted payload) + sizeof(crc)

crc之后
	|*****|*********************|***|
	|head |  encrypted payload  |crc|
***********************************************************************/
typedef enum{
	/*************** VERSION 2.0 start ***************/
	P2CMD_CNNT_REQ	  = 0x80, //Connect Request 连接请求(0x80)
	P2CMD_CHRGR_STAT	  = 0x81, //charger status  设备状态/电桩状态(心跳)(0x81)
	P2CMD_CHRG_REQ	= 0x82,	//charge request 充电请求(拍卡请求充电)(0x82)
	P2CMD_CHRGING_HR_BT	= 0x83, //charging heart beat //充电临时动态日志(充电状态下的心跳)(0x83)
	P2CMD_CHRG_STOP	  = 0x84, //charge stop 停止告知(拍卡停止充电)(0x84)
	P2CMD_PUSH_CFG	  = 0x85, //push config	写入配置(推送配置)(0x85)
	P2CMD_RD_CFG	  = 0x86, //read config	读取配置(0x86)
	P2CMD_RD_ACCPOWER	= 0x87,	//read accpower 读取历史用量(读取记录条数和总电量)(0x87)
	P2CMD_RD_TM_RECORD	= 0x88,	//read (time) record 请求读取记录日志(读取某时间段的记录)(0x88)
	P2CMD_RD_ST_RECORD	= 0x89,	//read (section) record 请求读取记录日志(读取某区间段的记录)(0x89)
	P2CMD_START_CHRG	= 0x8a,	//start charge	后台充电开始(0x8a)
	P2CMD_STOP_CHRG	= 0x8b,	//stop charge	后台停止(充电)(0x8b)
	P2CMD_RSVD		= 0x8c,	//reseved 开始预约(0x8c)
	P2CMD_UNRSVD	  = 0x8d, //unreseved	停止预约(0x8d)
	P2CMD_UPDATE_REQ	= 0x8e,	//update request 请求升级(0x8e)
	P2CMD_UPDATE		= 0x8f,	//update 升级传输(0x8f)
	P2CMD_REBOOT		= 0x90,	//reboot 重启(0x90)
	P2CMD_RD_UPLOADED  = 0x92, //0x92
	P2CMD_RD_SPECIFIED	= 0x79,	//0x79
	P2CMD_UPLOADEDFLAG	  = 0x93, //记录已上传标志(0x93)
	P2CMD_TIME_CALIBRATE = 0x99, //后台时间校准(0x99)
	P2CMD_NETWORKNOTIFY  = 0x9d, //告知电桩路由联网状态
	/*************** VERSION 2.0  end * ***************/
}p2cmd_t;

#define P2PROMPT "EV#"
#define P2LEN4PROMPT	(3)
#define P2LEN4LEN		(2)
#define P2LEN4CMD		(1)
#define P2LEN4CID		(8)
typedef struct{
	uint8_t prompt[P2LEN4PROMPT];	//EV>
	uint8_t len[P2LEN4LEN];		//从命令到CRC的长度
	uint8_t cmd[P2LEN4CMD];
	uint8_t cid[P2LEN4CID];
}p2head_t;

#define P2LEN4ABORTEDREASON	(1)
#define P2LEN4ACCPOWER			(4)
#define P2LEN4CHRGPERMIT		(1)
#define P2LEN4CHRGTOT			(2)
#define P2LEN4CONFIG			(384)
#define P2LEN4CPPWM			(1)
#define P2LEN4CPVOLTAGE		(4)
#define P2LEN4CRC				(2)
#define P2LEN4CURRENT			(4)
#define P2LEN4CYCLEID			(2)
#define P2LEN4CYCLEIDS8		(16)
#define P2LEN4DURATION			(4)
#define P2LEN4ENDRECORDID		(2)
#define P2LEN4ENDTIMESTAMP		(4)
#define P2LEN4ERRCODE			(1)
#define P2LEN4REASON4STOPFAILED	(1)

#define P2LEN4EVLINKID			(16)
#define P2LEN4FILEBYTESUM		(4)
#define P2LEN4FILELENGTH		(4)
#define P2LEN4FRAGMENT			(1024)/**/
#define P2LEN4FRAGMENTID		(4)
#define P2LEN4FWNAME			(12)
#define P2LEN4HWSTATUS			(80)
#define P2LEN4HWVER			(1)
#define P2LEN4KEY				(16)
#define P2LEN4CNNTMODE			(1)
#define P2LEN4MAC				(6)
#define P2LEN4MAXCURRENT		(1)
#define P2LEN4MAXCURRENTVALID	(1)
#define P2LEN4MODE				(1)
#define P2LEN4MODEL			(10)
#define P2LEN4NETWORKSTATUS	(1)
#define P2LEN4LIVELTEM	(1)
#define P2LEN4NEWSWVER			(2)
#define P2LEN4OUTOFSERVICETYPE	(4)
#define P2LEN4PACKAGEID		(16)
#define P2LEN4POWER			(4)
#define P2LEN4PROTCLVER		(2)
#define P2LEN4RECORD			(128)/**/
#define P2LEN4RECORDCNT		(1)
#define P2LEN4RECORDTOT		(2)
#define P2LEN4RFIDTYPE			(1)
#define P2LEN4RSVD1			(1)
#define P2LEN4RSVDSTATUS		(1)
#define P2LEN4RSVDTIME			(4)
#define P2LEN4SERIALNUMBER		(10)
#define P2LEN4STARTRECORDID	(2)
#define P2LEN4STARTTIMESTAMP	(4)
#define P2LEN4STOPWAY			(1)
#define P2LEN4SWVER			(2)
#define P2LEN4TOTALUSEDTIMES (2)
#define P2LEN4TOTALELECTRICITY (4)
#define P2LEN4UID				(16)
#define P2LEN4UNIXTIMESTAMP	(4)
#define P2LEN4UPDATEPERMIT		(1)
#define P2LEN4UPLOADRECORDBY	(1)
#define P2LEN4UNINUM		(4)
#define P2LEN4VOLTAGE			(4)

//日志表
#define P2LEN4ORDERIDTIMESTAMP	(4)
#define P2LEN4ORDERIDCID		(4)
#define P2LEN4ORDERIDRANDOM	(1)
#define P2LEN4PREPOWER			(4)
#define P2LEN4REALPOWER		(4)
#define P2LEN4CHRGRSTATE		(1)
#define P2LEN4CID4				(4)
#define P2LEN4STARTCHRGWAY		(1)
#define P2LEN4STOPREASON		(1)
#define P2LEN4ERRORCODE		(4)
#define P2LEN4RNG				(4)
#define P2LEN4EXT				(2)
//#define P2LEN4RSVD52			(52)
typedef struct{
	uint8_t cycleid[P2LEN4CYCLEID];//cycle id	记录id	(0 + 2)
	uint8_t uid[P2LEN4UID];//user id	用户id	(2 + 16)
	uint8_t orderidtimestamp[P2LEN4ORDERIDTIMESTAMP];//order ID timestamp	订单时间	(18 + 4)
	uint8_t orderidcid[P2LEN4ORDERIDCID];//设备ID1	(22 + 4)
	uint8_t orderidrandom[P2LEN4ORDERIDRANDOM];//系统控制资源	(26 + 1)
	uint8_t packageid[P2LEN4PACKAGEID];//套餐ID	(27 + 16)
	uint8_t prepower[P2LEN4PREPOWER];//目标电量(打算充的电量)	(43 + 4)[pre:预先打算]
	uint8_t starttimestamp[P2LEN4STARTTIMESTAMP];//启动时间	(47 + 4)
	uint8_t realpower[P2LEN4REALPOWER];//实际电量(当前已充电量/实时电量)	(51 + 4)
	uint8_t endtimestamp[P2LEN4ENDTIMESTAMP];//结束时间	(55 + 4)
	uint8_t chrgrstate[P2LEN4CHRGRSTATE];//记录状态	(59 + 1)
	uint8_t cid4[P2LEN4CID4];//设备ID2	(60 + 4)
	uint8_t startchrgway[P2LEN4STARTCHRGWAY];//开始方式	(64 + 1)
	uint8_t stopreason[P2LEN4STOPREASON];//结束方式	(65 + 1)
	uint8_t errorcode[P2LEN4ERRORCODE];//错误代码	(66 + 4)
	uint8_t rng[P2LEN4RNG];//随机数	(70 + 4)
	uint8_t ext[P2LEN4EXT];//扩展字段	(74 + 2)
	//uint8_t RSVD52[P2LEN4RSVD52];//保留字段	(76 + 52)
}p2record_t;

typedef struct{
    /* byte 0 */
    uint8_t emergency_bar:1;
    uint8_t not_remove_switch:1;
    uint8_t ble_link:1;
    uint8_t gprs_tcp_link:1;
    uint8_t wifi_tcp_link:1;
    uint8_t lan_tcp_link:1;
    uint8_t lan_dhcp_get:1;
    uint8_t lan_line_link:1;

    /* byte 1 */
    uint8_t plug_state:1;
    uint8_t gnd_state:1;
    uint8_t extern_power_supply:1;
    uint8_t battery_charging:1;
    uint8_t electri_lock2:1;
    uint8_t electri_lock1:1;
    uint8_t car2_sensor:1;
    uint8_t car1_sensor:1;

    /* byte 2 */
    uint8_t l3_state:1;
    uint8_t l2_state:1;
    uint8_t l1_state:1;
    uint8_t l3_switch:1;
    uint8_t l2_switch:1;
    uint8_t l1_switch:1;
    uint8_t cp1_s2_switch:1;
    uint8_t cp1_s1_switch:1;

    /* byte 3 */
    uint8_t rsvd:1;
    uint8_t hardware_screen_ver:1;
    uint8_t dns_online:1;
    uint8_t gateway_online:1;
    uint8_t meter2_aligned:1;
    uint8_t meter1_aligned:1;
    uint8_t board_cap_del:1; //1已删电容（防止电容对CP滤波，造成采集时间过长，不满足国标响应时间要求）
    uint8_t relay_new:1; //1新继电器（硬件优化老版继电器温度过高问题）

    /* byte 4 */
    uint8_t falt_meter:1;
    uint8_t falt_wifi:1;
    uint8_t falt_gprs:1;
    uint8_t falt_lcd:1;
    uint8_t falt_485:1;
    uint8_t falt_rfid:1;
    uint8_t falt_mcu_rtc:1;
    uint8_t falt_mcu_hse:1;

    /* byte 5 */
    uint8_t falt_temp2:1; //温感故障2(B)
    uint8_t falt_temp1:1; //温感故障1(A)
    uint8_t falt_meter2:1;
    uint8_t falt_button_battery:1;
    uint8_t falt_lan:1;
    uint8_t falt_sd:1;
    uint8_t falt_can:1;
    uint8_t falt_ble:1;

    uint8_t reserved3[2]; /* byte 6~7 */
    uint8_t signal_wifi; /* byte 8 */
    uint8_t signal_gprs; /* byte 9 */
    int8_t temp_mcu; /* byte 10 */
    uint8_t screen_kernel_ver; /* byte 11 */
    uint8_t gnd_analog_signal[4]; /* byte 12~15 */
    uint8_t cp1_pwm; /* byte 16 */
    uint8_t cp2_pwm; /* byte 17 */
    int8_t liveltem1; /* byte 18 */
    int8_t liveltem2; /* byte 19 */
    uint8_t cp1vcc[4]; /* byte 20~23 */
    uint8_t cp2vcc[4]; /* byte 24~27 */
    uint8_t ble_mac[6]; /* byte 28~33 */
    uint8_t lan_mac[6]; /* byte 34~39 */
    uint8_t wifi_mac[6]; /* byte 40~45 */
    uint8_t gprs_mac[6]; /* byte 46~51 */
    uint8_t lan_ip[4]; /* byte 52~55 */
    uint8_t wifi_ip[4]; /* byte 56~59 */
    uint8_t gprs_ip[4]; /* byte 60~63 */
    uint8_t meter1_v[4]; /* byte 64~67 */
    uint8_t meter2_v[4]; /* byte 68~71 */
    uint8_t meter1_a[4]; /* byte 72~75 */
    uint8_t meter2_a[4]; /* byte 76~79 */
}p2hwtable_t;

//配置
#define P2LEN4LANGUAGE	(1)
#define P2LEN4QRCODE	(64)
#define P2LEN4LAN_PORT	(2)
#define P2LEN4LAN_IP	(4)
#define P2LEN4WIFI_PORT	(2)
#define P2LEN4WIFI_IP	(4)
#define P2LEN4WIFI_NAME	(32)
#define P2LEN4WIFI_PASSWD	(32)
#define P2LEN4GPRS_PORT	(2)
#define P2LEN4GPRS_IP	(4)
#define P2LEN4PHASE	(1)
#define P2LEN4COST		(4)
#define P2LEN4RSVD179	(179)
typedef struct{
	uint8_t cid1[P2LEN4CID];
	uint8_t cid2[P2LEN4CID];
	uint8_t model[P2LEN4MODEL];
	uint8_t serialnumber[P2LEN4SERIALNUMBER];
	uint8_t language[P2LEN4LANGUAGE];
	uint8_t qrcode[P2LEN4QRCODE];
	uint8_t key[P2LEN4KEY];
	uint8_t lan_port[P2LEN4LAN_PORT];
	uint8_t lan_ip[P2LEN4LAN_IP];
	uint8_t	wifi_port[P2LEN4WIFI_PORT];
	uint8_t wifi_ip[P2LEN4WIFI_IP];
	uint8_t wifi_name[P2LEN4WIFI_NAME];
	uint8_t wifi_passwd[P2LEN4WIFI_PASSWD];
	uint8_t gprs_port[P2LEN4GPRS_PORT];
	uint8_t gprs_ip[P2LEN4GPRS_IP];
	uint8_t phase[P2LEN4PHASE];
	uint8_t maxcurrent[P2LEN4MAXCURRENT];
	uint8_t cost[P2LEN4COST];
	uint8_t RSVD179[P2LEN4RSVD179];
}p2config_t;


typedef struct{	//连接请求(0x80)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t model[P2LEN4MODEL];
	uint8_t serialnumber[P2LEN4SERIALNUMBER];
	uint8_t protclver[P2LEN4PROTCLVER];
	uint8_t swver[P2LEN4SWVER];
	uint8_t hwver[P2LEN4HWVER];
	uint8_t mac[P2LEN4MAC];
	uint8_t fwname[P2LEN4FWNAME];		//定制机型名
}p2c0x80_t;	//连接请求(0x80)

typedef struct{	//连接请求(0x80)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t key[P2LEN4KEY];
	uint8_t networkstatus[P2LEN4NETWORKSTATUS];
}p2r0x80_t;	//连接请求(0x80)


typedef struct{	//设备状态/电桩状态(心跳)(0x81)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t status[P2LEN4MODE];
	uint8_t outofservicetype[P2LEN4OUTOFSERVICETYPE];
	uint8_t cnntmode[P2LEN4CNNTMODE];
	uint8_t hwstatus[P2LEN4HWSTATUS];
}p2c0x81_t;	//设备状态/电桩状态(心跳)(0x81)

typedef struct{	//设备状态/电桩状态(心跳)(0x81)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t maxcurrent[P2LEN4MAXCURRENT];
	uint8_t maxcurrentvalid[P2LEN4MAXCURRENTVALID];
}p2r0x81_t;	//设备状态/电桩状态(心跳)(0x81)


typedef struct{	//充电请求(拍卡请求充电)(0x82)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t uid[P2LEN4UID];
	uint8_t packageid[P2LEN4PACKAGEID];
	uint8_t uninum[P2LEN4UNINUM];
	uint8_t cycleid[P2LEN4CYCLEID];
	uint8_t rfidtype[P2LEN4RFIDTYPE];
}p2c0x82_t;	//充电请求(拍卡请求充电)(0x82)

typedef struct{	//充电请求(拍卡请求充电)(0x82)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t chrgpermit[P2LEN4CHRGPERMIT];
	uint8_t packageid[P2LEN4PACKAGEID];
	uint8_t power[P2LEN4POWER];
}p2r0x82_t;	//充电请求(拍卡请求充电)(0x82)


typedef struct{	//充电临时动态日志(充电状态下的心跳)(0x83)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t liveltem[P2LEN4LIVELTEM];
	uint8_t cpvoltage[P2LEN4CPVOLTAGE];
	uint8_t cppwm[P2LEN4CPPWM];
	uint8_t voltage[P2LEN4VOLTAGE];
	uint8_t current[P2LEN4CURRENT];
	uint8_t power[P2LEN4POWER];
	uint8_t duration[P2LEN4DURATION];
	uint8_t record[P2LEN4RECORD];
}p2c0x83_t;	//充电临时动态日志(充电状态下的心跳)(0x83)

typedef struct{	//充电临时动态日志(充电状态下的心跳)(0x83)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
}p2r0x83_t;	//充电临时动态日志(充电状态下的心跳)(0x83)


typedef struct{	//停止告知(拍卡停止充电)(0x84)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t record[P2LEN4RECORD];
}p2c0x84_t;	//停止告知(拍卡停止充电)(0x84)

typedef struct{	//停止告知(拍卡停止充电)(0x84)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
}p2r0x84_t;	//停止告知(拍卡停止充电)(0x84)


typedef struct{	//写入配置(推送配置)(0x85)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t config[P2LEN4CONFIG];
}p2r0x85_t;	//写入配置(推送配置)(0x85)

typedef struct{	//写入配置(推送配置)(0x85)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
}p2c0x85_t;	//写入配置(推送配置)(0x85)


typedef struct{	//读取配置(0x86)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
}p2r0x86_t; //读取配置(0x86)

typedef struct{ //读取配置(0x86)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t config[P2LEN4CONFIG];
}p2c0x86_t; //读取配置(0x86)


typedef struct{	//请求读取记录(读取某时间段的记录)(0x88)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t starttimestamp[P2LEN4STARTTIMESTAMP];
	uint8_t endtimestamp[P2LEN4ENDTIMESTAMP];
}p2r0x88_t;	//请求读取记录(读取某时间段的记录)(0x88)

typedef struct{ //请求读取记录(读取某时间段的记录)(0x88)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t recordtot[P2LEN4RECORDTOT];
}p2c0x88_t; //请求读取记录(读取某时间段的记录)(0x88)

typedef struct{	//请求读取记录(读取某区间段的记录)(0x89)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t uploadrecordby[P2LEN4UPLOADRECORDBY];
	uint8_t startrecordid[P2LEN4STARTRECORDID];
	uint8_t endrecordid[P2LEN4ENDRECORDID];
}p2r0x89_t;	//请求读取记录(读取某区间段的记录)(0x89)

typedef struct{	//请求读取记录(读取某区间段的记录)(0x89)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t record[P2LEN4RECORD];
}p2c0x89_t;	//请求读取记录(读取某区间段的记录)(0x89)

typedef struct{	//按未上传读取记录(0x92)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
}p2r0x92_t;	//按未上传读取记录(0x92)

typedef struct{	//按未上传读取记录(0x92)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t recordtot[P2LEN4RECORDTOT];
}p2c0x92_t;	//按未上传读取记录(0x92)

typedef struct{	//按cycleid数组读取记录(0x79)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t cycleids8[P2LEN4CYCLEIDS8];
}p2r0x79_t;	//按cycleid数组读取记录(0x79)

typedef struct{	//按cycleid数组读取记录(0x79)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t recordcnt[P2LEN4RECORDCNT];
	uint8_t record[P2LEN4RECORD];
}p2c0x79_t;	//按cycleid数组读取记录(0x79)


typedef struct{	//标记记录已上传(0x93)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t oidstime[P2LEN4UNIXTIMESTAMP * 256];
}p2r0x93_t;	//标记记录已上传(0x93)

typedef struct{	//标记记录已上传(0x93)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
}p2c0x93_t;	//标记记录已上传(0x93)


typedef struct{	//后台充电开始(0x8a)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t evlinkid[P2LEN4EVLINKID];
	uint8_t packageid[P2LEN4PACKAGEID];
	uint8_t power[P2LEN4POWER];
}p2r0x8a_t;	//后台充电开始(0x8a)

typedef struct{	//后台充电开始(0x8a)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t abortedreason[P2LEN4ABORTEDREASON];
}p2c0x8a_t;	//后台充电开始(0x8a)


typedef struct{	//后台停止(充电)(0x8b)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t evlinkid[P2LEN4EVLINKID];
	uint8_t stopway[P2LEN4STOPWAY];
}p2r0x8b_t;	//后台停止(充电)(0x8b)

typedef struct{	//后台停止(充电)(0x8b)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t reason4stopfailed[P2LEN4REASON4STOPFAILED];
}p2c0x8b_t;	//后台停止(充电)(0x8b)


typedef struct{ //路由外网状态告知(0x9d)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t networkstatus[P2LEN4NETWORKSTATUS];
}p2r0x9d_t; //路由外网状态告知(0x9d)

typedef struct{ //路由外网状态告知(0x9d)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t reserved[1];
}p2c0x9d_t; //路由外网状态告知(0x9d) //0x9d


typedef struct{	//请求升级(0x8e)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t newswver[P2LEN4NEWSWVER];
	uint8_t filelength[P2LEN4FILELENGTH];
	uint8_t filebytesum[P2LEN4FILEBYTESUM];
}p2r0x8e_t;	//请求升级(0x8e)

typedef struct{	//升级传输(0x8f)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t fragmentpos[P2LEN4FRAGMENTID];
	uint8_t fragment[P2LEN4FRAGMENT];
}p2r0x8f_t;	//升级传输(0x8f)

typedef struct{	//请求升级(0x8e)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
	uint8_t updatepermit[P2LEN4UPDATEPERMIT];
}p2c0x8e_t;	//请求升级(0x8e)

typedef struct{	//升级传输(0x8f)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
}p2c0x8f_t;	//升级传输(0x8f)


typedef struct{ //重启电桩(0x90)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
}p2r0x90_t; //重启电桩(0x90)

typedef struct{ //重启电桩(0x90)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
}p2c0x90_t; //重启电桩(0x90)


typedef struct{
    p2head_t head;
    uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
}p2r0x87_t; //读取信息(0x87)

typedef struct{
    p2head_t head;
    uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
    uint8_t errcode[P2LEN4ERRCODE];
    uint8_t totalusedtimes[P2LEN4TOTALUSEDTIMES];
    uint8_t totalelectricity[P2LEN4TOTALELECTRICITY];
}p2c0x87_t; //读取信息(0x87)


typedef struct{	//后台时间校准(0x99)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t righttimestamp[P2LEN4UNIXTIMESTAMP];
}p2r0x99_t;	//后台时间校准(0x99)

typedef struct{	//后台时间校准(0x99)
	p2head_t head;
	uint8_t unixtimestamp[P2LEN4UNIXTIMESTAMP];
	uint8_t errcode[P2LEN4ERRCODE];
}p2c0x99_t;	//后台时间校准(0x99)


extern void p2rcpack_rc_head(void *dptr, void *prompt, uint8_t cmd, void *cid);
extern ssize_t p2crc_decrypt_unpadding(uint8_t *recv_buff, size_t recv_len,
		uint8_t *key);
extern int32_t p2padding_encrypt_crc_writen(int32_t fd, uint8_t *send_buff,
		size_t pkt_len, uint8_t *key);
extern size_t p2rcpack0x80(void *dptr, time_t unixtimestamp, uint8_t errcode,
		void *key, int status);	//连接请求(0x80)
extern size_t p2rcpack0x81(void *dptr, time_t unixtimestamp, uint8_t errcode,
		uint8_t maxcurrent, uint8_t maxcurrentvalid);	//设备状态/电桩状态(心跳)(0x81)
extern size_t p2rcpack0x82(void *dptr, time_t unixtimestamp, uint8_t errcode,
		uint8_t chrgpermit, void *packageid, uint32_t power);	//充电请求(拍卡请求充电)(0x82)
extern size_t p2rcpack0x83(void *dptr, time_t unixtimestamp, uint8_t errcode);	//充电临时动态日志(充电状态下的心跳)(0x83)
extern size_t p2rcpack0x84(void *dptr, time_t unixtimestamp, uint8_t errcode);	//停止告知(拍卡停止充电)(0x84)
extern size_t p2rcpack0x85(void *dptr, time_t unixtimestamp, void *config);	//写入配置(推送配置)(0x85)
extern size_t p2rcpack0x86(void *dptr, time_t unixtimestamp); //读取配置(0x86)
extern size_t p2rcpack0x88(void *dptr, time_t unixtimestamp, time_t
		starttimestamp, time_t endtimestamp);	//请求读取记录(读取某时间段的记录)(0x88)
extern size_t p2rcpack0x89(void *dptr, time_t unixtimestamp, uint8_t
		uploadrecordby, uint16_t startrecordid, uint16_t endrecordid);	//请求读取记录(读取某区间段的记录)(0x89)
extern size_t p2rcpack0x92(void *dptr, time_t unixtimestamp); //按未上传标记抄表(0x92)
extern size_t p2rcpack0x79(void *dptr, time_t unixtimestamp,/* uint8_t
															   cycleidcnt,*/
		uint8_t *cycleids);	//按cycleid数组读取记录(0x79)
extern size_t p2rcpack0x93(void *dptr, time_t unixtimestamp, uint8_t
		oidstimecnt, uint8_t *oidstime);	//标记记录已上传(0x93)
extern size_t p2rcpack0x8a(void *dptr, time_t unixtimestamp, void *evlinkid,
		void *packageid, uint32_t power);	//后台充电开始(0x8a)
extern size_t p2rcpack0x8b(void *dptr, time_t unixtimestamp, void *evlinkid,
		uint8_t stopway);	//后台停止(充电)(0x8b)
extern size_t p2rcpack0x9d(void *dptr, time_t unixtimestamp, int status); //路由外网状态告知(0x9d)
extern size_t p2rcpack0x8e(void *dptr, time_t unixtimestamp, void *newswver,
		uint32_t filelength, uint32_t filebytesum);	//请求升级(0x8e)
extern size_t p2rcpack0x8f(void *dptr, time_t unixtimestamp, uint32_t
		fragmentpos, void *fragment, size_t fragmentlen);	//升级传输(0x8f)
extern size_t p2rcpack0x90(void *dptr, time_t unixtimestamp); //重启电桩(0x90)
extern size_t p2rcpack0x87(void *dptr, time_t unixtimestamp); //读取信息(0x87)
extern size_t p2rcpack0x99(void *dptr, time_t unixtimestamp, uint32_t
		righttimestamp);	//后台时间校准(0x99)



#endif //__PROTOCOL_H__
