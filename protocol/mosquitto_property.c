#include "mosquitto_base.h"
#include "mqtt_protocol.h"

int property__process_will(struct mosquitto *context, struct mosquitto_message_all *msg, mosquitto_property **props)
{
	mosquitto_property *p, *p_prev;
	mosquitto_property *msg_properties, *msg_properties_last;

	p = *props;
	p_prev = NULL;
	msg_properties = NULL;
	msg_properties_last = NULL;
	while(p){
		switch(p->identifier){
			case MQTT_PROP_CONTENT_TYPE:
			case MQTT_PROP_CORRELATION_DATA:
			case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
			case MQTT_PROP_RESPONSE_TOPIC:
			case MQTT_PROP_USER_PROPERTY:
				if(msg_properties){
					msg_properties_last->next = p;
					msg_properties_last = p;
				}else{
					msg_properties = p;
					msg_properties_last = p;
				}
				if(p_prev){
					p_prev->next = p->next;
					p = p_prev->next;
				}else{
					*props = p->next;
					p = *props;
				}
				msg_properties_last->next = NULL;
				break;

			case MQTT_PROP_WILL_DELAY_INTERVAL:
				context->will_delay_interval = p->value.i32;
				p_prev = p;
				p = p->next;
				break;

			case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
				msg->expiry_interval = p->value.i32;
				p_prev = p;
				p = p->next;
				break;

			default:
				return MOSQ_ERR_PROTOCOL;
				break;
		}
	}

	msg->properties = msg_properties;
	return MOSQ_ERR_SUCCESS;
}

const mosquitto_property *property__get_property(const mosquitto_property *proplist, int identifier, bool skip_first)
{
	const mosquitto_property *p;
	bool is_first = true;

	p = proplist;

	while(p){
		if(p->identifier == identifier){
			if(!is_first || !skip_first){
				return p;
			}
			is_first = false;
		}
		p = p->next;
	}
	return NULL;
}


const mosquitto_property *mosquitto_property_read_binary(const mosquitto_property *proplist, int identifier, void **value, uint16_t *len, bool skip_first)
{
	const mosquitto_property *p;
	if(!proplist || (value && !len) || (!value && len)) return NULL;

	p = property__get_property(proplist, identifier, skip_first);
	if(!p) return NULL;
	if(p->identifier != MQTT_PROP_CORRELATION_DATA
			&& p->identifier != MQTT_PROP_AUTHENTICATION_DATA){

		return NULL;
	}

	if(value){
		*len = p->value.bin.len;
		*value = malloc(*len);
		if(!(*value)) return NULL;

		memcpy(*value, p->value.bin.v, *len);
	}

	return p;
}


const mosquitto_property *mosquitto_property_read_string(const mosquitto_property *proplist, int identifier, char **value, bool skip_first)
{
	const mosquitto_property *p;
	if(!proplist) return NULL;

	p = property__get_property(proplist, identifier, skip_first);
	if(!p) return NULL;
	if(p->identifier != MQTT_PROP_CONTENT_TYPE
			&& p->identifier != MQTT_PROP_RESPONSE_TOPIC
			&& p->identifier != MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER
			&& p->identifier != MQTT_PROP_AUTHENTICATION_METHOD
			&& p->identifier != MQTT_PROP_RESPONSE_INFORMATION
			&& p->identifier != MQTT_PROP_SERVER_REFERENCE
			&& p->identifier != MQTT_PROP_REASON_STRING){

		return NULL;
	}

	if(value){
		*value = calloc(1, p->value.s.len+1);
		if(!(*value)) return NULL;

		memcpy(*value, p->value.s.v, p->value.s.len);
	}

	return p;
}

/* Process the incoming properties, we should be able to assume that only valid
 * properties for CONNECT are present here. */
int property__process_connect(struct mosquitto *context, mosquitto_property **props)
{
	mosquitto_property *p;

	p = *props;

	while(p){
		if(p->identifier == MQTT_PROP_SESSION_EXPIRY_INTERVAL){
			context->session_expiry_interval = p->value.i32;
		}else if(p->identifier == MQTT_PROP_RECEIVE_MAXIMUM){
			if(p->value.i16 == 0){
				return MOSQ_ERR_PROTOCOL;
			}

			context->msgs_out.inflight_maximum = p->value.i16;
			context->msgs_out.inflight_quota = context->msgs_out.inflight_maximum;
		}else if(p->identifier == MQTT_PROP_MAXIMUM_PACKET_SIZE){
			if(p->value.i32 == 0){
				return MOSQ_ERR_PROTOCOL;
			}
			context->maximum_packet_size = p->value.i32;
		}
		p = p->next;
	}

	return MOSQ_ERR_SUCCESS;
}


