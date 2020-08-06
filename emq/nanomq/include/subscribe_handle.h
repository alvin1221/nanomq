#include "../../../include/nng/protocol/mqtt/mqtt.h"
#include "property_handle.h"

struct ctx_sub {
	uint32_t	id;
	struct mqtt_property * variable_property;
	struct topic_with_option * option;

// connect info
//	struct ctx_connect * ctx_con;
};
typedef struct ctx_sub ctx_sub;

uint8_t decode_sub_message(nni_msg * msg, packet_subscribe * sub_pkt);
uint8_t encode_suback_message(nng_msg * msg, packet_subscribe * sub_pkt);
void sub_ctx_handle(nng_msg * msg, packet_subscribe * sub_pkt);

uint8_t subscribe_handle(nni_msg * msg){
	// handle subscribe fixed header
	uint8_t *  header_ptr;
	uint8_t reason_code;
	header_ptr = nni_msg_header_ptr(msg);
	if((header_ptr[0] & 0xF0) != CMD_SUBSCRIBE){
		return SUCCESS;
	}

	struct packet_subscribe sub_pkt;

	if(reason_code = decode_sub_message(msg, &sub_pkt) != SUCCESS){
		return reason_code;
	}
	if(reason_code = encode_suback_message(msg, &sub_pkt) != SUCCESS){
		return reason_code;
	}else {
		// TODO send the msg
	}
	sub_ctx_handle(msg, &sub_pkt);
	debug_msg("END OF SUBSCRIBE Handle. ");
	return SUCCESS;
}

uint8_t decode_sub_message(nni_msg * msg, packet_subscribe * sub_pkt){
	uint8_t *  variable_ptr;
	uint8_t *  payload_ptr;

	uint16_t   packet_id;
	int        len_of_varint;
	size_t     remaining_len;
	int vpos = 0; // pos in variable
	int bpos = 0; // pos in payload

	bool version_v5 = false; // v3.1.1/v5

	debug_msg("Handle the SUB request............. \n");
	// handle variable header
	variable_ptr = nni_msg_variable_ptr(msg);

	NNI_GET16(variable_ptr + vpos, sub_pkt->packet_id);
	debug_msg("SUB packet id : %d  originData: %x%x. ", packet_id, variable_ptr[vpos+1], variable_ptr[vpos]);
	vpos += 2;

	mqtt_property * prop;
	sub_pkt->property = prop;
	// Only Mqtt_v5 include property. 
	if(version_v5){
		prop = nni_alloc(sizeof(mqtt_property));
		property_list_init(prop);
		prop->len = bin_parse_varint(variable_ptr+vpos, &len_of_varint);
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
			nni_free(prop, sizeof(mqtt_property));
		}
	}

	debug_msg("VARIABLE: %x %x %x %x. ", variable_ptr[0], variable_ptr[1], variable_ptr[2], variable_ptr[3]);
	debug_msg("PAYLOAD:  %x %x %x %x. ", payload_ptr[0], payload_ptr[1], payload_ptr[2], payload_ptr[3]);

    // handle payload
	debug_msg("Handle the payload IN SUB. \n");
	payload_ptr = nni_msg_payload_ptr(msg);

	topic_node * topic_node_t = nni_alloc(sizeof(topic_node));
	sub_pkt->node = topic_node_t;
	topic_node * _topic_node;

	while(1){
		int topic_len;
		NNI_GET16(payload_ptr + bpos, topic_len); // len of topic filter
		debug_msg("originData: %x %x", payload_ptr[bpos], payload_ptr[bpos+1]);
		debug_msg("Length of topic: %d . ", topic_len);
		bpos += 2;
		topic_with_option * topic_context = nni_alloc(sizeof(topic_with_option));
		topic_node_t->it = topic_context;
		_topic_node = topic_node_t;

		mqtt_string * str = nni_alloc(sizeof(mqtt_string));
		NNI_GET16(payload_ptr + bpos, str->len);
		bpos += 2;
		if(str->len + 2 != topic_len){
			return TOPIC_FILTER_INVALID;
		}

		memcpy(str->str, payload_ptr+bpos, str->len);
		bpos += str->len;

		debug_msg("Length of topic: %d topic_node: %s. ", topic_len, str->str);

		int tmp_qos = 0x03 & payload_ptr[bpos];
		if(tmp_qos > 2){
			debug_msg("ERROR IN QOS OF SUBSCRIBE. \n");
		}

		topic_context->no_local = (0x04 & payload_ptr[bpos]) == 1;
		topic_context->retain = (0x08 & payload_ptr[bpos]) == 1;
		topic_context->retain_option = (0x30 & payload_ptr[bpos]);

		remaining_len = nni_msg_remaining_len(msg);
		if(++bpos < remaining_len - vpos){
			topic_node_t = nni_alloc(sizeof(topic_node));
			_topic_node->next = topic_node_t;
		}
	}
	return SUCCESS;
}

uint8_t encode_suback_message(nng_msg * msg, packet_subscribe * sub_pkt){
	// handle variable header first
	// packet id
	bool version_v5 = false;
	nng_msg_append(msg, (uint8_t *) &(sub_pkt->packet_id), 2);

	// handle payload header
	topic_node * node = sub_pkt->node;
	while(node){
		if(version_v5){

		}else{
			uint8_t reason_code = 0x00; //qos0
			// 0x01--qos1     0x02--qos2    0x80--fail
			nng_msg_append(msg, (uint8_t *) &(sub_pkt->packet_id), 1);
		}
		node = node->next;
	}
	// handle fixed header
	uint8_t cmd = CMD_SUBACK;
	nni_msg_header_append(msg, (uint8_t *) &cmd, 1);

	uint32_t remaining_len = (uint32_t)nni_msg_len(msg);
	uint8_t varint[4];
	int len_of_varint = put_var_integer(varint, remaining_len);
	nni_msg_header_append(msg, varint, len_of_varint);
	return SUCCESS;
}

void sub_ctx_handle(nng_msg * msg, packet_subscribe * sub_pkt){
	// generate ctx for each topic
	debug_msg("GENERATE CTX. ");
	bool version_v5 = false;
	topic_node * topic_node_t;
	topic_node_t = sub_pkt->node;
	while(topic_node_t){
		struct Ctx_sub * ctx_sub = nni_alloc(sizeof(Ctx_sub));
		if(version_v5){
			ctx_sub->variable_property = sub_pkt->property;
		}
		ctx_sub->topic_with_option = topic_node_t->it;
		topic_node_t = topic_node_t->next;
		// insert ctx into tree,....
		//
		nni_free(ctx_sub, sizeof(Ctx_sub));
	}
	nni_free(sub_pkt, sizeof(packet_subscribe));
}
