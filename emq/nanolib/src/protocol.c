#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <openssl/md5.h>
#include "libevpower.h"
#include "libevpower/protocol.h"
#include "libevpower/CRC.h"

//hex(string) to int
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

void genrateuniquecode(uniquecode_t uniquecode)
{
	uint8_t randnumstr[20] = {0};
	unsigned char final[16] = {0};

	srand((int)time(0));
	sprintf((char *)randnumstr, "%u", rand());

	MD5(randnumstr, strlen((const char *)randnumstr), final);
	for(int i = 0; i < 16; i++)
	{
		sprintf(uniquecode + i * 2, "%2.2x", final[i]);
	}
}



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
void p1rcpack_head(void *dptr, void *prompt, uint8_t cmd, void *cid)
{
	p1head_t *head = (p1head_t *)dptr;
	if(prompt)		memcpy(head->prompt, prompt, P1LEN4PROMPT);
	head->len[0] = 0;
	head->cmd[0] = cmd;
	*(uint32_t *)head->cid = blsw32(atoi(cid));

	debug_msg("prompt:%s ", head->prompt);
	debug_msg("cmd:0x%02x ", head->cmd[0]);
	debug_msg("cid:%u(%s) ", ptou32(head->cid), (char *)cid);
}

size_t p1decrypt_unpadding(uint8_t *recv_buff, size_t pkt_len, uint8_t *key)
{
	uint8_t tmp[1500] = {0};
	size_t len = 0;

	pkt_len -= sizeof(p1head_t);
	recv_buff += sizeof(p1head_t);

	My_AES_CBC_Decrypt(key, recv_buff, pkt_len, tmp);
	RePadding(tmp, pkt_len, recv_buff, &len);

	len += sizeof(p1head_t);
	return len;
}

/* 函数：填充crc值 */
static void p1rcpack_crc(void *dptr, size_t len)
{
	uint16_t crc = 0;
	crc = getCRC(dptr, len);
	crc = blsw16(crc);
	memcpy(dptr + len, &crc, P1LEN4CRC);
}

static size_t writen(int fd, const void *buf, size_t count)
{
	size_t nleft = count; //剩余的需要写入的字节数。
	ssize_t nwritten; //成功写入的字节数。
	char *bufp = (char*) buf; //将缓冲的指针强制转换为字符类型的指针。

	while (nleft > 0)
	{
		if ((nwritten = write(fd, bufp, nleft)) < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		} else if (nwritten == 0)
			continue;
		bufp += nwritten;
		nleft -= nwritten;
	}
	return count;
}

/* *********************************************************************
 * 对包进行 crc padding encrypt writen
 * 	fd - 发送的到的目标socket套接字
 * 	send_buff - 发送的缓冲区
 * 	pkt_len - 发送的报文原始长度
 * 	key - 加密key
 * 返回 成功返回0，失败返回负数
 * ********************************************************************/
int32_t p1crc_padding_encrypt_writen(int32_t fd, uint8_t *send_buff, size_t pkt_len, uint8_t *key)
{
	uint8_t tmp_buff[1500] = {0};
	size_t org_payload_len = pkt_len - sizeof(p1head_t);	//原始有效载荷长度
	size_t encrypt_payload_len = 0;	//最终有效载荷长度
	size_t send_len = 0;			//最终发送的长度
	uint8_t *payload = send_buff + sizeof(p1head_t);	//有效载荷起始位置
	p1head_t *head = (p1head_t *)send_buff;

	if (fd == 0 || send_buff == NULL || pkt_len == 0 || pkt_len > sizeof(tmp_buff) || key == NULL){
		debug_msg("fd%d, pkt_len%u", fd, pkt_len);
		debug_msg("send_buff:%s", send_buff == NULL ? "NULL" : "NOT NULL");
		debug_msg("pkt_len %s sizeof(tmp_buff)", pkt_len > sizeof(tmp_buff) ? ">" : "<=");
		debug_msg("key:%s", key == NULL ? "NULL" : "NOT NULL");
		debug_msg("错误:[0x%02x]参数有误", head->cmd[0]);
		goto err;
	}

	//校验
	p1rcpack_crc(send_buff, pkt_len - P1LEN4CRC);
	//填充，加密
	Padding(payload, org_payload_len, tmp_buff, (uint32_t *)&encrypt_payload_len);
	My_AES_CBC_Encrypt(key, tmp_buff, encrypt_payload_len, payload);

	send_len = sizeof(p1head_t) + encrypt_payload_len;
	//发送
	if (writen(fd, send_buff, send_len) != send_len)
		goto err;

	return 0;

err:
	debug_msg("错误：命令发送失败");
	return -1;
}


