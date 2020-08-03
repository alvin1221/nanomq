/**
  * Created by Alvin on 2020/7/25.
  */


#include "nng/protocol/mqtt/pub_handler.h"
#include <stdio.h>
#include <string.h>
#include <nng/nng.h>
#include "core/nng_impl.h"

uint8_t put_var_integer(uint8_t *dest, uint32_t value);

uint32_t get_var_integer(const uint8_t *buf, int *pos);

static bool encode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet);

static bool decode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet);

static int32_t get_utf8_str(char *dest, const uint8_t *src, int *pos);

static int utf8_check(const char *str, size_t length);

static uint32_t power(uint32_t x, uint32_t n);

#if 0
uint32_t get_uint16(const uint8_t *buf);

uint32_t get_uint32(const uint8_t *buf);

uint64_t get_uint64(const uint8_t *buf);
#endif

static uint16_t get_variable_binary(uint8_t *dest, const uint8_t *src);


/**
 * publish logic
 *
 * @param msg
 * @param pub_packet
 */
void pub_handler(nng_msg *msg)
{
	struct pub_packet_struct pub_packet;

	if (decode_pub_message(msg, &pub_packet)) {
		//TODO save some useful publish message info and properties to global mqtt context while decode succeed
		//TODO search clients' pipe from topic_name
		//TODO do publish actions, eq: send payload to clients dependent on QoS ,topic alias if exists
	}
}

static bool encode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet)
{
	uint8_t tmp[4] = {0};
	uint32_t arr_len = 0;

	pub_packet->fixed_header.dup = 0;//reset dup to 0 when publish message is sent to client;

	nng_msg_append(msg, (uint8_t *) &pub_packet->fixed_header, 1);
	arr_len = put_var_integer(tmp, pub_packet->fixed_header.remain_len);
	nng_msg_append(msg, tmp, arr_len);

	switch (pub_packet->fixed_header.packet_type) {
		case PUBLISH:
			//topic name
			if (pub_packet->variable_header.publish.topic_name.str_len > 0) {
				nng_msg_append(msg, pub_packet->variable_header.publish.topic_name.str_body,
				               pub_packet->variable_header.publish.topic_name.str_len);
			}

			//identifier
			if (pub_packet->fixed_header.qos > 0) {
				nng_msg_append_u16(msg, pub_packet->variable_header.publish.packet_identifier);
			}

			//properties



			break;
		case PUBACK:
			break;
		case PUBREC:
			break;
		case PUBREL:
			break;
		case PUBCOMP:
			break;

		default:
			break;
	}


	return true;

}

