#include <nng/nng.h>
#include <nng/protocol/mqtt/mqtt_parser.h>
#include <nng/protocol/mqtt/mqtt.h>
#include "../../src/include/nng_debug.h"

#include "nanolib.h"
#include "include/subscribe_handle.h"

uint8_t subscribe_handle(nng_msg * msg){
	// handle subscribe fixed header
	uint8_t *  header_ptr;
	uint8_t reason_code;
	header_ptr = nng_msg_header_ptr(msg);
	if((header_ptr[0] & 0xF0) != CMD_SUBSCRIBE){
		return SUCCESS;
	}

	struct packet_subscribe sub_pkt;

	if((reason_code = decode_sub_message(msg, &sub_pkt)) != SUCCESS){
		return reason_code;
	}
	if((reason_code = encode_suback_message(msg, &sub_pkt)) != SUCCESS){
		return reason_code;
	}else {
		// TODO send the msg
	}
	sub_ctx_handle(msg, &sub_pkt);
	debug_msg("END OF SUBSCRIBE Handle. ");
	return SUCCESS;
}

uint8_t decode_sub_message(nng_msg * msg, packet_subscribe * sub_pkt){
	uint8_t *  variable_ptr;
	uint8_t *  payload_ptr;
	int vpos = 0; // pos in variable
	int bpos = 0; // pos in payload

	int        len_of_varint = 0, len_of_property = 0, len_of_properties = 0;
	int        len_of_str, len_of_topic;
	size_t     remaining_len = nng_msg_remaining_len(msg);

	bool version_v5 = false; // v3.1.1/v5
	uint8_t property_id;

	topic_node * topic_node_t, * _topic_node;

	// handle variable header
	variable_ptr = nng_msg_variable_ptr(msg);

	NNI_GET16(variable_ptr + vpos, sub_pkt->packet_id);
	vpos += 2;

	// Only Mqtt_v5 include property. 
	if(version_v5){
		// length of property in varibale
		len_of_properties = get_var_integer(variable_ptr+vpos, &len_of_varint);
		vpos += len_of_varint;

		// parse property in variable
		if(len_of_properties > 0){
			while(1){
				property_id = variable_ptr[vpos++];
				switch(property_id){
					case SUBSCRIPTION_IDENTIFIER:
						sub_pkt->sub_id.varint = get_var_integer(variable_ptr+vpos, &len_of_varint);
						vpos += len_of_varint;
						break;
					case USER_PROPERTY:
						// key
						len_of_str = get_utf8_str(sub_pkt->user_property.strpair.str_key, variable_ptr, &vpos);
						sub_pkt->user_property.strpair.len_key = len_of_str;
						// vpos += len_of_str;

						// value
						len_of_str = get_utf8_str(sub_pkt->user_property.strpair.str_value, variable_ptr, &vpos);
						sub_pkt->user_property.strpair.str_value = len_of_str;
						// vpos += len_of_str;
						break;
					default:
						// avoid error
						if(vpos > remaining_len){
							debug_msg("ERROR_IN_LEN_VPOS");
						}
				}
			}
		}
	}

	debug_msg("VARIABLE: %x %x %x %x. ", variable_ptr[0], variable_ptr[1], variable_ptr[2], variable_ptr[3]);

    // handle payload
	payload_ptr = nng_msg_payload_ptr(msg);

	debug_msg("PAYLOAD:  %x %x %x %x. ", payload_ptr[0], payload_ptr[1], payload_ptr[2], payload_ptr[3]);

	topic_node_t = nng_alloc(sizeof(topic_node));
	sub_pkt->node = topic_node_t;
	topic_node_t->next = NULL;

	while(1){
		debug_msg("originData: %x %x", payload_ptr[bpos], payload_ptr[bpos+1]);
		topic_with_option * topic_option = nng_alloc(sizeof(topic_with_option));
		topic_node_t->it = topic_option;
		_topic_node = topic_node_t;

		len_of_topic = get_utf8_str(topic_option->topic_filter.str_body, payload_ptr, &bpos); // len of topic filter
		topic_option->topic_filter.len = len_of_topic;

		debug_msg("Length of topic: %d topic_node: %s. ", len_of_topic, topic_option->topic_filter.str_body);

		memcpy(topic_option, payload_ptr+bpos, 1);

		debug_msg("bpos:%d remaining_len:%d vpos:%d. ", bpos, remaining_len, vpos);
		if(++bpos < remaining_len - vpos){
			topic_node_t = nng_alloc(sizeof(topic_node));
			topic_node_t->next = NULL;
			_topic_node->next = topic_node_t;
		}else{
			break;
		}
	}
	return SUCCESS;
}

