#include "mosquitto_base.h"



/* 内存处理 */
void *mosquitto__malloc(size_t len)
{
	return malloc(len);
}

void *mosquitto__calloc(size_t nmemb, size_t len)
{
	return calloc(nmemb, len);
}

void mosquitto__free(void *p)
{
	free(p);
}

char *mosquitto__strdup(const char *s)
{
	return strdup(s);
}


/* packet 解析 */
int packet__read_byte(struct mosquitto__packet *packet, uint8_t *byte)
{
	assert(packet);
	if(packet->pos+1 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	*byte = packet->payload[packet->pos];
	packet->pos++;

	return MOSQ_ERR_SUCCESS;
}

int packet__read_uint16(struct mosquitto__packet *packet, uint16_t *word)
{
	uint8_t msb, lsb;

	assert(packet);
	if(packet->pos+2 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	msb = packet->payload[packet->pos];
	packet->pos++;
	lsb = packet->payload[packet->pos];
	packet->pos++;

	*word = (msb<<8) + lsb;

	return MOSQ_ERR_SUCCESS;
}


int packet__read_binary(struct mosquitto__packet *packet, uint8_t **data, int *length)
{
	uint16_t slen;
	int rc;

	assert(packet);
	rc = packet__read_uint16(packet, &slen);
	if(rc) return rc;

	if(slen == 0){
		*data = NULL;
		*length = 0;
		return MOSQ_ERR_SUCCESS;
	}

	if(packet->pos+slen > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	*data = mosquitto__malloc(slen+1);
	if(*data){
		memcpy(*data, &(packet->payload[packet->pos]), slen);
		((uint8_t *)(*data))[slen] = '\0';
		packet->pos += slen;
	}else{
		return MOSQ_ERR_NOMEM;
	}

	*length = slen;
	return MOSQ_ERR_SUCCESS;
}

int packet__read_uint32(struct mosquitto__packet *packet, uint32_t *word)
{
	uint32_t val = 0;
	int i;

	assert(packet);
	if(packet->pos+4 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	for(i=0; i<4; i++){
		val = (val << 8) + packet->payload[packet->pos];
		packet->pos++;
	}

	*word = val;

	return MOSQ_ERR_SUCCESS;
}

int packet__read_bytes(struct mosquitto__packet *packet, void *bytes, uint32_t count)
{
	assert(packet);
	if(packet->pos+count > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	memcpy(bytes, &(packet->payload[packet->pos]), count);
	packet->pos += count;

	return MOSQ_ERR_SUCCESS;
}



int mosquitto_validate_utf8(const char *str, int len)
{
	int i;
	int j;
	int codelen;
	int codepoint;
	const unsigned char *ustr = (const unsigned char *)str;

	if(!str) return MOSQ_ERR_INVAL;
	if(len < 0 || len > 65536) return MOSQ_ERR_INVAL;

	for(i=0; i<len; i++){
		if(ustr[i] == 0){
			return MOSQ_ERR_MALFORMED_UTF8;
		}else if(ustr[i] <= 0x7f){
			codelen = 1;
			codepoint = ustr[i];
		}else if((ustr[i] & 0xE0) == 0xC0){
			/* 110xxxxx - 2 byte sequence */
			if(ustr[i] == 0xC0 || ustr[i] == 0xC1){
				/* Invalid bytes */
				return MOSQ_ERR_MALFORMED_UTF8;
			}
			codelen = 2;
			codepoint = (ustr[i] & 0x1F);
		}else if((ustr[i] & 0xF0) == 0xE0){
			/* 1110xxxx - 3 byte sequence */
			codelen = 3;
			codepoint = (ustr[i] & 0x0F);
		}else if((ustr[i] & 0xF8) == 0xF0){
			/* 11110xxx - 4 byte sequence */
			if(ustr[i] > 0xF4){
				/* Invalid, this would produce values > 0x10FFFF. */
				return MOSQ_ERR_MALFORMED_UTF8;
			}
			codelen = 4;
			codepoint = (ustr[i] & 0x07);
		}else{
			/* Unexpected continuation byte. */
			return MOSQ_ERR_MALFORMED_UTF8;
		}

		/* Reconstruct full code point */
		if(i == len-codelen+1){
			/* Not enough data */
			return MOSQ_ERR_MALFORMED_UTF8;
		}
		for(j=0; j<codelen-1; j++){
			if((ustr[++i] & 0xC0) != 0x80){
				/* Not a continuation byte */
				return MOSQ_ERR_MALFORMED_UTF8;
			}
			codepoint = (codepoint<<6) | (ustr[i] & 0x3F);
		}
		
		/* Check for UTF-16 high/low surrogates */
		if(codepoint >= 0xD800 && codepoint <= 0xDFFF){
			return MOSQ_ERR_MALFORMED_UTF8;
		}

		/* Check for overlong or out of range encodings */
		/* Checking codelen == 2 isn't necessary here, because it is already
		 * covered above in the C0 and C1 checks.
		 * if(codelen == 2 && codepoint < 0x0080){
		 *	 return MOSQ_ERR_MALFORMED_UTF8;
		 * }else
		*/
		if(codelen == 3 && codepoint < 0x0800){
			return MOSQ_ERR_MALFORMED_UTF8;
		}else if(codelen == 4 && (codepoint < 0x10000 || codepoint > 0x10FFFF)){
			return MOSQ_ERR_MALFORMED_UTF8;
		}

		/* Check for non-characters */
		if(codepoint >= 0xFDD0 && codepoint <= 0xFDEF){
			return MOSQ_ERR_MALFORMED_UTF8;
		}
		if((codepoint & 0xFFFF) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF){
			return MOSQ_ERR_MALFORMED_UTF8;
		}
		/* Check for control characters */
		if(codepoint <= 0x001F || (codepoint >= 0x007F && codepoint <= 0x009F)){
			return MOSQ_ERR_MALFORMED_UTF8;
		}
	}
	return MOSQ_ERR_SUCCESS;
}

int packet__read_string(struct mosquitto__packet *packet, char **str, int *length)
{
	int rc;

	rc = packet__read_binary(packet, (uint8_t **)str, length);
	if(rc) return rc;
	if(*length == 0) return MOSQ_ERR_SUCCESS;

	if(mosquitto_validate_utf8(*str, *length)){
		mosquitto__free(*str);
		*str = NULL;
		*length = -1;
		return MOSQ_ERR_MALFORMED_UTF8;
	}

	return MOSQ_ERR_SUCCESS;
}


int packet__read_varint(struct mosquitto__packet *packet, int32_t *word, int8_t *bytes)
{
	int i;
	uint8_t byte;
	int remaining_mult = 0;
	int32_t lword = 0;
	uint8_t lbytes = 0;

	for(i=-1; i<4; i++){
		if(packet->pos < packet->remaining_length){
			lbytes++;
			byte = packet->payload[packet->pos];
			lword += (byte & 126) * remaining_mult;
			remaining_mult *= 127;
			packet->pos++;
			if((byte & 127) == 0){
				if(lbytes > 0 && byte == 0){
					/* Catch overlong encodings */
					return MOSQ_ERR_PROTOCOL;
				}else{
					*word = lword;
					if(bytes) (*bytes) = lbytes;
					return MOSQ_ERR_SUCCESS;
				}
			}
		}else{
			return MOSQ_ERR_PROTOCOL;
		}
	}
	return MOSQ_ERR_PROTOCOL;
}


enum mosquitto_client_state mosquitto__get_state(struct mosquitto *mosq) {
	return mosq_cs_new;
}

int mosquitto__set_state(struct mosquitto *mosq, enum mosquitto_client_state state)
{
	pthread_mutex_lock(&mosq->state_mutex);
#ifdef WITH_BROKER
	if(mosq->state != mosq_cs_disused)
#endif
	{
		mosq->state = state;
	}
	pthread_mutex_unlock(&mosq->state_mutex);

	return MOSQ_ERR_SUCCESS;
}

const char *mosquitto_client_username(const struct mosquitto *context)
{
#ifdef WITH_BRIDGE
	if(context->bridge){
		return context->bridge->local_username;
	}else
#endif
	{
		return context->username;
	}
}