static bool decode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet)
{
	uint8_t *msg_body = nng_msg_body(msg);
	size_t msg_len = nng_msg_len(msg);

	int pos = 0;
	int temp_pos = 0;
	int len;

	memcpy((uint8_t *) &pub_packet->fixed_header, msg_body, 1);
	++pos;
	pub_packet->fixed_header.remain_len = get_var_integer(msg_body, &pos);

	if (pub_packet->fixed_header.remain_len <= msg_len - pos) {

		switch (pub_packet->fixed_header.packet_type) {
			case PUBLISH:
				//variable header
				//topic length
				len = get_utf8_str(pub_packet->variable_header.publish.topic_name.str_body, msg_body + pos, &pos);
				if (len < 0 ||
				    strstr(pub_packet->variable_header.publish.topic_name.str_body, "+") ||
				    strstr(pub_packet->variable_header.publish.topic_name.str_body, "#")) {
					//protocol error
					return false;
				}
				pub_packet->variable_header.publish.topic_name.str_len = len;

				if (pub_packet->fixed_header.qos > 0) { //extract packet_identifier while qos > 0
					NNI_GET16(msg_body + pos, pub_packet->variable_header.publish.packet_identifier);
					pos += 2;
				}

				pub_packet->variable_header.publish.properties.len = get_var_integer(msg_body, &pos);
				int used_pos = pos;
				if (pub_packet->variable_header.publish.properties.len > 0) {
					for (uint32_t i = 0; i < pub_packet->variable_header.publish.properties.len;) {
						properties_type prop_type = get_var_integer(msg_body, &pos);
						switch (prop_type) {
							case PAYLOAD_FORMAT_INDICATOR:
								pub_packet->variable_header.publish.properties.content.publish.payload_fmt_indicator = *(
										msg_body +
										pos);
								++pos;
								++i;
								break;

							case MESSAGE_EXPIRY_INTERVAL:
								NNI_GET32(
										msg_body + pos,
										pub_packet->variable_header.publish.properties.content.publish.msg_expiry_interval);
								pos += 4;
								i += 4;
								break;

							case CONTENT_TYPE:
								pub_packet->variable_header.publish.properties.content.publish.content_type.str_len =
										get_utf8_str(
												pub_packet->variable_header.publish.properties.content.publish.content_type.str_body,
												msg_body,
												&pos);
								i = i +
								    pub_packet->variable_header.publish.properties.content.publish.content_type.str_len +
								    2;
								break;

							case TOPIC_ALIAS:
								NNI_GET16(
										msg_body + pos,
										pub_packet->variable_header.publish.properties.content.publish.topic_alias);
								pos += 2;
								i += 2;

								if (pub_packet->variable_header.publish.properties.content.publish.topic_alias == 0) {
									//TODO protocol error, free memory and return ?
//                                return false;
								}
								break;

							case RESPONSE_TOPIC:
								pub_packet->variable_header.publish.properties.content.publish.response_topic.str_len
										= get_utf8_str(
										pub_packet->variable_header.publish.properties.content.publish.response_topic.str_body,
										msg_body,
										&pos);
								i = i +
								    pub_packet->variable_header.publish.properties.content.publish.content_type.str_len +
								    2;
								break;

							case CORRELATION_DATA:
								pub_packet->variable_header.publish.properties.content.publish.correlation_data.data_len
										= get_variable_binary(
										pub_packet->variable_header.publish.properties.content.publish.correlation_data.data,
										msg_body + pos);
								pos += pub_packet->variable_header.publish.properties.content.publish.correlation_data.data_len +
								       2;
								i += pub_packet->variable_header.publish.properties.content.publish.correlation_data.data_len +
								     2;
								break;

							case USER_PROPERTY:
								pub_packet->variable_header.publish.properties.content.publish.response_topic.str_len =
										get_utf8_str(
												pub_packet->variable_header.publish.properties.content.publish.user_property.str_body,
												msg_body,
												&pos);
								i += pub_packet->variable_header.publish.properties.content.publish.user_property.str_len +
								     2;
								break;

							case SUBSCRIPTION_IDENTIFIER:
								temp_pos = pos;
								pub_packet->variable_header.publish.properties.content.publish.subscription_identifier =
										get_var_integer(msg_body, &pos);
								i += (pos - temp_pos);
								break;

							default:
								i++;
								break;
						}
					}
				}

				//payload
				pub_packet->payload_body.payload_len = msg_len - used_pos;
//				pub_packet->payload_body.payload = nni_alloc(pub_packet->payload_body.payload_len);
				pub_packet->payload_body.payload = msg_body + pos;
//				memcpy(pub_packet->payload_body.payload, msg_body + pos, pub_packet->payload_body.payload_len);
				break;

			case PUBACK:
			case PUBREC:
			case PUBREL:
			case PUBCOMP:
				NNI_GET16(msg_body + pos, pub_packet->variable_header.pub_arrc.packet_identifier);
				pos += 2;
				if (pub_packet->fixed_header.remain_len == 2) {
					//Reason code can be ignored when remaining length = 2 and reason code = 0x00(Success)
					pub_packet->variable_header.pub_arrc.reason_code = SUCCESS;
					break;
				}
				pub_packet->variable_header.pub_arrc.reason_code = *(msg_body + pos);
				++pos;
				if (pub_packet->fixed_header.remain_len > 4) {
					pub_packet->variable_header.pub_arrc.properties.len = get_var_integer(msg_body, &pos);
					for (uint32_t i = 0; i < pub_packet->variable_header.pub_arrc.properties.len;) {
						properties_type prop_type = get_var_integer(msg_body, &pos);
						switch (prop_type) {
							case REASON_STRING:
								pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.reason_string.str_len = get_utf8_str(
										pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.reason_string.str_body,
										msg_body, &pos);
								i += pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.reason_string.str_len +
								     2;
								break;

							case USER_PROPERTY:
								pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.user_property.str_len = get_utf8_str(
										pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.user_property.str_body,
										msg_body, &pos);
								i += pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.user_property.str_len +
								     2;
								break;

							default:
								i++;
								break;
						}
					}
				}
				break;

			default:
				break;
		}
		return true;

	}


	return false;
}


uint8_t put_var_integer(uint8_t *dest, uint32_t value)
{
	uint8_t len = 0;
	uint32_t init_val = 0x7F;

	for (int i = 0; i < sizeof(value); ++i) {

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

static uint32_t power(uint32_t x, uint32_t n)
{

	uint32_t val = 1;

	for (uint32_t i = 0; i <= n; ++i) {
		val = x * val;
	}

	return val / x;
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
	}
	while ((temp & 0x80) > 0 && i++ < 4);
	*pos = p;
	return result;
}

#if 0
uint32_t get_uint32(const uint8_t *buf)
{
	return (((uint32_t) ((uint8_t) (buf)[0])) >> 24u) +
		   (((uint32_t) ((uint8_t) (buf)[+1])) >> 16u) +
		   (((uint32_t) ((uint8_t) (buf)[+2])) >> 8u) +
		   (((uint32_t) (uint8_t) (buf)[+3]));
}

uint64_t get_uint64(const uint8_t *buf)
{
	return (((uint64_t) ((uint8_t) (buf)[0])) >> 56u) +
		   (((uint64_t) ((uint8_t) (buf)[1])) >> 48u) +
		   (((uint64_t) ((uint8_t) (buf)[2])) >> 40u) +
		   (((uint64_t) ((uint8_t) (buf)[3])) >> 32u) +
		   (((uint64_t) ((uint8_t) (buf)[4])) >> 24u) +
		   (((uint64_t) ((uint8_t) (buf)[5])) >> 16u) +
		   (((uint64_t) ((uint8_t) (buf)[6])) >> 8u) +
		   (((uint64_t) (uint8_t) (buf)[7]));
}

uint32_t get_uint16(const uint8_t *buf)
{
	return (((uint32_t) ((uint8_t) (buf)[0])) >> 8u) +
		   (((uint32_t) (uint8_t) (buf)[1]));
}
#endif

static uint16_t get_variable_binary(uint8_t *dest, const uint8_t *src)
{
	uint16_t len = 0;
	NNI_GET16(src, len);
//	dest = nni_alloc(len);

//	memcpy(dest, src + 2, len);
	dest = (uint8_t *) (src + 2);
	return len;
}

/**
 * Get utf-8 string
 *
 * @param dest output string
 * @param src input bytes
 * @param pos
 * @return string length -1: not utf-8, 0: empty string, >0 : normal utf-8 string
 */
static int32_t get_utf8_str(char *dest, const uint8_t *src, int *pos)
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

static int utf8_check(const char *str, size_t len)
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