size_t p1rcpack0x11(void *dptr, void *ip, time_t unixtimestamp, void *key) //路->桩:连接响应(0x11)
{
	p10x11_t *pkt = (p10x11_t *)dptr;
	if(ip)				memcpy(pkt->ip, ip, P1LEN4IP);//*(uint32_t *)pkt->ip = blsw32(ip);
	*(uint32_t *)pkt->unixtimestamp = blsw32(unixtimestamp);
	if(key)			memcpy(pkt->key, key, P1LEN4KEY);
	pkt->head.len[0] = sizeof(p10x11_t) -4;

	debug_msg("ip:%d.%d.%d.%d ", pkt->ip[0], pkt->ip[1], pkt->ip[2], pkt->ip[3]);
	debug_msg("unixtimestamp:%u(%u) ", ptou32(pkt->unixtimestamp), (uint32_t)unixtimestamp);
	debug_msg("key:%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
			pkt->key[0], pkt->key[1],	pkt->key[2], pkt->key[3],
			pkt->key[4], pkt->key[5], pkt->key[6], pkt->key[7],
			pkt->key[8], pkt->key[9], pkt->key[10], pkt->key[11],
			pkt->key[12], pkt->key[13], pkt->key[14], pkt->key[15]);
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p10x11_t);
}

size_t p1rcpack0x35(void *dptr, time_t unixtimestamp, uint8_t maxcurrentsupply) //路->桩:心跳响应(0x35)
{
	p10x35_t *pkt = (p10x35_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = blsw32(unixtimestamp);
	pkt->maxcurrentsupply[0] = maxcurrentsupply;
	pkt->head.len[0] = sizeof(p10x35_t) -4;

	debug_msg("unixtimestamp:%u(%u) ", ptou32(pkt->unixtimestamp), (uint32_t)unixtimestamp);
	debug_msg("maxcurrentsupply:%u\n", pkt->maxcurrentsupply[0]);
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p10x35_t);
}

size_t p1rcpack0x55(void *dptr, time_t unixtimestamp, uint8_t targetmode, uint8_t systemmessage, uint16_t chargingduration, uint16_t chargingcode, void *evlinkid, uint8_t maxcurrent) //路->桩:充电请求响应(0x55)
{
	p10x55_t *pkt = (p10x55_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = blsw32(unixtimestamp);
	pkt->targetmode[0] = targetmode;
	pkt->systemmessage[0] = systemmessage;
	*(uint16_t *)pkt->chargingduration = blsw16(chargingduration);
	*(uint16_t *)pkt->chargingcode = blsw16(chargingcode);
	if(evlinkid)			memcpy(pkt->evlinkid, evlinkid, P1LEN4EVLINKID);
	pkt->maxcurrent[0] = maxcurrent;
	pkt->head.len[0] = sizeof(p10x55_t) -4;

	debug_msg("unixtimestamp:%u(%u) ", ptou32(pkt->unixtimestamp), (uint32_t)unixtimestamp);
	debug_msg("targetmode:%u ", pkt->targetmode[0]);
	debug_msg("systemmessage:%u ", pkt->systemmessage[0]);
	debug_msg("chargingduration:%u(%u) ", ptou16(pkt->chargingduration), chargingduration);
	debug_msg("chargingcode:%u(%u) ", ptou16(pkt->chargingcode), chargingcode);
	debug_msg("evlinkid:%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c ",
			pkt->evlinkid[0], pkt->evlinkid[1],	pkt->evlinkid[2], pkt->evlinkid[3],
			pkt->evlinkid[4], pkt->evlinkid[5], pkt->evlinkid[6], pkt->evlinkid[7],
			pkt->evlinkid[8], pkt->evlinkid[9], pkt->evlinkid[10], pkt->evlinkid[11],
			pkt->evlinkid[12], pkt->evlinkid[13], pkt->evlinkid[14], pkt->evlinkid[15]);
	debug_msg("maxcurrent:%u\n", pkt->maxcurrent[0]);
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p10x55_t);
}