int property__read(struct mosquitto__packet *packet, int32_t *len, mosquitto_property *property)
{
	int rc;
	int32_t property_identifier;
	uint8_t byte;
	int8_t byte_count;
	uint16_t uint16;
	uint32_t uint32;
	int32_t varint;
	char *str1, *str2;
	int slen1, slen2;

	if(!property) return MOSQ_ERR_INVAL;

	rc = packet__read_varint(packet, &property_identifier, NULL);
	if(rc) return rc;
	*len -= 1;

	memset(property, 0, sizeof(mosquitto_property));

	property->identifier = property_identifier;

	switch(property_identifier){
		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
		case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
		case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
		case MQTT_PROP_SHARED_SUB_AVAILABLE:
			rc = packet__read_byte(packet, &byte);
			if(rc) return rc;
			*len -= 1; /* byte */
			property->value.i8 = byte;
			break;

		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS:
			rc = packet__read_uint16(packet, &uint16);
			if(rc) return rc;
			*len -= 2; /* uint16 */
			property->value.i16 = uint16;
			break;

		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
		case MQTT_PROP_WILL_DELAY_INTERVAL:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
			rc = packet__read_uint32(packet, &uint32);
			if(rc) return rc;
			*len -= 4; /* uint32 */
			property->value.i32 = uint32;
			break;

		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
			rc = packet__read_varint(packet, &varint, &byte_count);
			if(rc) return rc;
			*len -= byte_count;
			property->value.varint = varint;
			break;

		case MQTT_PROP_CONTENT_TYPE:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_SERVER_REFERENCE:
		case MQTT_PROP_REASON_STRING:
			rc = packet__read_string(packet, &str1, &slen1);
			if(rc) return rc;
			*len = (*len) - 2 - slen1; /* uint16, string len */
			property->value.s.v = str1;
			property->value.s.len = slen1;
			break;

		case MQTT_PROP_AUTHENTICATION_DATA:
		case MQTT_PROP_CORRELATION_DATA:
			rc = packet__read_binary(packet, (uint8_t **)&str1, &slen1);
			if(rc) return rc;
			*len = (*len) - 2 - slen1; /* uint16, binary len */
			property->value.bin.v = str1;
			property->value.bin.len = slen1;
			break;

		case MQTT_PROP_USER_PROPERTY:
			rc = packet__read_string(packet, &str1, &slen1);
			if(rc) return rc;
			*len = (*len) - 2 - slen1; /* uint16, string len */

			rc = packet__read_string(packet, &str2, &slen2);
			if(rc){
				mosquitto__free(str1);
				return rc;
			}
			*len = (*len) - 2 - slen2; /* uint16, string len */

			property->name.v = str1;
			property->name.len = slen1;
			property->value.s.v = str2;
			property->value.s.len = slen2;
			break;

		default:
			// log__printf(NULL, MOSQ_LOG_DEBUG, "Unsupported property type: %d", property_identifier);
			return MOSQ_ERR_MALFORMED_PACKET;
	}

	return MOSQ_ERR_SUCCESS;
}


