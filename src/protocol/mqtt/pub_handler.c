/**
  * Created by Alvin on 2020/7/25.
  */


#include "nng/protocol/mqtt/pub_handler.h"
#include "nng/protocol/mqtt/mqtt_parser.h"
#include <stdio.h>
#include <string.h>
#include <nng/nng.h>
#include "core/nng_impl.h"


#if 0
uint32_t get_uint16(const uint8_t *buf);

uint32_t get_uint32(const uint8_t *buf);

uint64_t get_uint64(const uint8_t *buf);
#endif

static uint32_t append_bytes_with_type(nng_msg *msg, uint8_t type, uint8_t *content, uint32_t len);

/**
 * publish logic
 *
 * @param msg
 * @param pub_packet
 */
void pub_handler(nng_msg *msg)
{
	struct pub_packet_struct *pub_packet = NULL; //TODO get from nng_msg context

	if (decode_pub_message(msg, pub_packet)) {

		//process topic alias
		//TODO get "TOPIC Alias Maximum" from CONNECT Packet Properties ,
		// topic_alias can't be larger than Topic Alias Maximum when the latter isn't 0;
		// Compare with TOPIC Alias Maximum;
		if (pub_packet->variable_header.publish.properties.content.publish.topic_alias.has_value) {

			if (pub_packet->variable_header.publish.properties.content.publish.topic_alias.value == 0) {
				//Protocol Error
				//TODO Send a DISCONNECT Packet with Reason Code "0x94" before close the connection;
				return;
			}

			if (pub_packet->variable_header.publish.topic_name.str_len == 0) {
				//TODO
				// 1, query the entire Topic Name through Topic alias
				// 2, if query failed, Send a DISCONNECT Packet with Reason Code "0x82" before close the connection and return
				// 3, if query succeed, query node and data structure through Topic Name
			} else {
				//TODO
				// 1, update Map value of Topic Alias
				// 2, query node and data structure through Topic Name
			}
		}


		//TODO save some useful publish message info and properties to global mqtt context while decode succeed

		//TODO do publish actions, eq: send payload to clients dependent on QoS ,topic alias if exists

		if (pub_packet->fixed_header.dup) {


		}

		if (pub_packet->fixed_header.retain == 1) {
			//TODO store this message to the topic node
		}

		//TODO compare Publish QoS with Subscribe OoS, decide by the minimum;

		switch (pub_packet->fixed_header.qos) {
			case 0:
				break;
			case 1:
				break;
			case 2:
				break;
			default:
				//Error Qos
				break;
		}

		//TODO search clients' pipe from topic_name



	}
}


static uint32_t append_bytes_with_type(nng_msg *msg, uint8_t type, uint8_t *content, uint32_t len)
{
	if (len > 0) {
		nng_msg_append(msg, &type, 1);
		nng_msg_append_u16(msg, len);
		nng_msg_append(msg, content, len);
		return 0;
	}

	return 1;

}