size_t p1rcpack0x57(void *dptr, time_t unixtimestamp, uint8_t targetmode, uint8_t systemmessage, void *evlinkid) //路->桩:停止告知响应(0x57)
{
	p10x57_t *pkt = (p10x57_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = blsw32(unixtimestamp);
	pkt->targetmode[0] = targetmode;
	pkt->systemmessage[0] = systemmessage;
	if(evlinkid)			memcpy(pkt->evlinkid, evlinkid, P1LEN4EVLINKID);
	pkt->head.len[0] = sizeof(p10x57_t) -4;

	debug_msg("unixtimestamp:%u(%u) ", ptou32(pkt->unixtimestamp), (uint32_t)unixtimestamp);
	debug_msg("targetmode:%u ", pkt->targetmode[0]);
	debug_msg("systemmessage:%u ", pkt->systemmessage[0]);
	debug_msg("evlinkid:%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
			pkt->evlinkid[0], pkt->evlinkid[1],	pkt->evlinkid[2], pkt->evlinkid[3],
			pkt->evlinkid[4], pkt->evlinkid[5], pkt->evlinkid[6], pkt->evlinkid[7],
			pkt->evlinkid[8], pkt->evlinkid[9], pkt->evlinkid[10], pkt->evlinkid[11],
			pkt->evlinkid[12], pkt->evlinkid[13], pkt->evlinkid[14], pkt->evlinkid[15]);
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p10x57_t);
}

size_t p1rcpack0xa2(void *dptr, time_t unixtimestamp, uint8_t reset, uint8_t unlock) //路->桩:控制电桩(0xa2)
{
	p10xa2_t *pkt = (p10xa2_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = blsw32(unixtimestamp);
	pkt->reset[0] = reset;
	pkt->unlock[0] = unlock;
	pkt->head.len[0] = sizeof(p10xa2_t) -4;

	debug_msg("unixtimestamp:%u(%u) ", ptou32(pkt->unixtimestamp), (uint32_t)unixtimestamp);
	debug_msg("reset:%u ", pkt->reset[0]);
	debug_msg("unlock:%u\n", pkt->unlock[0]);
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p10xa2_t);
}

size_t p1rcpack0xa4(void *dptr, time_t unixtimestamp, uint16_t targetrecodeid, time_t starttimestamp, time_t endtimestamp, uint16_t chargercode) //路->桩:抄表(0xa4)
{
	p10xa4_t *pkt = (p10xa4_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = blsw32(unixtimestamp);
	*(uint16_t *)pkt->targetrecodeid = blsw16(targetrecodeid);
	*(uint32_t *)pkt->starttimestamp = blsw32(starttimestamp);
	*(uint32_t *)pkt->endtimestamp = blsw32(endtimestamp);
	*(uint16_t *)pkt->chargercode = blsw16(chargercode);
	pkt->head.len[0] = sizeof(p10xa4_t) -4;

	debug_msg("unixtimestamp:%u(%u) ", ptou32(pkt->unixtimestamp), (uint32_t)unixtimestamp);
	debug_msg("targetrecodeid:%u(%u) ", ptou16(pkt->targetrecodeid), targetrecodeid);
	debug_msg("starttimestamp:%u(%u) ", ptou32(pkt->starttimestamp), (uint32_t)starttimestamp);
	debug_msg("endtimestamp:%u(%u) ", ptou32(pkt->endtimestamp), (uint32_t)endtimestamp);
	debug_msg("chargercode:%u(%u)\n", ptou16(pkt->chargercode), chargercode);
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p10xa4_t);
}

size_t p1rcpack0x90(void *dptr, void *chargerversion, uint32_t framelength, uint8_t framenum) //路->桩:更新指令(0x90)
{
	p10x90_t *pkt = (p10x90_t *)dptr;
	if(chargerversion)		memcpy(pkt->chargerversion, chargerversion, P1LEN4CHARGERVERSION);
	*(uint32_t *)pkt->framelength = blsw32(framelength);
	pkt->framenum[0] = framenum;
	pkt->head.len[0] = sizeof(p10x90_t) -4;

	debug_msg("升级目标版本[%u.%u],固件大小(%u),分成(%u)片", pkt->chargerversion[0], pkt->chargerversion[1], framelength, framenum);
	debug_msg("chargerversion:%u.%u ", pkt->chargerversion[0], pkt->chargerversion[1]);
	debug_msg("framelength:%u(%u) ", ptou32(pkt->framelength), framelength);
	debug_msg("framenum:%u\n", pkt->framenum[0]);
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p10x90_t);
}

