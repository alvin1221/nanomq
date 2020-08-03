#include "../../../include/nng/protocol/mqtt/mqtt.h"
#include "property_handle.h"

void unsubsrcibe_handle(nni_msg* msg){
	uint8_t *  header_ptr;
	uint8_t *  variable_ptr;
	uint8_t *  payload_ptr;

	uint16_t   packet_id;
	int        len_of_varint;
	size_t *   remaining_len;
	int vpos = 0; // pos of variable
	int bpos = 0; // pos of payload

	// handle unsubscribe fixed header
	header_ptr = nni_msg_header_ptr(msg);
	if((header_ptr[0] & 0xF0) != CMD_UNSUBSCRIBE){
		return;
	}
	len = nni_msg_len(msg);

	// handle variable header
	debug_msg("Handle the variable header of unsub. \n");
	variable_ptr = nni_msg_variable_ptr(msg);
	packet_id = (variable_ptr[vpos+1]<<8) + variable_ptr[vpos];
	vpos += 2;

	mqtt_property * prop = nni_alloc(sizeof(mqtt_property));
	property_list_init(prop);
	prop->len = bin_parse_varint(variable_ptr+vpos, &len_of_varint);
	vpos += len_of_varint;

	// parse property in variable
	if(prop->len > 0){
		int prop_pos = 0;
		while(1){
			uint32_t property_id = variable_ptr[vpos++];
			len_of_varint = property_list_insert(prop, property_id, variable_ptr+vpos+1);
			if(len_of_varint == 0){
				debug_msg("ERROR IN PACKETID. \n");
				property_list_free(prop);
			}
			prop_pos += (1+len_of_varint);
			vpos += (1+len_of_varint);
			if(prop_pos >= prop->len){
				break;
			}
		}
	}

    // binary handle
	debug_msg("Handle the binary of unsub. \n");
	payload_ptr = nni_msg_payload_ptr(msg);
	mqtt_payload_unsubscribe * payload_unsub = nni_alloc(sizeof(mqtt_payload_unsubscribe));

    mqtt_string_node * topic_node_t = nni_alloc(sizeof(mqtt_string_node)); //header
	payload_unsub->topic_filter = topic_node_t;
	payload_unsub->count = 0;

    mqtt_string_node * _topic_node;
    while(1){
        int topic_len = (payload_ptr[bpos+1]<<8)+payload_ptr[bpos]; // len of topic filter
        bpos += 2;
		mqtt_string * topic_context = nni_alloc(sizeof(mqtt_string));
		topic_node_t->it = topic_context;
		_topic_node = topic_node_t;

		topic_context->len = topic_len;
		memcpy(topic_context->str, payload_ptr+bpos, topic_len);
		payload_unsub->count ++;

		bpos += topic_len;

		remaining_len = nni_msg_remaining_len(msg);
		if(bpos < *remaining_len - vpos){
			topic_node_t = nni_alloc(sizeof(mqtt_string_node));
			_topic_node->next = topic_node_t;
		}
	}

	// action for each topic
	uint8_t unsuback_reason_codes[payload_sub->count];
	int upos = 0;
	while(1){
		topic_node_t = payload_unsub->topic_filter;
		if(topic_node_t == NULL){
			break;
		}

		// delete the client id according the topic_filter from the tree

		unsuback_reason_codes[upos++] = 0x00; // success
	}
	nni_free(payload_unsub, sizeof(mqtt_payload_unsubscribe));
}