bool encode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet)
{
	uint8_t  tmp[4]  = {0};
	uint32_t arr_len = 0;

	properties_type prop_type;


	switch (pub_packet->fixed_header.packet_type) {
		case PUBLISH:
			/*fixed header*/
			nng_msg_append(msg, (uint8_t *) &pub_packet->fixed_header, 1);
			arr_len = put_var_integer(tmp, pub_packet->fixed_header.remain_len);
			nng_msg_append(msg, tmp, arr_len);
			/*variable header*/
			//topic name
			if (pub_packet->variable_header.publish.topic_name.str_len > 0) {
				nng_msg_append_u16(msg, pub_packet->variable_header.publish.topic_name.str_len);
				nng_msg_append(msg, pub_packet->variable_header.publish.topic_name.str_body,
				               pub_packet->variable_header.publish.topic_name.str_len);
			}

			//identifier
			if (pub_packet->fixed_header.qos > 0) {
				nng_msg_append_u16(msg, pub_packet->variable_header.publish.packet_identifier);
			}

			//properties
			//properties length
			memset(tmp, 0, sizeof(tmp));
			arr_len = put_var_integer(tmp, pub_packet->variable_header.publish.properties.len);
			nng_msg_append(msg, tmp, arr_len);

			//Payload Format Indicator
			prop_type = PAYLOAD_FORMAT_INDICATOR;
			nng_msg_append(msg, &prop_type, 1);
			nng_msg_append(msg, &pub_packet->variable_header.publish.properties.content.publish.payload_fmt_indicator,
			               sizeof(pub_packet->variable_header.publish.properties.content.publish.payload_fmt_indicator));

			//Message Expiry Interval
			prop_type = MESSAGE_EXPIRY_INTERVAL;
			nng_msg_append(msg, &prop_type, 1);
			nng_msg_append_u32(msg,
			                   pub_packet->variable_header.publish.properties.content.publish.msg_expiry_interval.value);

			//Topic Alias
			if (pub_packet->variable_header.publish.properties.content.publish.topic_alias.has_value) {
				prop_type = TOPIC_ALIAS;
				nng_msg_append(msg, &prop_type, 1);
				nng_msg_append_u16(msg,
				                   pub_packet->variable_header.publish.properties.content.publish.topic_alias.value);
			}

			//Response Topic
			append_bytes_with_type(msg, RESPONSE_TOPIC,
			                       (uint8_t *) pub_packet->variable_header.publish.properties.content.publish.response_topic.str_body,
			                       pub_packet->variable_header.publish.properties.content.publish.response_topic.str_len);

			//Correlation Data
			append_bytes_with_type(msg, CORRELATION_DATA,
			                       pub_packet->variable_header.publish.properties.content.publish.correlation_data.data,
			                       pub_packet->variable_header.publish.properties.content.publish.correlation_data.data_len);

			//User Property
			append_bytes_with_type(msg, USER_PROPERTY,
			                       (uint8_t *) pub_packet->variable_header.publish.properties.content.publish.user_property.str_body,
			                       pub_packet->variable_header.publish.properties.content.publish.user_property.str_len);

			//Subscription Identifier
			if (pub_packet->variable_header.publish.properties.content.publish.subscription_identifier.has_value) {
				prop_type = SUBSCRIPTION_IDENTIFIER;
				nng_msg_append(msg, &prop_type, 1);
				memset(tmp, 0, sizeof(tmp));
				arr_len = put_var_integer(tmp,
				                          pub_packet->variable_header.publish.properties.content.publish.subscription_identifier.value);
				nng_msg_append(msg, tmp, arr_len);
			}

			//CONTENT TYPE
			append_bytes_with_type(msg, CONTENT_TYPE,
			                       (uint8_t *) pub_packet->variable_header.publish.properties.content.publish.content_type.str_body,
			                       pub_packet->variable_header.publish.properties.content.publish.content_type.str_len);

			//payload
			if (pub_packet->payload_body.payload_len > 0) {
				nng_msg_append(msg, pub_packet->payload_body.payload, pub_packet->payload_body.payload_len);
			}
			break;

		case PUBREL:
			pub_packet->fixed_header.qos = 1; //set bit1 = 1;
		case PUBACK:
		case PUBREC:
		case PUBCOMP:
			/*fixed header*/
			nng_msg_append(msg, (uint8_t *) &pub_packet->fixed_header, 1);
			arr_len = put_var_integer(tmp, pub_packet->fixed_header.remain_len);
			nng_msg_append(msg, tmp, arr_len);

			/*variable header*/
			//identifier
			nng_msg_append_u16(msg, pub_packet->variable_header.pub_arrc.packet_identifier);

			//reason code
			if (pub_packet->fixed_header.remain_len > 2) {
				uint8_t reason_code = pub_packet->variable_header.pub_arrc.reason_code;
				nng_msg_append(msg, (uint8_t *) &reason_code, sizeof(reason_code));

				//properties
				if (pub_packet->fixed_header.remain_len >= 4) {

					memset(tmp, 0, sizeof(tmp));
					arr_len = put_var_integer(tmp, pub_packet->variable_header.pub_arrc.properties.len);
					nng_msg_append(msg, tmp, arr_len);

					//reason string
					append_bytes_with_type(msg, REASON_STRING,
					                       (uint8_t *) pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.reason_string.str_body,
					                       pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.reason_string.str_len);

					//user properties
					append_bytes_with_type(msg, USER_PROPERTY,
					                       (uint8_t *) pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.user_property.str_body,
					                       pub_packet->variable_header.pub_arrc.properties.content.pub_arrc.user_property.str_len);

				}

			}
			break;

		default:
			break;
//		case RESERVED:
//			break;
//		case CONNECT:
//			break;
//		case CONNACK:
//			break;
//		case SUBSCRIBE:
//			break;
//		case SUBACK:
//			break;
//		case UNSUBSCRIBE:
//			break;
//		case UNSUBACK:
//			break;
//		case PINGREQ:
//			break;
//		case PINGRESP:
//			break;
//		case DISCONNECT:
//			break;
//		case AUTH:
//			break;
	}


	return true;

}


