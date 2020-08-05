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

void subscribe_handle(nni_msg * msg){
	uint8_t *  header_ptr;
	uint8_t *  variable_ptr;
	uint8_t *  payload_ptr;

	uint16_t   packet_id;
	int        len_of_varint;
	size_t *   remaining_len;
	int vpos = 0; // pos in variable
	int bpos = 0; // pos in payload

	// handle subscribe fixed header
	header_ptr = nni_msg_header_ptr(msg);
	if((header_ptr[0] & 0xF0) != CMD_SUBSCRIBE){
		return;
	}
	size_t len = nni_msg_len(msg);

	debug_msg("Handle the SUB request............. \n");
	// handle variable header
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
	}

    // handle payload
	debug_msg("Handle the payload IN SUB. \n");
	payload_ptr = nni_msg_payload_ptr(msg);

	struct mqtt_payload_subscribe * payload_sub = nni_alloc(sizeof(mqtt_payload_subscribe));
	topic_node * topic_node_t = nni_alloc(sizeof(topic_node)); //header
	payload_sub->node = topic_node_t;
	topic_node * _topic_node;

	while(1){
		int topic_len = (payload_ptr[bpos+1]<<8)+payload_ptr[bpos]; // len of topic filter
		bpos += 2;
		topic_with_option * topic_context = nni_alloc(sizeof(topic_with_option));
		topic_node_t->it = topic_context;
		_topic_node = topic_node_t;

		mqtt_string * str = nni_alloc(sizeof(mqtt_string));
		str->len = topic_len;
		memcpy(str->str, payload_ptr+bpos, topic_len);
		bpos += topic_len;

		int tmp_qos = 0x03 & payload_ptr[bpos];
		if(tmp_qos > 2){
			debug_msg("ERROR IN QOS OF SUBSCRIBE. \n");
		}

		topic_context->no_local = (0x04 & payload_ptr[bpos]) == 1;
		topic_context->retain = (0x08 & payload_ptr[bpos]) == 1;
		topic_context->retain_option = (0x30 & payload_ptr[bpos]);

		remaining_len = nni_msg_remaining_len(msg);
		if(++bpos < *remaining_len - vpos){
			topic_node_t = nni_alloc(sizeof(topic_node));
			_topic_node->next = topic_node_t;
		}
	}

	// generate ctx for each topic
	topic_node_t = payload_sub->node;
	while(topic_node_t){
		struct Ctx_sub * ctx_sub = nni_alloc(sizeof(Ctx_sub));
		ctx_sub->variable_property = prop;
		ctx_sub->option = topic_node_t->it;
		topic_node_t = topic_node_t->next;
		// insert ctx into tree,....
		//
		nni_free(ctx_sub, sizeof(Ctx_sub));
	}
	nni_free(payload_sub, sizeof(mqtt_payload_subscribe));
}