size_t p1rcpack0x94(void *dptr, void *chargerversion, uint32_t currentframelength, uint8_t framesequence, void *framedata) //路->桩:发送更新碎片(0x94)
{
	p10x94_t *pkt = (p10x94_t *)dptr;
	if(chargerversion)	  memcpy(pkt->chargerversion, chargerversion,
			P1LEN4CHARGERVERSION);
	*(uint32_t *)pkt->currentframelength = blsw32(currentframelength);
	pkt->framesequence[0] = framesequence;
	if(framedata)		memcpy(pkt->framedata, framedata,
			currentframelength);/**/
	pkt->head.len[0] = 0; //EVRC协议定义错误，该字段应该为2个字节才对，暂时赋为0

	debug_msg("升级目标版本[%u.%u],当前碎片长度(%u),碎片序号(%u),碎片内容(...)", pkt->chargerversion[0], pkt->chargerversion[1], currentframelength, framesequence);
	debug_msg("chargerversion:%u.%u ", pkt->chargerversion[0], pkt->chargerversion[1]);
	debug_msg("currentframelength:%u(%u) ", ptou32(pkt->currentframelength), currentframelength);
	debug_msg("framesequence:%u ", pkt->framesequence[0]);
	debug_msg("framedata:...\n");
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p1head_t) + P1LEN4CHARGERVERSION + sizeof(currentframelength) + sizeof(framesequence) + currentframelength + P1LEN4CRC;
}

size_t p1rcpack0x51(void *dptr, uint8_t confignum, uint32_t configlength, uint32_t currentconfiglength, uint8_t framesequence, void *framedata) //路->桩:推送配置(0x51)
{
	p10x51_t *pkt = (p10x51_t *)dptr;
	pkt->confignum[0] = confignum;
	*(uint32_t *)pkt->configlength = blsw32(configlength);
	*(uint32_t *)pkt->currentconfiglength = blsw32(currentconfiglength);
	pkt->framesequence[0] = framesequence;
	if(framedata)	  memcpy(pkt->framedata, framedata,
			currentconfiglength);/**/
	pkt->head.len[0] = 0; //EVRC协议定义错误，该字段应该为2个字节才对，暂时赋为0

	debug_msg("confignum:%u ", pkt->confignum[0]);
	debug_msg("configlength:%u(%u) ", ptou32(pkt->configlength), configlength);
	debug_msg("currentconfiglength:%u(%u) ", ptou32(pkt->currentconfiglength), currentconfiglength);
	debug_msg("framesequence:%u ", pkt->framesequence[0]);
	debug_msg("framedata:...\n");
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p1head_t) + sizeof(confignum) + sizeof(configlength) + sizeof(currentconfiglength) + sizeof(framesequence) + currentconfiglength + P1LEN4CRC;
}

size_t p1rcpack0x60(void *dptr, void *oid, uint16_t chargercurrent, void *pid, uint32_t targetenegy, void *startuid) //路->桩:远程启动充电(0x60)
{
	p10x60_t *pkt = (p10x60_t *)dptr;
	if(oid)		  memcpy(pkt->oid, oid, P1LEN4OID);
	*(uint16_t *)pkt->chargercurrent = blsw16(chargercurrent);
	if(pid)		  memcpy(pkt->pid, pid, P1LEN4PID);
	sprintf((char *)pkt->targetenegy, "%04d", targetenegy);
	if(startuid)		memcpy(pkt->startuid, startuid, P1LEN4STARTUID);
	pkt->head.len[0] = sizeof(p10x60_t) -4;

	debug_msg("oid:%s ", pkt->oid);
	debug_msg("chargercurrent:%u(%u) ", ptou16(pkt->chargercurrent), chargercurrent);
	debug_msg("pid:%s ", pkt->pid);
	debug_msg("targetenegy:%c%c%c%c ", pkt->targetenegy[0], pkt->targetenegy[1], pkt->targetenegy[2], pkt->targetenegy[3]);
	debug_msg("startuid:%s\n", pkt->startuid);
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p10x60_t);
}

size_t p1rcpack0x62(void *dptr, void *stopuid, uint8_t stoplimit) //路->桩:远程停止充电(0x62)
{
	p10x62_t *pkt = (p10x62_t *)dptr;
	if(stopuid)			memcpy(pkt->stopuid, stopuid, P1LEN4STOPUID);
	pkt->stoplimit[0] = stoplimit;
	pkt->head.len[0] = sizeof(p10x62_t) -4;

	debug_msg("stopuid:%s ", pkt->stopuid);
	debug_msg("stoplimit:%u\n", pkt->stoplimit[0]);
	debug_msg("head.len:%u\n", pkt->head.len[0]);
	return sizeof(p10x62_t);
}




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
static uint16_t p2_calc_crc(uint16_t crc, uint32_t ch)
{
	uint16_t i;
#define POLINOMIAL   0x1021
	ch <<= 8;
	for (i = 8; i > 0; i--)
	{
		if ((ch ^ crc ) & 0x8000)
			crc = (crc << 1) ^ POLINOMIAL;
		else
			crc <<= 1;
		ch <<= 1;
	}
	return crc;
}