uint8_t encode_suback_message(nng_msg * msg, packet_subscribe * sub_pkt){
	debug_msg("generate the suback.......");
	bool version_v5 = false;
	nng_msg_clear(msg);

	uint8_t packet_id[2];
	uint8_t reason_code, cmd;
	uint32_t remaining_len;
	uint8_t varint[4];
	int len_of_varint;
	topic_node * node;

	// handle variable header first
	NNI_PUT16(packet_id, sub_pkt->packet_id);
	debug_msg("packetid: %x %x", packet_id[0], packet_id[1]);
	nng_msg_append(msg, packet_id, 2);
	if(version_v5){ // add property in variable
	}

	// handle payload header
	node = sub_pkt->node;
	while(node){
		if(version_v5){
		}else{
			reason_code = node->it->qos; //if no error happened
			// MQTT_v3: 0x00-qos0  0x01-qos1  0x02-qos2  0x80-fail
			nng_msg_append(msg, (uint8_t *) &reason_code, 1);
		}
		node = node->next;
		debug_msg("loading reason_code...\"%x\"", reason_code);
	}
	// handle fixed header
	debug_msg("handling the fixed header.....");
	cmd = CMD_SUBACK;
	nng_msg_header_append(msg, (uint8_t *) &cmd, 1);

	remaining_len = (uint32_t)nng_msg_len(msg);
	len_of_varint = put_var_integer(varint, remaining_len);
	nng_msg_header_append(msg, varint, len_of_varint);
	debug_msg("remain:%d varint:%d %d %d %d len:%d", remaining_len, varint[0], varint[1], varint[2], varint[3], len_of_varint);
	return SUCCESS;
}

void sub_ctx_handle(nng_msg * msg, packet_subscribe * sub_pkt){
	// generate ctx for each topic
	debug_msg("Generate ctx for echo topic");
	bool version_v5 = false;
	topic_node * topic_node_t = sub_pkt->node;
	struct ctx_sub * ctx_sub;

	while(topic_node_t){
		ctx_sub = nng_alloc(sizeof(ctx_sub));
		if(version_v5){
			ctx_sub->user_property.strpair.str_key = sub_pkt->user_property.strpair.str_key;
			ctx_sub->user_property.strpair.len_key = sub_pkt->user_property.strpair.len_key;
			ctx_sub->user_property.strpair.str_value = sub_pkt->user_property.strpair.str_value;
			ctx_sub->user_property.strpair.len_value = sub_pkt->user_property.strpair.len_value;
			// if(){} length should be limited
		}
		ctx_sub->topic_option = topic_node_t->it;
		topic_node_t = topic_node_t->next;
		debug_msg("Ctx generating.....");
		// insert ctx into tree, expose to app-layer to handle
		nng_free(ctx_sub, sizeof(ctx_sub));
	}
/*
	// insert ctx_sub into treeDB
	struct client * client = nng_alloc(sizeof(struct client));
	struct topic_and_node *tan = nng_alloc(sizeof(struct topic_and_node));
	char topic_str[6] = "a/b/t";
	search_node(work->db, topic_str, &tan);
	add_node(tan, client);

	// check treeDB
	debug_msg("start check dbtree");
	for(struct db_node * mnode = work->db->root ;mnode ;mnode = mnode->down){
		for(struct db_node * snode = mnode; snode; snode = snode->next){
			debug_msg("%s ", snode->topic);
		}
		debug_msg("----------");
	}
*/
	debug_msg("End of sub ctx handle. \n");
}