bool decode_pub_message(nng_msg *msg, struct pub_packet_struct *pub_packet)
{
	uint8_t *msg_body = nng_msg_body(msg);
	size_t  msg_len   = nng_msg_len(msg);

	int pos      = 0;
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
						//TODO the same property cannot appear twice
						switch (prop_type) {
							case PAYLOAD_FORMAT_INDICATOR:
								if (pub_packet->variable_header.publish.properties.content.publish.payload_fmt_indicator.has_value ==
								    false) {
									pub_packet->variable_header.publish.properties.content.publish.payload_fmt_indicator.value =
											*(msg_body + pos);
									pub_packet->variable_header.publish.properties.content.publish.payload_fmt_indicator.has_value = true;
									++pos;
									++i;
								} else {
									//Protocol Error
									return false;
								}
								break;

							case MESSAGE_EXPIRY_INTERVAL:
								if (pub_packet->variable_header.publish.properties.content.publish.msg_expiry_interval.has_value ==
								    false) {
									NNI_GET32(
											msg_body + pos,
											pub_packet->variable_header.publish.properties.content.publish.msg_expiry_interval.value);
									pub_packet->variable_header.publish.properties.content.publish.msg_expiry_interval.has_value = true;
									pos += 4;
									i += 4;
								} else {
									//Protocol Error
									return false;
								}
								break;

							case CONTENT_TYPE:
								if (pub_packet->variable_header.publish.properties.content.publish.content_type.str_len ==
								    0) {
									pub_packet->variable_header.publish.properties.content.publish.content_type.str_len =
											get_utf8_str(
													pub_packet->variable_header.publish.properties.content.publish.content_type.str_body,
													msg_body,
													&pos);
									i = i +
									    pub_packet->variable_header.publish.properties.content.publish.content_type.str_len +
									    2;
								} else {
									//Protocol Error
									return false;
								}
								break;

							case TOPIC_ALIAS:
								if (pub_packet->variable_header.publish.properties.content.publish.topic_alias.has_value ==
								    false) {
									NNI_GET16(
											msg_body + pos,
											pub_packet->variable_header.publish.properties.content.publish.topic_alias.value);
									pub_packet->variable_header.publish.properties.content.publish.topic_alias.has_value = true;
									pos += 2;
									i += 2;
								} else {
									//Protocol Error
									return false;
								}
								break;

							case RESPONSE_TOPIC:
								if (pub_packet->variable_header.publish.properties.content.publish.response_topic.str_len ==
								    0) {
									pub_packet->variable_header.publish.properties.content.publish.response_topic.str_len
											= get_utf8_str(
											pub_packet->variable_header.publish.properties.content.publish.response_topic.str_body,
											msg_body,
											&pos);
									i = i +
									    pub_packet->variable_header.publish.properties.content.publish.content_type.str_len +
									    2;
								} else {
									//Protocol Error
									return false;
								}

								break;

							case CORRELATION_DATA:
								if (pub_packet->variable_header.publish.properties.content.publish.correlation_data.data_len ==
								    0) {
									pub_packet->variable_header.publish.properties.content.publish.correlation_data.data_len
											= get_variable_binary(
											pub_packet->variable_header.publish.properties.content.publish.correlation_data.data,
											msg_body + pos);
									pos += pub_packet->variable_header.publish.properties.content.publish.correlation_data.data_len +
									       2;
									i += pub_packet->variable_header.publish.properties.content.publish.correlation_data.data_len +
									     2;
								} else {
									//Protocol Error
									return false;
								}
								break;

							case USER_PROPERTY:
								if (pub_packet->variable_header.publish.properties.content.publish.response_topic.str_len ==
								    0) {
									pub_packet->variable_header.publish.properties.content.publish.response_topic.str_len =
											get_utf8_str(
													pub_packet->variable_header.publish.properties.content.publish.user_property.str_body,
													msg_body,
													&pos);
									i += pub_packet->variable_header.publish.properties.content.publish.user_property.str_len +
									     2;
								} else {
									//Protocol Error
									return false;
								}
								break;

							case SUBSCRIPTION_IDENTIFIER:
								if (pub_packet->variable_header.publish.properties.content.publish.subscription_identifier.has_value ==
								    false) {
									temp_pos = pos;
									pub_packet->variable_header.publish.properties.content.publish.subscription_identifier.value =
											get_var_integer(msg_body, &pos);
									i += (pos - temp_pos);
									pub_packet->variable_header.publish.properties.content.publish.subscription_identifier.has_value = true;
									//Protocol error while Subscription Identifier = 0
									if (pub_packet->variable_header.publish.properties.content.publish.subscription_identifier.value ==
									    0) {
										return false;
									}
								} else {
									//Protocol Error
									return false;
								}
								break;

							default:
								i++;
								break;
						}
					}
				}

				//payload
				pub_packet->payload_body.payload_len             = msg_len - used_pos;
				pub_packet->payload_body.payload                 = msg_body + pos;
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