static uint16_t p2_pub_crc_count(uint8_t *p, uint16_t n)
{
	uint8_t  ch;
	uint16_t  i,crc=0;

	for (i = 0; i < n; i++)
	{
		ch = *p++;
		crc = p2_calc_crc(crc,(uint16_t)ch);
	}
	return crc;
}

/* 函数：填充crc值 */
static void p2rcpack_rc_crc(void *dptr, size_t len)
{
	uint16_t crc = 0;
	crc = p2_pub_crc_count(dptr, len);
	//p2rc_dbg("计算%u字节的校验值得到:[0x%04x]\n", len, crc);
	memcpy(dptr +len, &crc, P2LEN4CRC);
}

void p2rcpack_rc_head(void *dptr, void *prompt, uint8_t cmd, void *cid)
{
	p2head_t *head = (p2head_t *)dptr;
	if(prompt)		memcpy(head->prompt, prompt, P2LEN4PROMPT);
	*(uint16_t *)head->len = 0;
	head->cmd[0] = cmd;
	if(cid)		memcpy(head->cid, cid, P2LEN4CID);

	debug_msg("prompt:%s ", head->prompt);
	debug_msg("cmd:0x%02x ", head->cmd[0]);
	debug_msg("cid:%c%c%c%c%c%c%c%c ", head->cid[0], head->cid[1], head->cid[2], head->cid[3], head->cid[4], head->cid[5], head->cid[6], head->cid[7]);
}

/* *********************************************************************
 * 对接收到的包进行 crc decrypt unpadding
 *	recv_buff - 接收到的报文缓冲区
 * 	recv_len - 接收到的报文长度
 * 	key - 解密key
 * 返回 成功返回原始报文长度，失败返回负数
 * ********************************************************************/
ssize_t p2crc_decrypt_unpadding(uint8_t *recv_buff, size_t recv_len, uint8_t *key)
{
	uint16_t crc = 0;
	uint8_t tmp_buff[1500] = {0};
	size_t org_pkt_len = 0;
	size_t org_payload_len = 0;	//原始有效载荷长度
	size_t encrypt_payload_len = recv_len - sizeof(p2head_t) - P2LEN4CRC;	//加密后有效载荷长度
	uint8_t *payload = recv_buff + sizeof(p2head_t);	//有效载荷起始位置
	p2head_t *head = (p2head_t *)recv_buff;

	if (recv_buff == NULL || recv_len == 0 || recv_len > sizeof(tmp_buff) || key == NULL){
		debug_msg("错误：[0x%02x]参数有误", head->cmd[0]);
		goto err;
	}

	//CRC校验
	crc = p2_pub_crc_count(recv_buff, recv_len - P2LEN4CRC);
	if (crc != ptou16(recv_buff + recv_len - P2LEN4CRC) ) {
		debug_msg("错误：校验失败:crc[%04x] != recv_crc[%04x]", crc, ptou16(recv_buff + recv_len - P2LEN4CRC));
		goto err;
	}

	My_AES_CBC_Decrypt(key, payload, encrypt_payload_len, tmp_buff);
	if(RePadding(tmp_buff, encrypt_payload_len, payload, &org_payload_len) < 0){
		debug_msg("错误：%8.8s cmd[0x%02x] key[%16.16s] RePadding失败", head->cid, head->cmd[0], (char *)key);
		goto err;
	}

	org_pkt_len = sizeof(p2head_t) + org_payload_len;
	return org_pkt_len;

err:
	debug_msg("命令发送失败");
	return -1;
}

/* *********************************************************************
 * 对包进行 padding encrypt writen
 * 	fd - 发送的到的目标socket套接字
 * 	send_buff - 发送的缓冲区
 * 	pkt_len - 发送的报文原始长度
 * 	key - 加密key
 * 返回 成功返回0，失败返回负数
 * ********************************************************************/