int mosquitto_property_check_command(int command, int identifier)
{
	switch(identifier){
		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
		case MQTT_PROP_CONTENT_TYPE:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_CORRELATION_DATA:
			if(command != CMD_PUBLISH && command != CMD_WILL){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
			if(command != CMD_PUBLISH && command != CMD_SUBSCRIBE){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
			if(command != CMD_CONNECT && command != CMD_CONNACK && command != CMD_DISCONNECT){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_AUTHENTICATION_DATA:
			if(command != CMD_CONNECT && command != CMD_CONNACK && command != CMD_AUTH){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
		case MQTT_PROP_SHARED_SUB_AVAILABLE:
			if(command != CMD_CONNACK){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_WILL_DELAY_INTERVAL:
			if(command != CMD_WILL){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
		case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
			if(command != CMD_CONNECT){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_SERVER_REFERENCE:
			if(command != CMD_CONNACK && command != CMD_DISCONNECT){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_REASON_STRING:
			if(command == CMD_CONNECT || command == CMD_PUBLISH || command == CMD_SUBSCRIBE || command == CMD_UNSUBSCRIBE){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
			if(command != CMD_CONNECT && command != CMD_CONNACK){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_TOPIC_ALIAS:
			if(command != CMD_PUBLISH){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_USER_PROPERTY:
			break;

		default:
			return MOSQ_ERR_PROTOCOL;
	}
	return MOSQ_ERR_SUCCESS;
}


int mosquitto_property_check_all(int command, const mosquitto_property *properties)
{
	const mosquitto_property *p, *tail;
	int rc;

	p = properties;

	while(p){
		/* Validity checks */
		if(p->identifier == MQTT_PROP_REQUEST_PROBLEM_INFORMATION
				|| p->identifier == MQTT_PROP_REQUEST_RESPONSE_INFORMATION
				|| p->identifier == MQTT_PROP_MAXIMUM_QOS
				|| p->identifier == MQTT_PROP_RETAIN_AVAILABLE
				|| p->identifier == MQTT_PROP_WILDCARD_SUB_AVAILABLE
				|| p->identifier == MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE
				|| p->identifier == MQTT_PROP_SHARED_SUB_AVAILABLE){

			if(p->value.i8 > 1){
				return MOSQ_ERR_PROTOCOL;
			}
		}else if(p->identifier == MQTT_PROP_MAXIMUM_PACKET_SIZE){
			if( p->value.i32 == 0){
				return MOSQ_ERR_PROTOCOL;
			}
		}else if(p->identifier == MQTT_PROP_RECEIVE_MAXIMUM
				|| p->identifier == MQTT_PROP_TOPIC_ALIAS){

			if(p->value.i16 == 0){
				return MOSQ_ERR_PROTOCOL;
			}
		}

		/* Check for properties on incorrect commands */
		rc = mosquitto_property_check_command(command, p->identifier);
		if(rc) return rc;

		/* Check for duplicates */
		tail = p->next;
		while(tail){
			if(p->identifier == tail->identifier
					&& p->identifier != MQTT_PROP_USER_PROPERTY){

				return MOSQ_ERR_DUPLICATE_PROPERTY;
			}
			tail = tail->next;
		}

		p = p->next;
	}

	return MOSQ_ERR_SUCCESS;
}

void property__free(mosquitto_property **property)
{
	if(!property || !(*property)) return;

	switch((*property)->identifier){
		case MQTT_PROP_CONTENT_TYPE:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_SERVER_REFERENCE:
		case MQTT_PROP_REASON_STRING:
			mosquitto__free((*property)->value.s.v);
			break;

		case MQTT_PROP_AUTHENTICATION_DATA:
		case MQTT_PROP_CORRELATION_DATA:
			mosquitto__free((*property)->value.bin.v);
			break;

		case MQTT_PROP_USER_PROPERTY:
			mosquitto__free((*property)->name.v);
			mosquitto__free((*property)->value.s.v);
			break;

		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
		case MQTT_PROP_WILL_DELAY_INTERVAL:
		case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS:
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
		case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
		case MQTT_PROP_SHARED_SUB_AVAILABLE:
			/* Nothing to free */
			break;
	}

	free(*property);
	*property = NULL;
}


void mosquitto_property_free_all(mosquitto_property **property)
{
	mosquitto_property *p, *next;

	if(!property) return;

	p = *property;
	while(p){
		next = p->next;
		property__free(&p);
		p = next;
	}
	*property = NULL;
}

int property__read_all(int command, struct mosquitto__packet *packet, mosquitto_property **properties)
{
	int rc;
	int32_t proplen;
	mosquitto_property *p, *tail = NULL;

	rc = packet__read_varint(packet, &proplen, NULL);
	if(rc) return rc;

	*properties = NULL;

	/* The order of properties must be preserved for some types, so keep the
	 * same order for all */
	while(proplen > 0){
		p = mosquitto__calloc(1, sizeof(mosquitto_property));
		if(!p){
			mosquitto_property_free_all(properties);
			return MOSQ_ERR_NOMEM;
		}

		rc = property__read(packet, &proplen, p); 
		if(rc){
			mosquitto__free(p);
			mosquitto_property_free_all(properties);
			return rc;
		}

		if(!(*properties)){
			*properties = p;
		}else{
			tail->next = p;
		}
		tail = p;

	}

	rc = mosquitto_property_check_all(command, *properties);
	if(rc){
		mosquitto_property_free_all(properties);
		return rc;
	}
	return MOSQ_ERR_SUCCESS;
}


static void property__add(mosquitto_property **proplist, struct mqtt5__property *prop)
{
	mosquitto_property *p;

	if(!(*proplist)){
		*proplist = prop;
	}

	p = *proplist;
	while(p->next){
		p = p->next;
	}
	p->next = prop;
	prop->next = NULL;
}


int mosquitto_property_add_byte(mosquitto_property **proplist, int identifier, uint8_t value)
{
	mosquitto_property *prop;

	if(!proplist) return MOSQ_ERR_INVAL;
	if(identifier != MQTT_PROP_PAYLOAD_FORMAT_INDICATOR
			&& identifier != MQTT_PROP_REQUEST_PROBLEM_INFORMATION
			&& identifier != MQTT_PROP_REQUEST_RESPONSE_INFORMATION
			&& identifier != MQTT_PROP_MAXIMUM_QOS
			&& identifier != MQTT_PROP_RETAIN_AVAILABLE
			&& identifier != MQTT_PROP_WILDCARD_SUB_AVAILABLE
			&& identifier != MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE
			&& identifier != MQTT_PROP_SHARED_SUB_AVAILABLE){
		return MOSQ_ERR_INVAL;
	}

	prop = mosquitto__calloc(1, sizeof(mosquitto_property));
	if(!prop) return MOSQ_ERR_NOMEM;

	prop->client_generated = true;
	prop->identifier = identifier;
	prop->value.i8 = value;

	property__add(proplist, prop);
	return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_copy_all(mosquitto_property **dest, const mosquitto_property *src)
{
	mosquitto_property *pnew, *plast = NULL;

	if(!src) return MOSQ_ERR_SUCCESS;
	if(!dest) return MOSQ_ERR_INVAL;

	*dest = NULL;

	while(src){
		pnew = calloc(1, sizeof(mosquitto_property));
		if(!pnew){
			mosquitto_property_free_all(dest);
			return MOSQ_ERR_NOMEM;
		}
		if(plast){
			plast->next = pnew;
		}else{
			*dest = pnew;
		}
		plast = pnew;

		pnew->identifier = src->identifier;
		switch(pnew->identifier){
			case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
			case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
			case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
			case MQTT_PROP_MAXIMUM_QOS:
			case MQTT_PROP_RETAIN_AVAILABLE:
			case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
			case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
			case MQTT_PROP_SHARED_SUB_AVAILABLE:
				pnew->value.i8 = src->value.i8;
				break;

			case MQTT_PROP_SERVER_KEEP_ALIVE:
			case MQTT_PROP_RECEIVE_MAXIMUM:
			case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
			case MQTT_PROP_TOPIC_ALIAS:
				pnew->value.i16 = src->value.i16;
				break;

			case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
			case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
			case MQTT_PROP_WILL_DELAY_INTERVAL:
			case MQTT_PROP_MAXIMUM_PACKET_SIZE:
				pnew->value.i32 = src->value.i32;
				break;

			case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
				pnew->value.varint = src->value.varint;
				break;

			case MQTT_PROP_CONTENT_TYPE:
			case MQTT_PROP_RESPONSE_TOPIC:
			case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
			case MQTT_PROP_AUTHENTICATION_METHOD:
			case MQTT_PROP_RESPONSE_INFORMATION:
			case MQTT_PROP_SERVER_REFERENCE:
			case MQTT_PROP_REASON_STRING:
				pnew->value.s.len = src->value.s.len;
				pnew->value.s.v = strdup(src->value.s.v);
				if(!pnew->value.s.v){
					mosquitto_property_free_all(dest);
					return MOSQ_ERR_NOMEM;
				}
				break;

			case MQTT_PROP_AUTHENTICATION_DATA:
			case MQTT_PROP_CORRELATION_DATA:
				pnew->value.bin.len = src->value.bin.len;
				pnew->value.bin.v = malloc(pnew->value.bin.len);
				if(!pnew->value.bin.v){
					mosquitto_property_free_all(dest);
					return MOSQ_ERR_NOMEM;
				}
				memcpy(pnew->value.bin.v, src->value.bin.v, pnew->value.bin.len);
				break;

			case MQTT_PROP_USER_PROPERTY:
				pnew->value.s.len = src->value.s.len;
				pnew->value.s.v = strdup(src->value.s.v);
				if(!pnew->value.s.v){
					mosquitto_property_free_all(dest);
					return MOSQ_ERR_NOMEM;
				}

				pnew->name.len = src->name.len;
				pnew->name.v = strdup(src->name.v);
				if(!pnew->name.v){
					mosquitto_property_free_all(dest);
					return MOSQ_ERR_NOMEM;
				}
				break;

			default:
				mosquitto_property_free_all(dest);
				return MOSQ_ERR_INVAL;
		}

		src = src->next;
	}

	return MOSQ_ERR_SUCCESS;
}


int packet__varint_bytes(int32_t word)
{
	if(word < 128){
		return 1;
	}else if(word < 16384){
		return 2;
	}else if(word < 2097152){
		return 3;
	}else if(word < 268435456){
		return 4;
	}else{
		return 5;
	}
}


void packet__write_byte(struct mosquitto__packet *packet, uint8_t byte)
{
	assert(packet);
	assert(packet->pos+1 <= packet->packet_length);

	packet->payload[packet->pos] = byte;
	packet->pos++;
}

void packet__write_bytes(struct mosquitto__packet *packet, const void *bytes, uint32_t count)
{
	assert(packet);
	assert(packet->pos+count <= packet->packet_length);

	memcpy(&(packet->payload[packet->pos]), bytes, count);
	packet->pos += count;
}


int packet__write_varint(struct mosquitto__packet *packet, int32_t word)
{
	uint8_t byte;
	int count = 0;

	do{
		byte = word % 128;
		word = word / 128;
		/* If there are more digits to encode, set the top bit of this digit */
		if(word > 0){
			byte = byte | 0x80;
		}
		packet__write_byte(packet, byte);
		count++;
	}while(word > 0 && count < 5);

	if(count == 5){
		return MOSQ_ERR_PROTOCOL;
	}
	return MOSQ_ERR_SUCCESS;
}

void packet__write_uint16(struct mosquitto__packet *packet, uint16_t word)
{
	packet__write_byte(packet, MOSQ_MSB(word));
	packet__write_byte(packet, MOSQ_LSB(word));
}

void packet__write_uint32(struct mosquitto__packet *packet, uint32_t word)
{
	packet__write_byte(packet, (word & 0xFF000000) >> 24);
	packet__write_byte(packet, (word & 0x00FF0000) >> 16);
	packet__write_byte(packet, (word & 0x0000FF00) >> 8);
	packet__write_byte(packet, (word & 0x000000FF));
}


void packet__write_string(struct mosquitto__packet *packet, const char *str, uint16_t length)
{
	assert(packet);
	packet__write_uint16(packet, length);
	packet__write_bytes(packet, str, length);
}

int property__write(struct mosquitto__packet *packet, const mosquitto_property *property)
{
	int rc;

	rc = packet__write_varint(packet, property->identifier);
	if(rc) return rc;

	switch(property->identifier){
		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
		case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
		case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
		case MQTT_PROP_SHARED_SUB_AVAILABLE:
			packet__write_byte(packet, property->value.i8);
			break;

		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS:
			packet__write_uint16(packet, property->value.i16);
			break;

		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
		case MQTT_PROP_WILL_DELAY_INTERVAL:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
			packet__write_uint32(packet, property->value.i32);
			break;

		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
			return packet__write_varint(packet, property->value.varint);

		case MQTT_PROP_CONTENT_TYPE:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_SERVER_REFERENCE:
		case MQTT_PROP_REASON_STRING:
			packet__write_string(packet, property->value.s.v, property->value.s.len);
			break;

		case MQTT_PROP_AUTHENTICATION_DATA:
		case MQTT_PROP_CORRELATION_DATA:
			packet__write_uint16(packet, property->value.bin.len);
			packet__write_bytes(packet, property->value.bin.v, property->value.bin.len);
			break;

		case MQTT_PROP_USER_PROPERTY:
			packet__write_string(packet, property->name.v, property->name.len);
			packet__write_string(packet, property->value.s.v, property->value.s.len);
			break;

		default:
			//  log__printf(NULL, MOSQ_LOG_DEBUG, "Unsupported property type: %d", property->identifier);
			return MOSQ_ERR_INVAL;
	}

	return MOSQ_ERR_SUCCESS;
}


int property__get_length(const mosquitto_property *property)
{
	if(!property) return 0;

	switch(property->identifier){
		/* Byte */
		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
		case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
		case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
		case MQTT_PROP_SHARED_SUB_AVAILABLE:
			return 2; /* 1 (identifier) + 1 byte */

		/* uint16 */
		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS:
			return 3; /* 1 (identifier) + 2 bytes */

		/* uint32 */
		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
		case MQTT_PROP_WILL_DELAY_INTERVAL:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
			return 5; /* 1 (identifier) + 4 bytes */

		/* varint */
		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
			if(property->value.varint < 128){
				return 2;
			}else if(property->value.varint < 16384){
				return 3;
			}else if(property->value.varint < 2097152){
				return 4;
			}else if(property->value.varint < 268435456){
				return 5;
			}else{
				return 0;
			}

		/* binary */
		case MQTT_PROP_CORRELATION_DATA:
		case MQTT_PROP_AUTHENTICATION_DATA:
			return 3 + property->value.bin.len; /* 1 + 2 bytes (len) + X bytes (payload) */

		/* string */
		case MQTT_PROP_CONTENT_TYPE:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_SERVER_REFERENCE:
		case MQTT_PROP_REASON_STRING:
			return 3 + property->value.s.len; /* 1 + 2 bytes (len) + X bytes (string) */

		/* string pair */
		case MQTT_PROP_USER_PROPERTY:
			return 5 + property->value.s.len + property->name.len; /* 1 + 2*(2 bytes (len) + X bytes (string))*/

		default:
			return 0;
	}
	return 0;
}

int property__get_length_all(const mosquitto_property *property)
{
	const mosquitto_property *p;
	int len = 0;

	p = property;
	while(p){
		len += property__get_length(p);
		p = p->next;
	}
	return len;
}

int property__write_all(struct mosquitto__packet *packet, const mosquitto_property *properties, bool write_len)
{
	int rc;
	const mosquitto_property *p;

	if(write_len){
		rc = packet__write_varint(packet, property__get_length_all(properties));
		if(rc) return rc;
	}

	p = properties;
	while(p){
		rc = property__write(packet, p);
		if(rc) return rc;
		p = p->next;
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_add_int16(mosquitto_property **proplist, int identifier, uint16_t value)
{
	mosquitto_property *prop;

	if(!proplist) return MOSQ_ERR_INVAL;
	if(identifier != MQTT_PROP_SERVER_KEEP_ALIVE
			&& identifier != MQTT_PROP_RECEIVE_MAXIMUM
			&& identifier != MQTT_PROP_TOPIC_ALIAS_MAXIMUM
			&& identifier != MQTT_PROP_TOPIC_ALIAS){
		return MOSQ_ERR_INVAL;
	}

	prop = mosquitto__calloc(1, sizeof(mosquitto_property));
	if(!prop) return MOSQ_ERR_NOMEM;

	prop->client_generated = true;
	prop->identifier = identifier;
	prop->value.i16 = value;

	property__add(proplist, prop);
	return MOSQ_ERR_SUCCESS;
}


int mosquitto_property_add_int32(mosquitto_property **proplist, int identifier, uint32_t value)
{
	mosquitto_property *prop;

	if(!proplist) return MOSQ_ERR_INVAL;
	if(identifier != MQTT_PROP_MESSAGE_EXPIRY_INTERVAL
			&& identifier != MQTT_PROP_SESSION_EXPIRY_INTERVAL
			&& identifier != MQTT_PROP_WILL_DELAY_INTERVAL
			&& identifier != MQTT_PROP_MAXIMUM_PACKET_SIZE){

		return MOSQ_ERR_INVAL;
	}

	prop = mosquitto__calloc(1, sizeof(mosquitto_property));
	if(!prop) return MOSQ_ERR_NOMEM;

	prop->client_generated = true;
	prop->identifier = identifier;
	prop->value.i32 = value;

	property__add(proplist, prop);
	return MOSQ_ERR_SUCCESS;
}


int mosquitto_property_add_binary(mosquitto_property **proplist, int identifier, const void *value, uint16_t len)
{
	mosquitto_property *prop;

	if(!proplist) return MOSQ_ERR_INVAL;
	if(identifier != MQTT_PROP_CORRELATION_DATA
			&& identifier != MQTT_PROP_AUTHENTICATION_DATA){

		return MOSQ_ERR_INVAL;
	}

	prop = mosquitto__calloc(1, sizeof(mosquitto_property));
	if(!prop) return MOSQ_ERR_NOMEM;

	prop->client_generated = true;
	prop->identifier = identifier;

	if(len){
		prop->value.bin.v = mosquitto__malloc(len);
		if(!prop->value.bin.v){
			mosquitto__free(prop);
			return MOSQ_ERR_NOMEM;
		}

		memcpy(prop->value.bin.v, value, len);
		prop->value.bin.len = len;
	}

	property__add(proplist, prop);
	return MOSQ_ERR_SUCCESS;
}


int mosquitto_property_add_string(mosquitto_property **proplist, int identifier, const char *value)
{
	mosquitto_property *prop;

	if(!proplist) return MOSQ_ERR_INVAL;
	if(value){
		if(mosquitto_validate_utf8(value, strlen(value))) return MOSQ_ERR_MALFORMED_UTF8;
	}

	if(identifier != MQTT_PROP_CONTENT_TYPE
			&& identifier != MQTT_PROP_RESPONSE_TOPIC
			&& identifier != MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER
			&& identifier != MQTT_PROP_AUTHENTICATION_METHOD
			&& identifier != MQTT_PROP_RESPONSE_INFORMATION
			&& identifier != MQTT_PROP_SERVER_REFERENCE
			&& identifier != MQTT_PROP_REASON_STRING){

		return MOSQ_ERR_INVAL;
	}

	prop = mosquitto__calloc(1, sizeof(mosquitto_property));
	if(!prop) return MOSQ_ERR_NOMEM;

	prop->client_generated = true;
	prop->identifier = identifier;
	if(value && strlen(value)){
		prop->value.s.v = mosquitto__strdup(value);
		if(!prop->value.s.v){
			mosquitto__free(prop);
			return MOSQ_ERR_NOMEM;
		}
		prop->value.s.len = strlen(value);
	}

	property__add(proplist, prop);
	return MOSQ_ERR_SUCCESS;
}

int packet__check_oversize(struct mosquitto *mosq, uint32_t remaining_length)
{
	uint32_t len;

	if(mosq->maximum_packet_size == 0) return MOSQ_ERR_SUCCESS;

	len = remaining_length + packet__varint_bytes(remaining_length);
	if(len > mosq->maximum_packet_size){
		return MOSQ_ERR_OVERSIZE_PACKET;
	}else{
		return MOSQ_ERR_SUCCESS;
	}
}


 int packet__alloc(struct mosquitto__packet *packet)
{
	uint8_t remaining_bytes[5], byte;
	uint32_t remaining_length;
	int i;

	assert(packet);

	remaining_length = packet->remaining_length;
	packet->payload = NULL;
	packet->remaining_count = 0;
	do{
		byte = remaining_length % 128;
		remaining_length = remaining_length / 128;
		/* If there are more digits to encode, set the top bit of this digit */
		if(remaining_length > 0){
			byte = byte | 0x80;
		}
		remaining_bytes[packet->remaining_count] = byte;
		packet->remaining_count++;
	}while(remaining_length > 0 && packet->remaining_count < 5);
	if(packet->remaining_count == 5) return MOSQ_ERR_PAYLOAD_SIZE;
	packet->packet_length = packet->remaining_length + 1 + packet->remaining_count;
#ifdef WITH_WEBSOCKETS
	packet->payload = mosquitto__malloc(sizeof(uint8_t)*packet->packet_length + LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING);
#else
	packet->payload = mosquitto__malloc(sizeof(uint8_t)*packet->packet_length);
#endif
	if(!packet->payload) return MOSQ_ERR_NOMEM;

	packet->payload[0] = packet->command;
	for(i=0; i<packet->remaining_count; i++){
		packet->payload[i+1] = remaining_bytes[i];
	}
	packet->pos = 1 + packet->remaining_count;

	return MOSQ_ERR_SUCCESS;
}


 void packet__cleanup(struct mosquitto__packet *packet)
{
	if(!packet) return;

	/* Free data and reset values */
	packet->command = 0;
	packet->remaining_count = 0;
	packet->remaining_mult = 1;
	packet->remaining_length = 0;
	mosquitto__free(packet->payload);
	packet->payload = NULL;
	packet->to_process = 0;
	packet->pos = 0;
}

int packet__write(struct mosquitto *mosq)
{
	ssize_t write_length;
	struct mosquitto__packet *packet;
	int state;

//  	if(!mosq) return MOSQ_ERR_INVAL;
//  	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;
//  
//  	pthread_mutex_lock(&mosq->current_out_packet_mutex);
//  	pthread_mutex_lock(&mosq->out_packet_mutex);
//  	if(mosq->out_packet && !mosq->current_out_packet){
//  		mosq->current_out_packet = mosq->out_packet;
//  		mosq->out_packet = mosq->out_packet->next;
//  		if(!mosq->out_packet){
//  			mosq->out_packet_last = NULL;
//  		}
//  	}
//  	pthread_mutex_unlock(&mosq->out_packet_mutex);
//  
//  	state = mosquitto__get_state(mosq);
//  //  #if defined(WITH_TLS) && !defined(WITH_BROKER)
//  //  	if((state == mosq_cs_connect_pending) || mosq->want_connect){
//  //  #else
//  	if(state == mosq_cs_connect_pending){
//  //  #endif
//  		pthread_mutex_unlock(&mosq->current_out_packet_mutex);
//  		return MOSQ_ERR_SUCCESS;
//  	}
//  
//  	while(mosq->current_out_packet){
//  		packet = mosq->current_out_packet;
//  
//  		while(packet->to_process > 0){
//  			write_length = net__write(mosq, &(packet->payload[packet->pos]), packet->to_process);
//  			if(write_length > 0){
//  				G_BYTES_SENT_INC(write_length);
//  				packet->to_process -= write_length;
//  				packet->pos += write_length;
//  			}else{
//  //  #ifdef WIN32
//  //  				errno = WSAGetLastError();
//  //  #endif
//  				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK
//  //  #ifdef WIN32
//  //  						|| errno == WSAENOTCONN
//  //  #endif
//  						){
//  					pthread_mutex_unlock(&mosq->current_out_packet_mutex);
//  					return MOSQ_ERR_SUCCESS;
//  				}else{
//  					pthread_mutex_unlock(&mosq->current_out_packet_mutex);
//  					switch(errno){
//  						case COMPAT_ECONNRESET:
//  							return MOSQ_ERR_CONN_LOST;
//  						default:
//  							return MOSQ_ERR_ERRNO;
//  					}
//  				}
//  			}
//  		}
//  
//  		// G_MSGS_SENT_INC(1);
//  		if(((packet->command)&0xF6) == CMD_PUBLISH){
//  			G_PUB_MSGS_SENT_INC(1);
//  #ifndef WITH_BROKER
//  			pthread_mutex_lock(&mosq->callback_mutex);
//  			if(mosq->on_publish){
//  				/* This is a QoS=0 message */
//  				mosq->in_callback = true;
//  				mosq->on_publish(mosq, mosq->userdata, packet->mid);
//  				mosq->in_callback = false;
//  			}
//  			if(mosq->on_publish_v5){
//  				/* This is a QoS=0 message */
//  				mosq->in_callback = true;
//  				mosq->on_publish_v5(mosq, mosq->userdata, packet->mid, 0, NULL);
//  				mosq->in_callback = false;
//  			}
//  			pthread_mutex_unlock(&mosq->callback_mutex);
//  		}else if(((packet->command)&0xF0) == CMD_DISCONNECT){
//  			do_client_disconnect(mosq, MOSQ_ERR_SUCCESS, NULL);
//  			packet__cleanup(packet);
//  			mosquitto__free(packet);
//  			return MOSQ_ERR_SUCCESS;
//  #endif
//  		}
//  
//  		/* Free data and reset values */
//  		pthread_mutex_lock(&mosq->out_packet_mutex);
//  		mosq->current_out_packet = mosq->out_packet;
//  		if(mosq->out_packet){
//  			mosq->out_packet = mosq->out_packet->next;
//  			if(!mosq->out_packet){
//  				mosq->out_packet_last = NULL;
//  			}
//  		}
//  		pthread_mutex_unlock(&mosq->out_packet_mutex);
//  
//  		packet__cleanup(packet);
//  		mosquitto__free(packet);
//  
//  		pthread_mutex_lock(&mosq->msgtime_mutex);
//  		mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
//  		pthread_mutex_unlock(&mosq->msgtime_mutex);
//  	}
//  	pthread_mutex_unlock(&mosq->current_out_packet_mutex);
	return MOSQ_ERR_SUCCESS;
}

int packet__queue(struct mosquitto *mosq, struct mosquitto__packet *packet)
{
#ifndef WITH_BROKER
	char sockpair_data = 0;
#endif
	assert(mosq);
	assert(packet);

	packet->pos = 0;
	packet->to_process = packet->packet_length;

	packet->next = NULL;
//	pthread_mutex_lock(&mosq->out_packet_mutex);
//	if(mosq->out_packet){
//		mosq->out_packet_last->next = packet;
//	}else{
//		mosq->out_packet = packet;
//	}
//	mosq->out_packet_last = packet;
//	pthread_mutex_unlock(&mosq->out_packet_mutex);
//#ifdef WITH_BROKER
  //  #  ifdef WITH_WEBSOCKETS
  //  	if(mosq->wsi){
  //  		libwebsocket_callback_on_writable(mosq->ws_context, mosq->wsi);
  //  		return MOSQ_ERR_SUCCESS;
  //  	}else{
  //  		return packet__write(mosq);
  //  	}
  //  #  else
return packet__write(mosq);
  //  #  endif
//#else
//  
//  	/* Write a single byte to sockpairW (connected to sockpairR) to break out
//  	 * of select() if in threaded mode. */
//  	if(mosq->sockpairW != INVALID_SOCKET){
//  //  #ifndef WIN32
//  //  		if(write(mosq->sockpairW, &sockpair_data, 1)){
//  //  		}
//  //  #else
//  		send(mosq->sockpairW, &sockpair_data, 1, 0);
//  // #endif
//  	}
//  
//  	if(mosq->in_callback == false && mosq->threaded == mosq_ts_none){
//  		return packet__write(mosq);
//  	}else{
//  		return MOSQ_ERR_SUCCESS;
//  	}
//  #endif
}
