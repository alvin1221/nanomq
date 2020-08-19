#include <nng/nng.h>

uint8_t decode_unsub_message(nng_msg * msg, packet_unsubscribe * unsub_pkt){
	uint8_t * variable_ptr;
	uint8_t * payload_ptr;
	int vpos = 0; // pos in variable
	int bpos = 0; // pos in payload

	int        len_of_varint = 0, len_of_property = 0, len_of_properties = 0;
	int        len_of_str, len_of_topic;
	size_t     remaining_len = nng_msg_remaining_len(msg);

	bool version_v5 = false; // v3.1.1/v5
	uint8_t property_id;

	topic_node * topic_node_t, * _topic_node;

	// handle varibale header
	variable_ptr = nng_msg_variable_ptr(msg);

	NNI_GET16(variable_ptr, unsub_pkt->packet_id);
	vpos += 2;

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
uint8_t encode_unsuback_message(nng_msg *, packet_unsubscribe *);
void unsub_ctx_handle(emq_work *);