int32_t p2padding_encrypt_crc_writen(int32_t fd, uint8_t *send_buff, size_t pkt_len, uint8_t *key)
{
	uint8_t tmp_buff[1500] = {0};
	size_t org_payload_len = pkt_len - sizeof(p2head_t);	//原始有效载荷长度
	size_t encrypt_payload_len = 0;	//最终有效载荷长度
	size_t send_len = 0;			//最终发送的长度
	uint8_t *payload = send_buff + sizeof(p2head_t);	//有效载荷起始位置
	p2head_t *head = (p2head_t *)send_buff;

	if (fd == 0 || send_buff == NULL || pkt_len == 0 || pkt_len > sizeof(tmp_buff) || key == NULL){
		debug_msg("fd%d, pkt_len%u", fd, pkt_len);
		debug_msg("send_buff:%s", send_buff == NULL ? "NULL" : "NOT NULL");
		debug_msg("pkt_len %s sizeof(tmp_buff)", pkt_len > sizeof(tmp_buff) ? ">" : "<=");
		debug_msg("key:%s", key == NULL ? "NULL" : "NOT NULL");
		debug_msg("错误:[0x%02x]参数有误", head->cmd[0]);
		goto err;
	}

	//填充，加密
	Padding(payload, org_payload_len, tmp_buff, (uint32_t *)&encrypt_payload_len);
	My_AES_CBC_Encrypt(key, tmp_buff, encrypt_payload_len, payload);

	send_len = sizeof(p2head_t) + encrypt_payload_len + P2LEN4CRC;
	*(uint16_t *)head->len = send_len - P2LEN4PROMPT - P2LEN4LEN;

	//校验
	p2rcpack_rc_crc(send_buff, send_len - P2LEN4CRC);
	//发送(如果是后台命令，需要等待3000ms，超时重发????)
	if (writen(fd, send_buff, send_len) != send_len)
		goto err;

	return 0;

err:
	debug_msg("错误：命令发送失败");
	return -1;
}


