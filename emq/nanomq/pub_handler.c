/**
  * Created by Alvin on 2020/7/25.
  */


#include <stdio.h>
#include <string.h>
#include <nng/nng.h>
#include <nng/protocol/mqtt/mqtt.h>
#include <nng/protocol/mqtt/mqtt_parser.h>
#include <include/zmalloc.h>
#include <include/mqtt_db.h>
#include <apps/broker.h>

#include "include/pub_handler.h"
#include "include/nanomq.h"

#define SUPPORT_MQTT5_0 0

#if 0
uint32_t get_uint16(const uint8_t *buf);

uint32_t get_uint32(const uint8_t *buf);

uint64_t get_uint64(const uint8_t *buf);
#endif


static char *bytes_to_str(const unsigned char *src, char *dest, int src_len);
static void print_hex(const char *prefix, const unsigned char *src, int src_len);
static uint32_t append_bytes_with_type(nng_msg *msg, uint8_t type, uint8_t *content, uint32_t len);
static void
forward_msg(struct topic_and_node *res_node, char *topic, nng_msg *send_msg, struct pub_packet_struct *pub_packet);

/**
 * pub handler
 *
 * @param arg: struct work pointer
 */
void pub_handler(void *arg)
{
	emq_work *work = arg;

	work->pub_packet                       = nng_alloc(sizeof(struct pub_packet_struct *));

	struct topic_and_node    *res_node;
	struct pub_packet_struct *pub_response = NULL;;


	nng_msg *send_msg = NULL;
	debug_msg("start decode msg");
	if (decode_pub_message(work->msg, work->pub_packet)) {
		debug_msg("end decode msg");


		switch (work->pub_packet->fixed_header.packet_type) {
			case PUBLISH:
				debug_msg("handing cmd[%d] msg\n", work->pub_packet->fixed_header.packet_type);
#if SUPPORT_MQTT5_0
				//process topic alias (For MQTT 5.0)
				//TODO get "TOPIC Alias Maximum" from CONNECT Packet Properties ,
				// topic_alias can't be larger than Topic Alias Maximum when the latter isn't equals 0;
				// Compare with TOPIC Alias Maximum;
				if (pub_packet->variable_header.publish.properties.content.publish.topic_alias.has_value) {

					if (pub_packet->variable_header.publish.properties.content.publish.topic_alias.value == 0) {
						//Protocol Error
						//TODO Send a DISCONNECT Packet with Reason Code "0x94" before close the connection (MQTT 5.0);
						return;
					}

					if (pub_packet->variable_header.publish.topic.str_len == 0) {
						//TODO
						// 1, query the entire Topic Name through Topic alias
						// 2, if query failed, Send a DISCONNECT Packet with Reason Code "0x82" before close the connection and return (MQTT 5.0);
						// 3, if query succeed, query node and data structure through Topic Name

					} else {
						topic = &pub_packet->variable_header.publish.topic;
						//TODO
						// 1, update Map value of Topic Alias
						// 2, query node and data structure through Topic Name
					}
				}
				//TODO save some useful publish message info and properties to global mqtt context while decode succeed
#endif

				//TODO add some logic if support MQTT3.1.1 & MQTT5.0
				debug_msg("topic: %*.*s\n",
				          work->pub_packet->variable_header.publish.topic_name.str_len,
				          work->pub_packet->variable_header.publish.topic_name.str_len,
				          work->pub_packet->variable_header.publish.topic_name.str_body);

				//do publish actions, eq: send payload to clients dependent on QoS ,topic alias if exists
				debug_msg("res_node[1]: %p\n",res_node);
				res_node = (struct topic_and_node *) zmalloc(sizeof(struct topic_and_node ));
//				res_node = (struct topic_and_node *) nng_alloc(sizeof(struct topic_and_node ));
//				if (res_node == NULL) {
//					debug_msg("res_node zmalloc failed!");
//					break;
//				}
				debug_msg("res_node[2]: %p\n", res_node);
				debug_msg("start search node! work->db: %p, work->db->root->topic: %s, work->db->root->next: %p\n", work->db,work->db->root->topic, work->db->root->next);
				search_node(work->db, work->pub_packet->variable_header.publish.topic_name.str_body, &res_node);
				debug_msg("end search node!");

				if (work->pub_packet->fixed_header.retain == 1) {
					//store this message to the topic node
					res_node->node->retain  = true;
					res_node->node->len     = work->pub_packet->payload_body.payload_len;
					res_node->node->message = nng_alloc(res_node->node->len);

					memcpy((uint8_t *) res_node->node->message, work->pub_packet->payload_body.payload,
					       res_node->node->len);//according to node.len, free memory before delete this node

					if (res_node->node->state == UNEQUAL) {
						//TODO add node but client_id is unnecessary;
					}

				} else {
					if (res_node->node->state == UNEQUAL) {
						//topic not found,
						zfree(res_node);
						debug_msg("wocaosb");
						work->msg   = NULL;
						work->state = RECV;
						nng_ctx_recv(work->ctx, work->aio);
						return;
					}
				}

				//TODO compare Publish QoS with Subscribe OoS, decide by the maximum;
				switch (work->pub_packet->fixed_header.qos) {
					case 0:
						//publish only once
						work->pub_packet->fixed_header.dup = 0;


						forward_msg(res_node, work->pub_packet->variable_header.publish.topic_name.str_body, send_msg,
						            work->pub_packet);



						break;

					case 1:
						work->pub_packet->fixed_header.dup = 0;

						forward_msg(res_node, work->pub_packet->variable_header.publish.topic_name.str_body, send_msg,
						            work->pub_packet);

						pub_response = nng_alloc(sizeof(struct pub_packet_struct *));

						pub_response->fixed_header.packet_type = PUBACK;
						pub_response->fixed_header.dup         = 0;
						pub_response->fixed_header.qos         = 0;
						pub_response->fixed_header.retain      = 0;
						pub_response->fixed_header.remain_len  = 2;

						pub_response->variable_header.puback.packet_identifier =
								work->pub_packet->variable_header.publish.packet_identifier;

						encode_pub_message(send_msg, pub_response);

						//response PUBACK to client
						work->state = SEND;
						work->msg   = send_msg;
						nng_aio_set_msg(work->aio, work->msg);
						nng_ctx_send(work->ctx, work->aio);

						nng_free(pub_response, sizeof(struct pub_packet_struct *));

						break;

					case 2:
						break;

					default:
						//Error Qos
						break;
				}

				break;

			case PUBACK:
				break;

			case PUBREL:
				break;

			case PUBREC:
				break;
			case PUBCOMP:
				break;

			default:
				break;
		}
	}
}


