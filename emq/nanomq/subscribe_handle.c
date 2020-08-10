#include <nng/nng.h>
#include <nng/protocol/mqtt/mqtt_parser.h>
#include <nng/protocol/mqtt/mqtt.h>

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

	int        len_of_varint = 0;
	size_t     remaining_len;
	int vpos = 0; // pos in variable
	int bpos = 0; // pos in payload

	bool version_v5 = false; // v3.1.1/v5

	// handle variable header
	variable_ptr = nng_msg_variable_ptr(msg);

	NNI_GET16(variable_ptr + vpos, sub_pkt->packet_id);
	vpos += 2;

	mqtt_property * prop;
	sub_pkt->property = prop;
	// Only Mqtt_v5 include property. 
	if(version_v5){
		prop = nng_alloc(sizeof(mqtt_property));
		property_list_init(prop);
		prop->len = get_var_integer(variable_ptr+vpos, &len_of_varint);
		vpos += len_of_varint;

		// parse property in variable
		if(prop->len > 0){
			int prop_pos = 0;
			while(1){
				uint8_t property_id = variable_ptr[vpos++];
				len_of_varint = property_list_insert(prop, property_id, variable_ptr+vpos);
				if(len_of_varint == 0){
					debug_msg("ERROR IN PACKETID. \n");
					property_list_free(prop);
				}
				prop_pos += (1+len_of_varint);
				vpos += len_of_varint;
				if(prop_pos >= prop->len){
					break;
				}
			}
		}else{
			nng_free(prop, sizeof(mqtt_property));
		}
	}

	debug_msg("VARIABLE: %x %x %x %x. ", variable_ptr[0], variable_ptr[1], variable_ptr[2], variable_ptr[3]);

    // handle payload
	payload_ptr = nng_msg_payload_ptr(msg);
	
	debug_msg("PAYLOAD:  %x %x %x %x. ", payload_ptr[0], payload_ptr[1], payload_ptr[2], payload_ptr[3]);

	topic_node * topic_node_t = nni_alloc(sizeof(topic_node));
	sub_pkt->node = topic_node_t;
	topic_node_t->next = NULL;
	topic_node * _topic_node;

	while(1){
		int topic_len;
		NNI_GET16(payload_ptr + bpos, topic_len); // len of topic filter
		debug_msg("originData: %x %x", payload_ptr[bpos], payload_ptr[bpos+1]);
		bpos += 2;
		topic_with_option * topic_context = nni_alloc(sizeof(topic_with_option));
		topic_node_t->it = topic_context;
		_topic_node = topic_node_t;

		mqtt_string * str = nni_alloc(sizeof(mqtt_string));
		str->len = topic_len;
		str->str = nni_alloc(topic_len+1);
		memcpy(str->str, payload_ptr+bpos, str->len);
		str->str[topic_len] = '\0';
		bpos += topic_len;

		debug_msg("Length of topic: %d topic_node: %s. ", topic_len, str->str);

		int tmp_qos = 0x03 & payload_ptr[bpos];
		if(tmp_qos > 2){
			debug_msg("ERROR IN QOS OF SUBSCRIBE. \n");
		}

		topic_context->topic_filter = str;
		topic_context->no_local = (0x04 & payload_ptr[bpos]) == 1;
		topic_context->retain = (0x08 & payload_ptr[bpos]) == 1;
		topic_context->retain_option = (0x30 & payload_ptr[bpos]);

		remaining_len = nng_msg_remaining_len(msg);
		debug_msg("bpos:%d remaining_len:%d vpos:%d. ", bpos, remaining_len, vpos);
		if(++bpos < remaining_len - vpos){
			topic_node_t = nni_alloc(sizeof(topic_node));
			topic_node_t->next = NULL;
			_topic_node->next = topic_node_t;
		}else{
			break;
		}
	}
	return SUCCESS;
}

uint8_t encode_suback_message(nng_msg * msg, packet_subscribe * sub_pkt){
	// handle variable header first
	debug_msg("generate the ack.......");
	bool version_v5 = false;
	nng_msg_clear(msg);
	// packet id
	uint8_t packet_id[2];
	NNI_PUT16(packet_id, sub_pkt->packet_id);
	debug_msg("packetid: %x %x", packet_id[0], packet_id[1]);
	nng_msg_append(msg, packet_id, 2);

	// handle payload header
	topic_node * node = sub_pkt->node;
	while(node){
		if(version_v5){

		}else{
			uint8_t reason_code = 0x00; //qos0
			// 0x01--qos1     0x02--qos2    0x80--fail
			nng_msg_append(msg, (uint8_t *) &reason_code, 1);
		}
		node = node->next;
		debug_msg("ERRORRRR.....");
	}
	// handle fixed header
	uint8_t cmd = CMD_SUBACK;
	nng_msg_header_append(msg, (uint8_t *) &cmd, 1);

	debug_msg("ERRORRRRhererere.....");
	uint32_t remaining_len = (uint32_t)nng_msg_len(msg);
	uint8_t varint[4];
	int len_of_varint = put_var_integer(varint, remaining_len);
	debug_msg("remain:%d varint:%d %d %d %d len:%d", remaining_len, varint[0], varint[1], varint[2], varint[3], len_of_varint);
	nng_msg_header_append(msg, varint, len_of_varint);
	return SUCCESS;
}

void sub_ctx_handle(nng_msg * msg, packet_subscribe * sub_pkt){
	// generate ctx for each topic
	debug_msg("Generate ctx for echo topic");
	bool version_v5 = false;
	debug_msg("ERROR???");
	topic_node * topic_node_t = sub_pkt->node;

	while(topic_node_t){
		struct Ctx_sub * ctx_sub = nni_alloc(sizeof(struct Ctx_sub));
		if(version_v5){
			ctx_sub->variable_property = sub_pkt->property;
		}
		debug_msg("ERROR???");
		ctx_sub->topic_with_option = topic_node_t->it;
		debug_msg("ERROR???");
		topic_node_t = topic_node_t->next;
		// insert ctx into tree,....
		//
		nni_free(ctx_sub, sizeof(Ctx_sub));
	}
	debug_msg("End of sub ctx handle");
}

