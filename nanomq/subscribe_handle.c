#include <nng.h>
#include <mqtt_db.h>
#include <hash.h>
#include <protocol/mqtt/mqtt_parser.h>
#include <protocol/mqtt/mqtt.h>
#include "include/nanomq.h"
#include "include/subscribe_handle.h"

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
						sub_pkt->user_property.strpair.len_value = len_of_str;
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

	debug_msg("Remain_len: [%d] packet_id : [%d]", remaining_len, sub_pkt->packet_id);
	// handle payload
	payload_ptr = nng_msg_payload_ptr(msg);

	debug_msg("V:[%x %x %x %x] P:[%x %x %x %x].", variable_ptr[0], variable_ptr[1], variable_ptr[2], variable_ptr[3],
			payload_ptr[0], payload_ptr[1], payload_ptr[2], payload_ptr[3]);

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

		debug_msg("Bpos+Vpos: [%d] Remain_len:%d.", bpos+vpos, remaining_len);
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
	nng_msg_append(msg, packet_id, 2);
	if(version_v5){ // add property in variable
	}

	// handle payload
	node = sub_pkt->node;
	while(node){
		if(version_v5){
		}else{
			if(node->it->reason_code == 0x80){
				reason_code = 0x80;
			}else{
				reason_code = node->it->qos;
			}
			// MQTT_v3: 0x00-qos0  0x01-qos1  0x02-qos2  0x80-fail
			nng_msg_append(msg, (uint8_t *) &reason_code, 1);
		}
		node = node->next;
		debug_msg("reason_code: [%x]", reason_code);
	}
	// handle fixed header
	cmd = CMD_SUBACK;
	nng_msg_header_append(msg, (uint8_t *) &cmd, 1);

	remaining_len = (uint32_t)nng_msg_len(msg);
	len_of_varint = put_var_integer(varint, remaining_len);
	nng_msg_header_append(msg, varint, len_of_varint);
	debug_msg("remain: [%d] varint: [%d %d %d %d] len: [%d] packet_id: [%x %x]", remaining_len, varint[0], varint[1], varint[2], varint[3], len_of_varint, packet_id[0], packet_id[1]);
	return SUCCESS;
}

void sub_ctx_handle(emq_work * work){
	// generate ctx for each topic
	int count = 0;
	bool version_v5 = false;

	topic_node * topic_node_t = work->sub_pkt->node;
	char * topic_str;

	// insert ctx_sub into treeDB
	while(topic_node_t){
		struct topic_and_node *tan = nng_alloc(sizeof(struct topic_and_node));
		struct client * client = nng_alloc(sizeof(struct client));

		//setting client
		client->id = conn_param_get_clentid(nng_msg_get_conn_param(work->msg));
		client->ctxt = work;
		client->next = NULL;
		debug_msg("client id: [%s], ctxt: [%d], aio: [%p], pipe_id: [%d]\n",client->id, work->ctx.id, work->aio, work->pid.id);

		topic_str = (char *)nng_alloc(topic_node_t->it->topic_filter.len + 1);
		strncpy(topic_str, topic_node_t->it->topic_filter.str_body, topic_node_t->it->topic_filter.len);
		topic_str[topic_node_t->it->topic_filter.len] = '\0';
		debug_msg("topicLen:%d, Body:%s", topic_node_t->it->topic_filter.len, (char *)topic_str);

		char ** topic_queue = topic_parse(topic_str);
//		debug_msg("topic_queue: -%s -%s -%s -%s", *topic_queue, *(topic_queue+1), *(topic_queue+2), *(topic_queue+3));
		search_node(work->db, topic_queue, tan);
		debug_msg("finish SEARCH_NODE; tan->node->topic: %s", tan->node->topic);
		if(tan->topic){
			debug_msg("finish SEARCH_NODE; tan->topic: %s", (char *)(*tan->topic));
		}
		if(tan->topic){
			add_node(tan, client);
			add_topic(client->id, topic_str);
			struct topic_queue * q = get_topic(client->id);
			debug_msg("------CHECKHASHTABLE----clientid:%s---topic:%s", client->id, q->topic);
		}else{
			// TODO contain but not strcmp
			if(tan->node->sub_client==NULL || !check_client(tan->node, client->id)){
				add_topic(client->id, topic_str);
				struct topic_queue * q = get_topic(client->id);
				debug_msg("------CHECKHASHTABLE----clientid:%s---topic:%s", client->id, q->topic);
				add_client(tan, client);
			}else{
				work->sub_pkt->node->it->reason_code = 0x80;
			}
		}
//		search_node(work->db, topic_parse(topic_str), tan);
//		debug_msg("ENSURE CLIENTID: %s", tan->node->sub_client->id);
		nng_free(tan, sizeof(struct topic_and_node));
		nng_free(topic_str, topic_node_t->it->topic_filter.len+1);
		topic_node_t = topic_node_t->next;
		debug_msg("finish ADD_CLIENT");
	}

	// check treeDB
//	print_db_tree(work->db);
	debug_msg("End of sub ctx handle. \n");
}

void destroy_sub_ctx(void * ctxt){
	emq_work * work = ctxt;
	packet_subscribe * sub_pkt = work->sub_pkt;
	topic_node * topic_node_t = sub_pkt->node;
	topic_node * _topic_node;
	while(topic_node_t){
		nng_free(topic_node_t->it, sizeof(topic_with_option));

		_topic_node = topic_node_t->next;
		nng_free(topic_node_t, sizeof(topic_node));
		topic_node_t = _topic_node;
	}
}