static void
forward_msg(struct topic_and_node *res_node, char *topic, nng_msg *send_msg, struct pub_packet_struct *pub_packet)
{
	if(res_node->node->state == UNEQUAL){
		debug_msg("forward failed, node state = %d",res_node->node->state );
		return;
	}
	struct client *clients = search_client(res_node->node, &topic);
debug_msg("sbsbsbsbsbs\n");
	emq_work *client_work;

	while (clients) {
		encode_pub_message(send_msg, pub_packet);

		client_work = (emq_work *) clients->ctxt;

		client_work->state = SEND;
		client_work->msg   = send_msg;

		debug_msg("send msg to client.id: %s, ctx.id: %d\n", clients->id, client_work->ctx.id);
		print_hex("msg header: ", nng_msg_header(send_msg), nng_msg_header_len(send_msg));
		print_hex("msg body  : ", nng_msg_body(send_msg), nng_msg_len(send_msg));

		nng_aio_set_msg(client_work->aio, send_msg);
		nng_ctx_send(client_work->ctx, client_work->aio);

		clients = clients->next;
	}
//	while (clients && nng_aio_result(client_work->aio) == );
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

	//TODO nng_msg_set_cmd_type ?
	switch (pub_packet->fixed_header.packet_type) {
		case PUBLISH:
			/*fixed header*/
			nng_msg_header_append(msg, (uint8_t *) &pub_packet->fixed_header, 1);
			arr_len = put_var_integer(tmp, pub_packet->fixed_header.remain_len);
			nng_msg_header_append(msg, tmp, arr_len);
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

#if SUPPORT_MQTT5_0
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
#endif
			//payload
			if (pub_packet->payload_body.payload_len > 0) {
				nng_msg_append(msg, pub_packet->payload_body.payload, pub_packet->payload_body.payload_len);
			}
			break;

		case PUBREL:
		case PUBACK:
		case PUBREC:
		case PUBCOMP:
			/*fixed header*/
			nng_msg_header_append(msg, (uint8_t *) &pub_packet->fixed_header, 1);
			arr_len = put_var_integer(tmp, pub_packet->fixed_header.remain_len);
			nng_msg_header_append(msg, tmp, arr_len);

			/*variable header*/
			//identifier
			nng_msg_append_u16(msg, pub_packet->variable_header.pub_arrc.packet_identifier);

			//reason code
			if (pub_packet->fixed_header.remain_len > 2) {
				uint8_t reason_code = pub_packet->variable_header.pub_arrc.reason_code;
				nng_msg_append(msg, (uint8_t *) &reason_code, sizeof(reason_code));

#if SUPPORT_MQTT5_0
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
#endif

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
	int      pos       = 0;
	int      temp_pos  = 0;
	int      used_pos  = 0;
	int      len;
	char     *temp_str = 0;
	uint16_t temp_u16;

	uint8_t *msg_header = nng_msg_header(msg);
	uint8_t *msg_body   = nng_msg_body(msg);
	size_t  msg_len     = nng_msg_len(msg);

//	print_hex("nng_msg body: ", msg_body, msg_len);

	debug_msg("nng_msg len: %zu", msg_len);

//	memcpy((uint8_t *) &pub_packet->fixed_header, nng_msg_header(msg), 1);

	pub_packet->fixed_header = *(struct fixed_header *) nng_msg_header(msg);

	debug_msg("fixed header----------> ");
	debug_msg("cmd: %d, retain: %d, qos: %d, dup: %d\n",
	          pub_packet->fixed_header.packet_type,
	          pub_packet->fixed_header.retain,
	          pub_packet->fixed_header.qos,
	          pub_packet->fixed_header.dup);

	pub_packet->fixed_header.remain_len = nng_msg_remaining_len(msg);

	debug_msg("remaining length-------> %d\n", pub_packet->fixed_header.remain_len);

	if (pub_packet->fixed_header.remain_len <= msg_len) {

		debug_msg("variable header------->");
		switch (pub_packet->fixed_header.packet_type) {
			case PUBLISH:
				//variable header
				//topic length
				pub_packet->variable_header.publish.topic_name.str_body = (char *) nng_alloc(
						NNI_GET16(msg_body + pos, temp_u16) + 1);

				len = copy_utf8_str((uint8_t *) pub_packet->variable_header.publish.topic_name.str_body,
				                    msg_body + pos,
				                    &pos);

				debug_msg("get topic: %-*.*s, len: %d ,strlen(topic): %lu\n", len, len,
				          pub_packet->variable_header.publish.topic_name.str_body, len,
				          strlen(pub_packet->variable_header.publish.topic_name.str_body));

				if (len < 0
//Fixme 			||
//				    strchr(pub_packet->variable_header.publish.topic_name.str_body, '+') != NULL ||
//				    strchr(pub_packet->variable_header.publish.topic_name.str_body, '#') != NULL
						) {
					//protocol error
					debug_msg("protocol error in topic field: %s\n",
					          pub_packet->variable_header.publish.topic_name.str_body);
					return false;
				}

				pub_packet->variable_header.publish.topic_name.str_len = len;
//				debug_msg("topic len: %d\n", pub_packet->variable_header.publish.topic_name.str_len);

				if (pub_packet->fixed_header.qos > 0) { //extract packet_identifier while qos > 0
					NNI_GET16(msg_body + pos, pub_packet->variable_header.publish.packet_identifier);
					debug_msg("packet identifier: %d\n", pub_packet->variable_header.publish.packet_identifier);
					pos += 2;
				}

				used_pos = pos;


#if SUPPORT_MQTT5_0
				pub_packet->variable_header.publish.properties.len = get_var_integer(msg_body, &pos);

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
#endif


				//payload
//				*pub_packet->payload_body.payload = NULL;
//				*pub_packet->payload_body.payload = (uint8_t *) (msg_body + pos);


				pub_packet->payload_body.payload_len             = (uint32_t) (msg_len - (size_t) used_pos);
				pub_packet->payload_body.payload                 = (uint8_t *) nng_alloc(
						pub_packet->payload_body.payload_len);

				memcpy(pub_packet->payload_body.payload, (uint8_t *) (msg_body + pos),
				       pub_packet->payload_body.payload_len);
				debug_msg("payload len: %u\n", pub_packet->payload_body.payload_len);
				debug_msg("payload: %s\n", pub_packet->payload_body.payload);

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
#if SUPPORT_MQTT5_0
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
#endif
				break;

			default:
				break;
		}
		return true;

	}

	return false;
}

/**
 * byte array to hex string
 *
 * @param src
 * @param dest
 * @param src_len
 * @return
 */
static char *bytes_to_str(const unsigned char *src, char *dest, int src_len)
{
	int  i;
	char szTmp[3] = {0};

	for (i = 0; i < src_len; i++) {
		sprintf(szTmp, "%02X", (unsigned char) src[i]);
		memcpy(dest + i * 2, szTmp, 2);
	}
	return dest;
}

static void print_hex(const char *prefix, const unsigned char *src, int src_len)
{
	if (src_len > 0) {
		char *dest = (char *) nng_alloc(src_len * 2 + 1);

		if (dest == NULL) {
			debug_msg("alloc fail!");
			return;
		}
		dest = bytes_to_str(src, dest, src_len);

		debug_msg("%s%s\n", prefix, dest);

		nng_free(dest, src_len * 2 + 1);
	}
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
