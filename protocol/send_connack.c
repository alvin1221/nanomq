#include "mqtt_protocol.h"
#include "mosquitto_base.h"

int send__connack(struct mosquitto_db *db, struct mosquitto *context, int ack, int reason_code, const mosquitto_property *properties)
{
	struct mosquitto__packet *packet = NULL;
	int rc;
	mosquitto_property *connack_props = NULL;
	int proplen, varbytes;
	uint32_t remaining_length;

	rc = mosquitto_property_copy_all(&connack_props, properties);
	if(rc){
		return rc;
	}

	if(context->id){
		//  log__printf(NULL, MOSQ_LOG_DEBUG, "Sending CONNACK to %s (%d, %d)", context->id, ack, reason_code);
	}else{
		//  log__printf(NULL, MOSQ_LOG_DEBUG, "Sending CONNACK to %s (%d, %d)", context->address, ack, reason_code);
	}

	remaining_length = 2;

	if(context->protocol == mosq_p_mqtt5){
		if(reason_code < 128 && db->config->retain_available == false){
			rc = mosquitto_property_add_byte(&connack_props, MQTT_PROP_RETAIN_AVAILABLE, 0);
			if(rc){
				mosquitto_property_free_all(&connack_props);
				return rc;
			}
		}
		if(db->config->max_packet_size > 0){
			rc = mosquitto_property_add_int32(&connack_props, MQTT_PROP_MAXIMUM_PACKET_SIZE, db->config->max_packet_size);
			if(rc){
				mosquitto_property_free_all(&connack_props);
				return rc;
			}
		}

		proplen = property__get_length_all(connack_props);
		varbytes = packet__varint_bytes(proplen);
		remaining_length += proplen + varbytes;
	}

	if(packet__check_oversize(context, remaining_length)){
		mosquitto_property_free_all(&connack_props);
		mosquitto__free(packet);
		return MOSQ_ERR_OVERSIZE_PACKET;
	}

	packet = mosquitto__calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->command = CMD_CONNACK;
	packet->remaining_length = remaining_length;

	rc = packet__alloc(packet);
	if(rc){
		mosquitto_property_free_all(&connack_props);
		mosquitto__free(packet);
		return rc;
	}
	packet__write_byte(packet, ack);
	packet__write_byte(packet, reason_code);
	if(context->protocol == mosq_p_mqtt5){
		property__write_all(packet, connack_props, true);
	}
	mosquitto_property_free_all(&connack_props);

	return packet__queue(context, packet);
}

int send__auth(struct mosquitto_db *db, struct mosquitto *context, int reason_code, const void *auth_data, uint16_t auth_data_len)
{
	struct mosquitto__packet *packet = NULL;
	int rc;
	mosquitto_property *properties = NULL;
	int proplen, varbytes;
	uint32_t remaining_length;

	if(context->auth_method == NULL) return MOSQ_ERR_INVAL;
	if(context->protocol != mosq_p_mqtt5) return MOSQ_ERR_PROTOCOL;

	//	log__printf(NULL, MOSQ_LOG_DEBUG, "Sending AUTH to %s (rc%d, %s)", context->id, reason_code, context->auth_method);

	remaining_length = 1;

	rc = mosquitto_property_add_string(&properties, MQTT_PROP_AUTHENTICATION_METHOD, context->auth_method);
	if(rc){
		mosquitto_property_free_all(&properties);
		return rc;
	}

	if(auth_data != NULL && auth_data_len > 0){
		rc = mosquitto_property_add_binary(&properties, MQTT_PROP_AUTHENTICATION_DATA, auth_data, auth_data_len);
		if(rc){
			mosquitto_property_free_all(&properties);
			return rc;
		}
	}

	proplen = property__get_length_all(properties);
	varbytes = packet__varint_bytes(proplen);
	remaining_length += proplen + varbytes;

	if(packet__check_oversize(context, remaining_length)){
		mosquitto_property_free_all(&properties);
		mosquitto__free(packet);
		return MOSQ_ERR_OVERSIZE_PACKET;
	}

	packet = mosquitto__calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->command = CMD_AUTH;
	packet->remaining_length = remaining_length;

	rc = packet__alloc(packet);
	if(rc){
		mosquitto_property_free_all(&properties);
		mosquitto__free(packet);
		return rc;
	}
	packet__write_byte(packet, reason_code);
	property__write_all(packet, properties, true);
	mosquitto_property_free_all(&properties);

	return packet__queue(context, packet);
}

