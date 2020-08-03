/**
  * Created by Alvin on 2020/7/25.
  */


#include "nng/protocol/mqtt/pub_handler.h"
#include "nng/protocol/mqtt/connect_parser.h"
#include <stdio.h>
#include <string.h>
#include <nng/nng.h>
#include "core/nng_impl.h"


static bool encode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet);

static bool decode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet);


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

	nng_msg_append(msg, &pub_packet->fixed_header, 1);


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

				if (pub_packet->fixed_header.flag_bits.qos > 0) { //extract packet_identifier while qos > 0
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


uint8_t put_var_integer(uint8_t *dest, uint32_t value) {
	uint8_t bytes_len;
	//TODO not completed

//	for (int i = 0; i < 4; ++i) {
//		(value * i*8) & 0x80
//	}

	return bytes_len;
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