size_t p2rcpack0x80(void *dptr, time_t unixtimestamp, uint8_t errcode, void *key, int status)	//连接请求(0x80)
{
	p2r0x80_t *pkt = (p2r0x80_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	pkt->errcode[0] = errcode;
	if(key)			memcpy(pkt->key, key, P2LEN4KEY);
	pkt->networkstatus[0] = status;

	debug_msg("--数据  ");
	debug_msg("errcode:%d ", pkt->errcode[0]);
	debug_msg("key:%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
		pkt->key[0], pkt->key[1], pkt->key[2], pkt->key[3],
		pkt->key[4], pkt->key[5], pkt->key[6], pkt->key[7],
		pkt->key[8], pkt->key[9], pkt->key[10], pkt->key[11],
		pkt->key[12], pkt->key[13], pkt->key[14], pkt->key[15]);
	return sizeof(p2r0x80_t);
}

size_t p2rcpack0x81(void *dptr, time_t unixtimestamp, uint8_t errcode, uint8_t maxcurrent, uint8_t maxcurrentvalid)	//设备状态/电桩状态(心跳)(0x81)
{
	p2r0x81_t *pkt = (p2r0x81_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	pkt->errcode[0] = errcode;
	pkt->maxcurrent[0] = maxcurrent;
	pkt->maxcurrentvalid[0] = maxcurrentvalid;

	debug_msg("----包数据----\n");
	debug_msg("errcode:%d ", pkt->errcode[0]);
	debug_msg("maxcurrent:%d ", pkt->maxcurrent[0]);
	debug_msg("maxcurrentvalid:%d\n", pkt->maxcurrentvalid[0]);
	return sizeof(p2r0x81_t);
}

size_t p2rcpack0x82(void *dptr, time_t unixtimestamp, uint8_t errcode, uint8_t chrgpermit, void *packageid, uint32_t power)	//充电请求(拍卡请求充电)(0x82)
{
	p2r0x82_t *pkt = (p2r0x82_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	pkt->errcode[0] = errcode;
	pkt->chrgpermit[0] = chrgpermit;
	memcpy(pkt->packageid, packageid, P2LEN4PACKAGEID);
	*(uint32_t *)pkt->power = power;

	return sizeof(p2r0x82_t);
}

size_t p2rcpack0x83(void *dptr, time_t unixtimestamp, uint8_t errcode)	//充电临时动态日志(充电状态下的心跳)(0x83)
{
	p2r0x83_t *pkt = (p2r0x83_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	pkt->errcode[0] = errcode;

	debug_msg("----包数据----\n");
	debug_msg("errcode:%d\n", pkt->errcode[0]);
	return sizeof(p2r0x83_t);
}

size_t p2rcpack0x84(void *dptr, time_t unixtimestamp, uint8_t errcode)	//停止告知(拍卡停止充电)(0x84)
{
	p2r0x84_t *pkt = (p2r0x84_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	pkt->errcode[0] = errcode;

	debug_msg("----包数据----\n");
	debug_msg("errcode:%d\n", pkt->errcode[0]);
	return sizeof(p2r0x84_t);
}

size_t p2rcpack0x85(void *dptr, time_t unixtimestamp, void *config)	//写入配置(推送配置)(0x85)
{
	p2r0x85_t *pkt = (p2r0x85_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	if(config)		memcpy(pkt->config, config, P2LEN4CONFIG);

	return sizeof(p2r0x85_t);
}

size_t p2rcpack0x86(void *dptr, time_t unixtimestamp) //读取配置(0x86)
{
	p2r0x86_t *pkt = (p2r0x86_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;

	return sizeof(p2r0x86_t);
}

size_t p2rcpack0x88(void *dptr, time_t unixtimestamp, time_t starttimestamp, time_t endtimestamp)	//请求读取记录(读取某时间段的记录)(0x88)
{
	p2r0x88_t *pkt = (p2r0x88_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	*(uint32_t *)pkt->starttimestamp = starttimestamp;
	*(uint32_t *)pkt->endtimestamp = endtimestamp;

	debug_msg("----包数据----\n");
	debug_msg("starttimestamp:%u ", ptou32(pkt->starttimestamp));
	debug_msg("endtimestamp:%u\n", ptou32(pkt->endtimestamp));
	return sizeof(p2r0x88_t);
}

size_t p2rcpack0x89(void *dptr, time_t unixtimestamp, uint8_t uploadrecordby, uint16_t startrecordid, uint16_t endrecordid)	//请求读取记录(读取某区间段的记录)(0x89)
{
	p2r0x89_t *pkt = (p2r0x89_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	pkt->uploadrecordby[0] = uploadrecordby;
	*(uint16_t *)pkt->startrecordid = startrecordid;
	*(uint16_t *)pkt->endrecordid = endrecordid;

	debug_msg("----包数据----\n");
	debug_msg("startrecordid:%u ", ptou16(pkt->startrecordid));
	debug_msg("endrecordid:%u\n", ptou16(pkt->endrecordid));
	return sizeof(p2r0x89_t);
}

size_t p2rcpack0x92(void *dptr, time_t unixtimestamp) //按未上传标记抄表(0x92)
{
	p2r0x92_t *pkt = (p2r0x92_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;

	return sizeof(p2r0x92_t);
}

size_t p2rcpack0x79(void *dptr, time_t unixtimestamp,/* uint8_t cycleidcnt,*/ uint8_t *cycleids)	//按cycleid数组读取记录(0x79)
{
	p2r0x79_t *pkt = (p2r0x79_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	//memcpy(pkt->cycleids, cycleids, P2LEN4CYCLEID * cycleidcnt);
	//return sizeof(p2r0x79_t) + P2LEN4UNIXTIMESTAMP + P2LEN4CYCLEID * cycleidcnt ;
	memcpy(pkt->cycleids8, cycleids, P2LEN4CYCLEIDS8);
	return sizeof(p2r0x79_t);
}

size_t p2rcpack0x93(void *dptr, time_t unixtimestamp, uint8_t oidstimecnt, uint8_t *oidstime)	//标记记录已上传(0x93)
{
	p2r0x93_t *pkt = (p2r0x93_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	memcpy(pkt->oidstime, oidstime, P2LEN4UNIXTIMESTAMP * oidstimecnt);

	return sizeof(p2head_t) + P2LEN4UNIXTIMESTAMP + P2LEN4UNIXTIMESTAMP * oidstimecnt ;
}

size_t p2rcpack0x8a(void *dptr, time_t unixtimestamp, void *evlinkid, void *packageid, uint32_t power)	//后台充电开始(0x8a)
{
	p2r0x8a_t *pkt = (p2r0x8a_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	if(evlinkid)		memcpy(pkt->evlinkid, evlinkid, P2LEN4EVLINKID);
	if(packageid)		memcpy(pkt->packageid, packageid, P2LEN4PACKAGEID);
	*(uint32_t *)pkt->power = power;

	debug_msg("----包数据----\n");
	debug_msg("evlinkid:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x ",
			pkt->evlinkid[0], pkt->evlinkid[1], pkt->evlinkid[2], pkt->evlinkid[3],
			pkt->evlinkid[4], pkt->evlinkid[5], pkt->evlinkid[6], pkt->evlinkid[7],
			pkt->evlinkid[8], pkt->evlinkid[9], pkt->evlinkid[10], pkt->evlinkid[11],
			pkt->evlinkid[12], pkt->evlinkid[13], pkt->evlinkid[14], pkt->evlinkid[15]);
	debug_msg("power:%u\n", ptou32(pkt->power));
	return sizeof(p2r0x8a_t);
}

size_t p2rcpack0x8b(void *dptr, time_t unixtimestamp, void *evlinkid, uint8_t stopway)	//后台停止(充电)(0x8b)
{
	p2r0x8b_t *pkt = (p2r0x8b_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	if(evlinkid)		memcpy(pkt->evlinkid, evlinkid, P2LEN4EVLINKID);
	pkt->stopway[0] = stopway;

	debug_msg("----包数据----\n");
	debug_msg("evlinkid:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x ",
		pkt->evlinkid[0], pkt->evlinkid[1], pkt->evlinkid[2], pkt->evlinkid[3],
		pkt->evlinkid[4], pkt->evlinkid[5], pkt->evlinkid[6], pkt->evlinkid[7],
		pkt->evlinkid[8], pkt->evlinkid[9], pkt->evlinkid[10], pkt->evlinkid[11],
		pkt->evlinkid[12], pkt->evlinkid[13], pkt->evlinkid[14], pkt->evlinkid[15]);
	debug_msg("stopway:%u\n", pkt->stopway[0]);
	return sizeof(p2r0x8b_t);
}

size_t p2rcpack0x9d(void *dptr, time_t unixtimestamp, int status) //路由外网状态告知(0x9d)
{
	p2r0x9d_t *pkt = (p2r0x9d_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	pkt->networkstatus[0] = status;

	return sizeof(p2r0x9d_t);
}

size_t p2rcpack0x8e(void *dptr, time_t unixtimestamp, void *newswver, uint32_t filelength, uint32_t filebytesum)	//请求升级(0x8e)
{
	p2r0x8e_t *pkt = (p2r0x8e_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	if(newswver)	memcpy(pkt->newswver, newswver, P2LEN4NEWSWVER);
	*(uint32_t *)pkt->filelength = filelength;
	*(uint32_t *)pkt->filebytesum = filebytesum;

	debug_msg("升级目标版本[%s],固件大小(%u),字节和(%u)", (char *)newswver, filelength, filebytesum);
	debug_msg("----包数据----\n");
	debug_msg("newswver:%u.%u ", pkt->newswver[0], pkt->newswver[1]);
	debug_msg("filelength:%u ", ptou32(pkt->filelength));
	debug_msg("filebytesum:%u\n", ptou32(pkt->filebytesum));
	return sizeof(p2r0x8e_t);
}

size_t p2rcpack0x8f(void *dptr, time_t unixtimestamp, uint32_t fragmentpos, void *fragment, size_t fragmentlen)	//升级传输(0x8f)
{
	p2r0x8f_t *pkt = (p2r0x8f_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	*(uint32_t *)pkt->fragmentpos = fragmentpos;
	if(fragment)		memcpy(pkt->fragment, fragment, fragmentlen);

	debug_msg("升级传输 pos[%u] len[%u]", fragmentpos, fragmentlen);
	debug_msg("----包数据----\n");
	debug_msg("fragmentpos:%u ", ptou32(pkt->fragmentpos));
	debug_msg("fragment:(%uBytes)\n", fragmentlen);
	//int i;
	//for(i = 0; i < fragmentlen; i++)
	//{
	//	debug_msg("\t%u:[0x%02x]\n", i, pkt->fragment[i]);
	//}
	return sizeof(p2head_t) + P2LEN4UNIXTIMESTAMP + P2LEN4FRAGMENTID + fragmentlen;
}

size_t p2rcpack0x90(void *dptr, time_t unixtimestamp) //重启电桩(0x90)
{
	p2r0x90_t *pkt = (p2r0x90_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;

	return sizeof(p2r0x90_t);
}

size_t p2rcpack0x87(void *dptr, time_t unixtimestamp) //读取信息(0x87)
{
    p2r0x87_t *pkt = (p2r0x87_t *)dptr;
    *(uint32_t *)pkt->unixtimestamp = unixtimestamp;

    return sizeof(p2r0x87_t);
}

size_t p2rcpack0x99(void *dptr, time_t unixtimestamp, uint32_t righttimestamp)	//后台时间校准(0x99)
{
	p2r0x99_t *pkt = (p2r0x99_t *)dptr;
	*(uint32_t *)pkt->unixtimestamp = unixtimestamp;
	*(uint32_t *)pkt->righttimestamp = righttimestamp;

	return sizeof(p2r0x99_t);
}
