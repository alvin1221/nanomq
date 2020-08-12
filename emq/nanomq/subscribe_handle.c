#include <nng/nng.h>
#include <nng/protocol/mqtt/mqtt_parser.h>
#include <nng/protocol/mqtt/mqtt.h>
#include "include/nanomq.h"
#include "include/subscribe_handle.h"
#include "include/mqtt_db.h"

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
	}
//	sub_ctx_handle(msg, &sub_pkt);
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
						len_of_str = get_utf8_str(&(sub_pkt->user_property.strpair.str_key), variable_ptr, &vpos);
						sub_pkt->user_property.strpair.len_key = len_of_str;
						// vpos += len_of_str;

						// value
						len_of_str = get_utf8_str(&(sub_pkt->user_property.strpair.str_value), variable_ptr, &vpos);
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
		topic_with_option * topic_option = nng_alloc(sizeof(topic_with_option));
		topic_node_t->it = topic_option;
		_topic_node = topic_node_t;

		len_of_topic = get_utf8_str(&(topic_option->topic_filter.str_body), payload_ptr, &bpos); // len of topic filter
		if(len_of_topic != -1){
			topic_option->topic_filter.len = len_of_topic;
		}else {
			debug_msg("NOT utf-8 format string. ");
		}

		debug_msg("Length of topic: %d topic_node: %x %x. ", len_of_topic, (uint8_t)(topic_option->topic_filter.str_body[0]), (uint8_t)(topic_option->topic_filter.str_body[1]));

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

void sub_ctx_handle(emq_work * work){
	// generate ctx for each topic
	int count = 0;
	bool version_v5 = false;
	debug_msg("Generate ctx for each topic");

	topic_node * topic_node_t = work->sub_pkt->node;

	// insert ctx_sub into treeDB
	while(topic_node_t){
		struct topic_and_node *tan = nng_alloc(sizeof(struct topic_and_node));
		struct client * client = nng_alloc(sizeof(struct client));
		client->id = conn_param_get_clentid(nng_msg_get_conn_param(work->msg));
		client->ctxt = work;

		debug_msg("Ctx generating... Len:%d, Body:%s", topic_node_t->it->topic_filter.len, (char *)topic_node_t->it->topic_filter.str_body);
		search_node(work->db, topic_node_t->it->topic_filter.str_body, &tan);
		add_node(tan, client);
		topic_node_t = topic_node_t->next;
	}

	// check treeDB
	debug_msg("---check dbtree---");

	for(struct db_node * mnode = work->db->root ;mnode ;mnode = mnode->down){
		for(struct db_node * snode = mnode; snode; snode = snode->next){
			debug_msg("%d: %s ", count, snode->topic);
		}
		debug_msg("----------");
		if(++count > 1){
			break;
		}
	}

	debug_msg("End of sub ctx handle. \n");
}

