#include <nng/nng.h>
#include <nng/protocol/mqtt/mqtt_parser.h>
#include <nng/protocol/mqtt/mqtt.h>
#include "include/nanomq.h"
#include "include/unsubscribe_handle.h"
#include <nanolib/mqtt_db.h>

uint8_t decode_unsub_message(nng_msg * msg, packet_unsubscribe * unsub_pkt){
	uint8_t * variable_ptr;
	uint8_t * payload_ptr;
	int vpos = 0; // pos in variable
	int bpos = 0; // pos in payload

	int        len_of_varint = 0, len_of_property = 0, len_of_properties = 0;
	int        len_of_str, len_of_topic;
	size_t     remaining_len = nng_msg_remaining_len(msg);
	debug_msg("remainlen : %d", remaining_len);

	bool version_v5 = false; // v3.1.1/v5
	uint8_t property_id;

	topic_node * topic_node_t, * _topic_node;

	// handle varibale header
	variable_ptr = nng_msg_variable_ptr(msg);
	NNI_GET16(variable_ptr, unsub_pkt->packet_id);
	vpos += 2;
	debug_msg("packetid : %d", unsub_pkt->packet_id);

	// Mqtt_v5 include property
	if(version_v5){
		// length of property in variable
		len_of_properties = get_var_integer(variable_ptr, &len_of_varint);
		vpos += len_of_varint;

		if(len_of_properties > 0){
			while(1){
				property_id = variable_ptr[vpos];
				switch(property_id){
					case USER_PROPERTY:
						// key
						len_of_str = get_utf8_str(&(unsub_pkt->user_property.strpair.str_key), variable_ptr, &vpos);
						unsub_pkt->user_property.strpair.len_key = len_of_str;
						// value
						len_of_str = get_utf8_str(&(unsub_pkt->user_property.strpair.str_value), variable_ptr, &vpos);
						unsub_pkt->user_property.strpair.len_value = len_of_str;
					default:
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
	unsub_pkt->node = topic_node_t;
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

		debug_msg("bpos:%d remaining_len:%d vpos:%d. ", bpos, remaining_len, vpos);
		if(bpos < remaining_len - vpos){
			topic_node_t = nng_alloc(sizeof(topic_node));
			topic_node_t->next = NULL;
			_topic_node->next = topic_node_t;
		}else{
			break;
		}
	}
	return SUCCESS;
}

uint8_t encode_unsuback_message(nng_msg * msg, packet_unsubscribe * unsub_pkt){
	debug_msg("generate the unsuback.......");
	bool version_v5 = false;
	nng_msg_clear(msg);

	uint8_t packet_id[2];
	uint8_t reason_code, cmd;
	uint32_t remaining_len;
	uint8_t varint[4];
	int len_of_varint;
	topic_node * node;

	// handle variable header first
	NNI_PUT16(packet_id, unsub_pkt->packet_id);
	debug_msg("packetid: %x %x", packet_id[0], packet_id[1]);
	nng_msg_append(msg, packet_id, 2);
	if(version_v5){ // add property in variable
	}

	// handle payload
	// no payload in mqtt_v3
	if(version_v5){
		node = unsub_pkt->node;
		while(node){
			reason_code = unsub_pkt->node->it->reason_code;
			nng_msg_append(msg, (uint8_t *) &reason_code, 1);
		}
		node = node->next;
		debug_msg("loading reason_code...\"%x\"", reason_code);
	}

	// handle fixed header
	debug_msg("handling the fixed header.....");
	cmd = CMD_UNSUBACK;
	nng_msg_header_append(msg, (uint8_t *) &cmd, 1);

	remaining_len = (uint32_t)nng_msg_len(msg);
	len_of_varint = put_var_integer(varint, remaining_len);
	nng_msg_header_append(msg, varint, len_of_varint);
	debug_msg("remain:%d varint:%d %d %d %d len:%d", remaining_len, varint[0], varint[1], varint[2], varint[3], len_of_varint);

	return SUCCESS;
}

void unsub_ctx_handle(emq_work * work){
	// generate ctx for each topic
	bool version_v5 = false;
	debug_msg("Generate ctx for each topic");

	topic_node * topic_node_t = work->unsub_pkt->node;
	char * topic_str;
	char * clientid;

	// delete ctx_unsub into treeDB
	while(topic_node_t){
		struct topic_and_node *tan = nng_alloc(sizeof(struct topic_and_node));
		clientid = conn_param_get_clentid(nng_msg_get_conn_param(work->msg));

		// parse topic string
		topic_str = (char *)nng_alloc(topic_node_t->it->topic_filter.len + 1);
		strncpy(topic_str, topic_node_t->it->topic_filter.str_body, topic_node_t->it->topic_filter.len);
		topic_str[topic_node_t->it->topic_filter.len] = '\0';

		char ** topic_queue = topic_parse(topic_str);
		search_node(work->db, topic_queue, tan);

		if(tan->topic == NULL){ // find the topic
			del_client(tan, clientid);
			topic_node_t->it->reason_code = 0x00;
		}else{ // not find the topic
			topic_node_t->it->reason_code = 0x11;
		}

		// free local varibale
		nng_free(topic_str, topic_node_t->it->topic_filter.len+1);
		nng_free(tan, sizeof(struct topic_and_node));

		// free in treedb
		// TODO free client, current free be included in delete_node()
		// nng_free();

		topic_node_t = topic_node_t->next;
		debug_msg("finish ADD_CLIENT");
	}

	// check treeDB
	print_db_tree(work->db);

	debug_msg("End of sub ctx handle. \n");
}

