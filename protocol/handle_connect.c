#include "handle_connect.h"
#include "send_connack.c"




static int will_delay__cmp(struct will_delay_list *i1, struct will_delay_list *i2)
{
	return i1->context->will_delay_interval - i2->context->will_delay_interval;
}

int will_delay__add(struct mosquitto *context)
{
	struct will_delay_list *item;

	item = mosquitto__calloc(1, sizeof(struct will_delay_list));
	if(!item) return MOSQ_ERR_NOMEM;

	item->context = context;
	context->will_delay_entry = item;
	item->context->will_delay_time = time(NULL) + item->context->will_delay_interval;

	DL_INSERT_INORDER(delay_list, item, will_delay__cmp);

	return MOSQ_ERR_SUCCESS;
}


/* Does a topic match a subscription? */
int mosquitto_topic_matches_sub2(const char *sub, size_t sublen, const char *topic, size_t topiclen, bool *result)
{
	size_t spos;

	UNUSED(sublen);
	UNUSED(topiclen);

	if(!result) return MOSQ_ERR_INVAL;
	*result = false;

	if(!sub || !topic || sub[0] == 0 || topic[0] == 0){
		return MOSQ_ERR_INVAL;
	}

	if((sub[0] == '$' && topic[0] != '$')
			|| (topic[0] == '$' && sub[0] != '$')){

		return MOSQ_ERR_SUCCESS;
	}

	spos = 0;

	while(sub[0] != 0){
		if(topic[0] == '+' || topic[0] == '#'){
			return MOSQ_ERR_INVAL;
		}
		if(sub[0] != topic[0] || topic[0] == 0){ /* Check for wildcard matches */
			if(sub[0] == '+'){
				/* Check for bad "+foo" or "a/+foo" subscription */
				if(spos > 0 && sub[-1] != '/'){
					return MOSQ_ERR_INVAL;
				}
				/* Check for bad "foo+" or "foo+/a" subscription */
				if(sub[1] != 0 && sub[1] != '/'){
					return MOSQ_ERR_INVAL;
				}
				spos++;
				sub++;
				while(topic[0] != 0 && topic[0] != '/'){
					if(topic[0] == '+' || topic[0] == '#'){
						return MOSQ_ERR_INVAL;
					}
					topic++;
				}
				if(topic[0] == 0 && sub[0] == 0){
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}
			}else if(sub[0] == '#'){
				/* Check for bad "foo#" subscription */
				if(spos > 0 && sub[-1] != '/'){
					return MOSQ_ERR_INVAL;
				}
				/* Check for # not the final character of the sub, e.g. "#foo" */
				if(sub[1] != 0){
					return MOSQ_ERR_INVAL;
				}else{
					while(topic[0] != 0){
						if(topic[0] == '+' || topic[0] == '#'){
							return MOSQ_ERR_INVAL;
						}
						topic++;
					}
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}
			}else{
				/* Check for e.g. foo/bar matching foo/+/# */
				if(topic[0] == 0
						&& spos > 0
						&& sub[-1] == '+'
						&& sub[0] == '/'
						&& sub[1] == '#')
				{
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}

				/* There is no match at this point, but is the sub invalid? */
				while(sub[0] != 0){
					if(sub[0] == '#' && sub[1] != 0){
						return MOSQ_ERR_INVAL;
					}
					spos++;
					sub++;
				}

				/* Valid input, but no match */
				return MOSQ_ERR_SUCCESS;
			}
		}else{
			/* sub[spos] == topic[tpos] */
			if(topic[1] == 0){
				/* Check for e.g. foo matching foo/# */
				if(sub[1] == '/'
						&& sub[2] == '#'
						&& sub[3] == 0){
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}
			}
			spos++;
			sub++;
			topic++;
			if(sub[0] == 0 && topic[0] == 0){
				*result = true;
				return MOSQ_ERR_SUCCESS;
			}else if(topic[0] == 0 && sub[0] == '+' && sub[1] == 0){
				if(spos > 0 && sub[-1] != '/'){
					return MOSQ_ERR_INVAL;
				}
				spos++;
				sub++;
				*result = true;
				return MOSQ_ERR_SUCCESS;
			}
		}
	}
	if((topic[0] != 0 || sub[0] != 0)){
		*result = false;
	}
	while(topic[0] != 0){
		if(topic[0] == '+' || topic[0] == '#'){
			return MOSQ_ERR_INVAL;
		}
		topic++;
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool *result)
{
	return mosquitto_topic_matches_sub2(sub, 0, topic, 0, result);
}


int mosquitto_acl_check_default(struct mosquitto_db *db, struct mosquitto *context, const char *topic, int access)
{
	char *local_acl;
	struct mosquitto__acl *acl_root;
	bool result;
	int i;
	int len, tlen, clen, ulen;
	char *s;
	struct mosquitto__security_options *security_opts = NULL;

	if(!db || !context || !topic) return MOSQ_ERR_INVAL;
	if(context->bridge) return MOSQ_ERR_SUCCESS;

	if(db->config->per_listener_settings){
		if(!context->listener) return MOSQ_ERR_ACL_DENIED;
		security_opts = &context->listener->security_options;
	}else{
		security_opts = &db->config->security_options;
	}
	if(!security_opts->acl_file && !security_opts->acl_list && !security_opts->acl_patterns){
			return MOSQ_ERR_PLUGIN_DEFER;
	}

	if(access == MOSQ_ACL_SUBSCRIBE) return MOSQ_ERR_SUCCESS; /* FIXME - implement ACL subscription strings. */
	if(!context->acl_list && !security_opts->acl_patterns) return MOSQ_ERR_ACL_DENIED;

	if(context->acl_list){
		acl_root = context->acl_list->acl;
	}else{
		acl_root = NULL;
	}

	/* Loop through all ACLs for this client. */
	while(acl_root){
		/* Loop through the topic looking for matches to this ACL. */

		/* If subscription starts with $, acl_root->topic must also start with $. */
		if(topic[0] == '$' && acl_root->topic[0] != '$'){
			acl_root = acl_root->next;
			continue;
		}
		mosquitto_topic_matches_sub(acl_root->topic, topic, &result);
		if(result){
			if(access & acl_root->access){
				/* And access is allowed. */
				return MOSQ_ERR_SUCCESS;
			}
		}
		acl_root = acl_root->next;
	}

	acl_root = security_opts->acl_patterns;

	if(acl_root){
		/* We are using pattern based acls. Check whether the username or
		 * client id contains a + or # and if so deny access.
		 *
		 * Without this, a malicious client may configure its username/client
		 * id to bypass ACL checks (or have a username/client id that cannot
		 * publish or receive messages to its own place in the hierarchy).
		 */
		if(context->username && strpbrk(context->username, "+#")){
			// log__printf(NULL, MOSQ_LOG_NOTICE, "ACL denying access to client with dangerous username \"%s\"", context->username);
			return MOSQ_ERR_ACL_DENIED;
		}

		if(context->id && strpbrk(context->id, "+#")){
			// log__printf(NULL, MOSQ_LOG_NOTICE, "ACL denying access to client with dangerous client id \"%s\"", context->id);
			return MOSQ_ERR_ACL_DENIED;
		}
	}

	/* Loop through all pattern ACLs. */
	if(!context->id) return MOSQ_ERR_ACL_DENIED;
	clen = strlen(context->id);

	while(acl_root){
		tlen = strlen(acl_root->topic);

		if(acl_root->ucount && !context->username){
			acl_root = acl_root->next;
			continue;
		}

		if(context->username){
			ulen = strlen(context->username);
			len = tlen + acl_root->ccount*(clen-2) + acl_root->ucount*(ulen-2);
		}else{
			ulen = 0;
			len = tlen + acl_root->ccount*(clen-2);
		}
		local_acl = mosquitto__malloc(len+1);
		if(!local_acl) return 1; /* FIXME */
		s = local_acl;
		for(i=0; i<tlen; i++){
			if(i<tlen-1 && acl_root->topic[i] == '%'){
				if(acl_root->topic[i+1] == 'c'){
					i++;
					strncpy(s, context->id, clen);
					s+=clen;
					continue;
				}else if(context->username && acl_root->topic[i+1] == 'u'){
					i++;
					strncpy(s, context->username, ulen);
					s+=ulen;
					continue;
				}
			}
			s[0] = acl_root->topic[i];
			s++;
		}
		local_acl[len] = '\0';

		mosquitto_topic_matches_sub(local_acl, topic, &result);
		mosquitto__free(local_acl);
		if(result){
			if(access & acl_root->access){
				/* And access is allowed. */
				return MOSQ_ERR_SUCCESS;
			}
		}

		acl_root = acl_root->next;
	}

	return MOSQ_ERR_ACL_DENIED;
}

static int acl__check_dollar(const char *topic, int access)
{
	int rc;
	bool match = false;

	if(topic[0] != '$') return MOSQ_ERR_SUCCESS;

	if(!strncmp(topic, "$SYS", 4)){
		if(access == MOSQ_ACL_WRITE){
			/* Potentially allow write access for bridge status, otherwise explicitly deny. */
			rc = mosquitto_topic_matches_sub("$SYS/broker/connection/+/state", topic, &match);
			if(rc == MOSQ_ERR_SUCCESS && match == true){
				return MOSQ_ERR_SUCCESS;
			}else{
				return MOSQ_ERR_ACL_DENIED;
			}
		}else{
			return MOSQ_ERR_SUCCESS;
		}
	}else if(!strncmp(topic, "$share", 6)){
		/* Only allow sub/unsub to shared subscriptions */
		if(access == MOSQ_ACL_SUBSCRIBE){
		/* FIXME if(access == MOSQ_ACL_SUBSCRIBE || access == MOSQ_ACL_UNSUBSCRIBE){ */
			return MOSQ_ERR_SUCCESS;
		}else{
			return MOSQ_ERR_ACL_DENIED;
		}
	}else{
		/* This is an unknown $ topic, for the moment just defer to actual tests. */
		return MOSQ_ERR_SUCCESS;
	}
}


/* int mosquitto_acl_check(struct mosquitto_db *db, struct mosquitto *context, const char *topic, int access) */
static int acl__check_single(struct mosquitto__auth_plugin_config *auth_plugin, struct mosquitto *context, struct mosquitto_acl_msg *msg, int access)
{
	const char *username;
	const char *topic = msg->topic;

	username = mosquitto_client_username(context);
	if(auth_plugin->deny_special_chars == true){
		/* Check whether the client id or username contains a +, # or / and if
		* so deny access.
		*
		* Do this check for every message regardless, we have to protect the
		* plugins against possible pattern based attacks.
		*/
		if(username && strpbrk(username, "+#")){
			// log__printf(NULL, MOSQ_LOG_NOTICE, "ACL denying access to client with dangerous username \"%s\"", username);
			return MOSQ_ERR_ACL_DENIED;
		}
		if(context->id && strpbrk(context->id, "+#")){
			// log__printf(NULL, MOSQ_LOG_NOTICE, "ACL denying access to client with dangerous client id \"%s\"", context->id);
			return MOSQ_ERR_ACL_DENIED;
		}
	}

	if(auth_plugin->plugin.version == 4){
		return auth_plugin->plugin.acl_check_v4(auth_plugin->plugin.user_data, access, context, msg);
	}else if(auth_plugin->plugin.version == 3){
		return auth_plugin->plugin.acl_check_v3(auth_plugin->plugin.user_data, access, context, msg);
	}else if(auth_plugin->plugin.version == 2){
		if(access == MOSQ_ACL_SUBSCRIBE){
			return MOSQ_ERR_SUCCESS;
		}
		return auth_plugin->plugin.acl_check_v2(auth_plugin->plugin.user_data, context->id, username, topic, access);
	}else{
		return MOSQ_ERR_INVAL;
	}
}


int mosquitto_acl_check(struct mosquitto_db *db, struct mosquitto *context, const char *topic, long payloadlen, void* payload, int qos, bool retain, int access)
{
	int rc;
	int i;
	struct mosquitto__security_options *opts;
	struct mosquitto_acl_msg msg;

	if(!context->id){
		return MOSQ_ERR_ACL_DENIED;
	}

	rc = acl__check_dollar(topic, access);
	if(rc) return rc;

	rc = mosquitto_acl_check_default(db, context, topic, access);
	if(rc != MOSQ_ERR_PLUGIN_DEFER){
		return rc;
	}
	/* Default check has accepted or deferred at this point.
	 * If no plugins exist we should accept at this point so set rc to success.
	 */
	rc = MOSQ_ERR_SUCCESS;

	if(db->config->per_listener_settings){
		opts = &context->listener->security_options;
	}else{
		opts = &db->config->security_options;
	}

	memset(&msg, 0, sizeof(msg));
	msg.topic = topic;
	msg.payloadlen = payloadlen;
	msg.payload = payload;
	msg.qos = qos;
	msg.retain = retain;

	for(i=0; i<opts->auth_plugin_config_count; i++){
		rc = acl__check_single(&opts->auth_plugin_configs[i], context, &msg, access);
		if(rc != MOSQ_ERR_PLUGIN_DEFER){
			return rc;
		}
	}

	/* If all plugins deferred, this is a denial. If rc == MOSQ_ERR_SUCCESS
	 * here, then no plugins were configured. */
	if(rc == MOSQ_ERR_PLUGIN_DEFER){
		rc = MOSQ_ERR_ACL_DENIED;
	}
	return rc;
}

/* Check that a topic used for publishing is valid.
 * Search for + or # in a topic. Return MOSQ_ERR_INVAL if found.
 * Also returns MOSQ_ERR_INVAL if the topic string is too long.
 * Returns MOSQ_ERR_SUCCESS if everything is fine.
 */
int mosquitto_pub_topic_check(const char *str)
{
	int len = 0;
#ifdef WITH_BROKER
	int hier_count = 0;
#endif
	while(str && str[0]){
		if(str[0] == '+' || str[0] == '#'){
			return MOSQ_ERR_INVAL;
		}
#ifdef WITH_BROKER
		else if(str[0] == '/'){
			hier_count++;
		}
#endif
		len++;
		str = &str[1];
	}
	if(len > 65535) return MOSQ_ERR_INVAL;
#ifdef WITH_BROKER
	if(hier_count > TOPIC_HIERARCHY_LIMIT) return MOSQ_ERR_INVAL;
#endif

	return MOSQ_ERR_SUCCESS;
}


void db__msg_store_add(struct mosquitto_db *db, struct mosquitto_msg_store *store)
{
	store->next = db->msg_store;
	store->prev = NULL;
	if(db->msg_store){
		db->msg_store->prev = store;
	}
	db->msg_store = store;
}


int sub__messages_queue(struct mosquitto_db *db, const char *source_id, const char *topic, int qos, int retain, struct mosquitto_msg_store **stored)
{
	if(retain){
		last_retained = (*stored)->db_id;
	}
	return MOSQ_ERR_SUCCESS;
}

/* This function requires topic to be allocated on the heap. Once called, it owns topic and will free it on error. Likewise payload and properties. */
int db__message_store(struct mosquitto_db *db, const struct mosquitto *source, uint16_t source_mid, char *topic, int qos, uint32_t payloadlen, mosquitto__payload_uhpa *payload, int retain, struct mosquitto_msg_store **stored, uint32_t message_expiry_interval, mosquitto_property *properties, dbid_t store_id, enum mosquitto_msg_origin origin)
{
	struct mosquitto_msg_store *temp = NULL;
	int rc = MOSQ_ERR_SUCCESS;

	assert(db);
	assert(stored);

	temp = mosquitto__calloc(1, sizeof(struct mosquitto_msg_store));
	if(!temp){
		// log__printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
		rc = MOSQ_ERR_NOMEM;
		goto error;
	}

	temp->topic = NULL;
	temp->payload.ptr = NULL;

	temp->ref_count = 0;
	if(source && source->id){
		temp->source_id = mosquitto__strdup(source->id);
	}else{
		temp->source_id = mosquitto__strdup("");
	}
	if(!temp->source_id){
		// log__printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
		rc = MOSQ_ERR_NOMEM;
		goto error;
	}

	if(source && source->username){
		temp->source_username = mosquitto__strdup(source->username);
		if(!temp->source_username){
			rc = MOSQ_ERR_NOMEM;
			goto error;
		}
	}
	if(source){
		temp->source_listener = source->listener;
	}
	temp->source_mid = source_mid;
	temp->mid = 0;
	temp->qos = qos;
	temp->retain = retain;
	temp->topic = topic;
	topic = NULL;
	temp->payloadlen = payloadlen;
	temp->properties = properties;
	temp->origin = origin;
	if(payloadlen){
		UHPA_MOVE(temp->payload, *payload, payloadlen);
	}else{
		temp->payload.ptr = NULL;
	}
	if(message_expiry_interval > 0){
		temp->message_expiry_time = time(NULL) + message_expiry_interval;
	}else{
		temp->message_expiry_time = 0;
	}

	temp->dest_ids = NULL;
	temp->dest_id_count = 0;
	db->msg_store_count++;
	db->msg_store_bytes += payloadlen;
	(*stored) = temp;

	if(!store_id){
		temp->db_id = ++db->last_db_id;
	}else{
		temp->db_id = store_id;
	}

	db__msg_store_add(db, temp);

	return MOSQ_ERR_SUCCESS;
error:
	mosquitto__free(topic);
	if(temp){
		mosquitto__free(temp->source_id);
		mosquitto__free(temp->source_username);
		mosquitto__free(temp->topic);
		mosquitto__free(temp);
	}
	mosquitto_property_free_all(&properties);
	UHPA_FREE(*payload, payloadlen);
	return rc;
}


int db__messages_easy_queue(struct mosquitto_db *db, struct mosquitto *context, const char *topic, int qos, uint32_t payloadlen, const void *payload, int retain, uint32_t message_expiry_interval, mosquitto_property **properties)
{
	struct mosquitto_msg_store *stored;
	char *source_id;
	char *topic_heap;
	mosquitto__payload_uhpa payload_uhpa;
	mosquitto_property *local_properties = NULL;
	enum mosquitto_msg_origin origin;

	assert(db);

	payload_uhpa.ptr = NULL;

	if(!topic) return MOSQ_ERR_INVAL;
	topic_heap = mosquitto__strdup(topic);
	if(!topic_heap) return MOSQ_ERR_INVAL;

	if(db->config->retain_available == false){
		retain = 0;
	}

	if(UHPA_ALLOC(payload_uhpa, payloadlen) == 0){
		mosquitto__free(topic_heap);
		return MOSQ_ERR_NOMEM;
	}
	memcpy(UHPA_ACCESS(payload_uhpa, payloadlen), payload, payloadlen);

	if(context && context->id){
		source_id = context->id;
	}else{
		source_id = "";
	}
	if(properties){
		local_properties = *properties;
		*properties = NULL;
	}

	if(context){
		origin = mosq_mo_client;
	}else{
		origin = mosq_mo_broker;
	}
	if(db__message_store(db, context, 0, topic_heap, qos, payloadlen, &payload_uhpa, retain, &stored, message_expiry_interval, local_properties, 0, origin)) return 1;

	return sub__messages_queue(db, source_id, topic_heap, qos, retain, &stored);
}

/* Close a socket associated with a context and set it to -1.
 * Returns 1 on failure (context is NULL)
 * Returns 0 on success.
 */
// #ifdef WITH_BROKER
int net__socket_close(struct mosquitto_db *db, struct mosquitto *mosq)
//	#else
//	int net__socket_close(struct mosquitto *mosq)
//	#endif
{
	int rc = 0;
	assert(mosq);

	if(mosq->sock != INVALID_SOCKET){
#ifdef WITH_BROKER
		HASH_DELETE(hh_sock, db->contexts_by_sock, mosq);
#endif
		rc = COMPAT_CLOSE(mosq->sock);
		mosq->sock = INVALID_SOCKET;
	}
#ifdef WITH_BROKER
	if(mosq->listener){
		mosq->listener->client_count--;
	}
#endif

	return rc;
}


int will__clear(struct mosquitto *mosq)
{
	if(!mosq->will) return MOSQ_ERR_SUCCESS;

	mosquitto__free(mosq->will->msg.topic);
	mosq->will->msg.topic = NULL;

	mosquitto__free(mosq->will->msg.payload);
	mosq->will->msg.payload = NULL;

	mosquitto_property_free_all(&mosq->will->properties);

	mosquitto__free(mosq->will);
	mosq->will = NULL;

	return MOSQ_ERR_SUCCESS;
}

void context__send_will(struct mosquitto_db *db, struct mosquitto *ctxt)
{
	if(ctxt->state != mosq_cs_disconnecting && ctxt->will){
		if(ctxt->will_delay_interval > 0){
			will_delay__add(ctxt);
			return;
		}

		if(mosquitto_acl_check(db, ctxt,
					ctxt->will->msg.topic,
					ctxt->will->msg.payloadlen,
					ctxt->will->msg.payload,
					ctxt->will->msg.qos,
					ctxt->will->msg.retain,
					MOSQ_ACL_WRITE) == MOSQ_ERR_SUCCESS){

			/* Unexpected disconnect, queue the client will. */
			db__messages_easy_queue(db, ctxt,
					ctxt->will->msg.topic,
					ctxt->will->msg.qos,
					ctxt->will->msg.payloadlen,
					ctxt->will->msg.payload,
					ctxt->will->msg.retain,
					ctxt->will->expiry_interval,
					&ctxt->will->properties);
		}
	}
	will__clear(ctxt);
}

void context__remove_from_by_id(struct mosquitto_db *db, struct mosquitto *context)
{
	if(context->removed_from_by_id == false && context->id){
		HASH_DELETE(hh_id, db->contexts_by_id, context);
		context->removed_from_by_id = true;
	}
}

void context__add_to_disused(struct mosquitto_db *db, struct mosquitto *context)
{
	if(context->state == mosq_cs_disused) return;

	mosquitto__set_state(context, mosq_cs_disused);

	if(context->id){
		context__remove_from_by_id(db, context);
		mosquitto__free(context->id);
		context->id = NULL;
	}

	context->for_free_next = db->ll_for_free;
	db->ll_for_free = context;
}


static int session_expiry__cmp(struct session_expiry_list *i1, struct session_expiry_list *i2)
{
	if(i1->context->session_expiry_time == i2->context->session_expiry_time){
		return 0;
	}else if(i1->context->session_expiry_time > i2->context->session_expiry_time){
		return 1;
	}else{
		return -1;
	}
}


int session_expiry__add(struct mosquitto_db *db, struct mosquitto *context)
{
	struct session_expiry_list *item;

	if(db->config->persistent_client_expiration == 0){
		if(context->session_expiry_interval == UINT32_MAX){
			/* There isn't a global expiry set, and the client has asked to
			 * never expire, so we don't add it to the list. */
			return MOSQ_ERR_SUCCESS;
		}
	}

	item = mosquitto__calloc(1, sizeof(struct session_expiry_list));
	if(!item) return MOSQ_ERR_NOMEM;

	item->context = context;
	item->context->session_expiry_time = time(NULL);

	if(db->config->persistent_client_expiration == 0){
		/* No global expiry, so use the client expiration interval */
		item->context->session_expiry_time += item->context->session_expiry_interval;
	}else{
		/* We have a global expiry interval */
		if(db->config->persistent_client_expiration < item->context->session_expiry_interval){
			/* The client expiry is longer than the global expiry, so use the global */
			item->context->session_expiry_time += db->config->persistent_client_expiration;
		}else{
			/* The global expiry is longer than the client expiry, so use the client */
			item->context->session_expiry_time += item->context->session_expiry_interval;
		}
	}
	context->expiry_list_item = item;

	DL_INSERT_INORDER(expiry_list, item, session_expiry__cmp);

	return MOSQ_ERR_SUCCESS;
}



void context__disconnect(struct mosquitto_db *db, struct mosquitto *context)
{
	if(mosquitto__get_state(context) == mosq_cs_disconnected){
		return;
	}

	net__socket_close(db, context);

	context__send_will(db, context);
	if(context->session_expiry_interval == 0){
		/* Client session is due to be expired now */
#ifdef WITH_BRIDGE
		if(!context->bridge)
#endif
		{

			if(context->will_delay_interval == 0){
				/* This will be done later, after the will is published for delay>0. */
				context__add_to_disused(db, context);
			}
		}
	}else{
		session_expiry__add(db, context);
	}
	mosquitto__set_state(context, mosq_cs_disconnected);
}

int mosquitto_unpwd_check_default(struct mosquitto_db *db, struct mosquitto *context, const char *username, const char *password)
{
	struct mosquitto__unpwd *u, *tmp;
	struct mosquitto__unpwd *unpwd_ref;

	if(!db) return MOSQ_ERR_INVAL;

	if(db->config->per_listener_settings){
		if(context->bridge) return MOSQ_ERR_SUCCESS;
		if(!context->listener) return MOSQ_ERR_INVAL;
		if(context->listener->security_options.password_file == NULL) return MOSQ_ERR_PLUGIN_DEFER;
		unpwd_ref = context->listener->unpwd;
	}else{
		if(db->config->security_options.password_file == NULL) return MOSQ_ERR_PLUGIN_DEFER;
		unpwd_ref = db->unpwd;
	}
	if(!username){
		/* Check must be made only after checking unpwd_ref.
		 * This is DENY here, because in MQTT v5 username can be missing when
		 * password is present, but we don't support that. */
		return MOSQ_ERR_AUTH;
	}

	HASH_ITER(hh, unpwd_ref, u, tmp){
		if(!strcmp(u->username, username)){
			if(u->password){
				if(password){
					if(!strcmp(u->password, password)){
						return MOSQ_ERR_SUCCESS;
					}
				}else{
					return MOSQ_ERR_AUTH;
				}
			}else{
				return MOSQ_ERR_SUCCESS;
			}
		}
	}

	return MOSQ_ERR_AUTH;
}
		
int mosquitto_unpwd_check(struct mosquitto_db *db, struct mosquitto *context, const char *username, const char *password)
{
	int rc;
	int i;
	struct mosquitto__security_options *opts;

	rc = mosquitto_unpwd_check_default(db, context, username, password);
	if(rc != MOSQ_ERR_PLUGIN_DEFER){
		return rc;
	}
	/* Default check has accepted or deferred at this point.
	 * If no plugins exist we should accept at this point so set rc to success.
	 */
	if(db->config->per_listener_settings){
		opts = &context->listener->security_options;
	}else{
		opts = &db->config->security_options;
	}

	rc = MOSQ_ERR_SUCCESS;
	for(i=0; i<opts->auth_plugin_config_count; i++){
		if(opts->auth_plugin_configs[i].plugin.version == 4 
				&& opts->auth_plugin_configs[i].plugin.unpwd_check_v4){

			rc = opts->auth_plugin_configs[i].plugin.unpwd_check_v4(
					opts->auth_plugin_configs[i].plugin.user_data,
					context,
					username,
					password);

		}else if(opts->auth_plugin_configs[i].plugin.version == 3){
			rc = opts->auth_plugin_configs[i].plugin.unpwd_check_v3(
					opts->auth_plugin_configs[i].plugin.user_data,
					context,
					username,
					password);

		}else if(opts->auth_plugin_configs[i].plugin.version == 2){
			rc = opts->auth_plugin_configs[i].plugin.unpwd_check_v2(
					opts->auth_plugin_configs[i].plugin.user_data,
					username,
					password);
		}else{
			rc = MOSQ_ERR_INVAL;
		}
		if(rc != MOSQ_ERR_PLUGIN_DEFER){
			return rc;
		}
	}
	/* If all plugins deferred, this is a denial. If rc == MOSQ_ERR_SUCCESS
	 * here, then no plugins were configured. */
	if(rc == MOSQ_ERR_PLUGIN_DEFER){
		rc = MOSQ_ERR_AUTH;
	}
	return rc;
}

/* clientId 生成 */
static char nibble_to_hex(uint8_t value)
{
	if(value < 0x0A){
		return '0'+value;
	}else{
		return 'A'+value-0x0A;
	}
}

int util__random_bytes(void *bytes, int count)
{
	int rc = MOSQ_ERR_UNKNOWN;

//  #ifdef WITH_TLS
//  	if(RAND_bytes(bytes, count) == 1){
//  		rc = MOSQ_ERR_SUCCESS;
//  	}
//  #elif defined(HAVE_GETRANDOM)
//  	if(getrandom(bytes, count, 0) == count){
//  		rc = MOSQ_ERR_SUCCESS;
//  	}
//  #elif defined(WIN32)
//  	HCRYPTPROV provider;
//  
//  	if(!CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
//  		return MOSQ_ERR_UNKNOWN;
//  	}
//  
//  	if(CryptGenRandom(provider, count, bytes)){
//  		rc = MOSQ_ERR_SUCCESS;
//  	}
//  
//  	CryptReleaseContext(provider, 0);
//  #else
//    	int i;
//    
//    	for(i=0; i<count; i++){
//    		((uint8_t *)bytes)[i] = (uint8_t )(random()&0xFF);
//    	}
  	rc = MOSQ_ERR_SUCCESS;
//  #endif
	return rc;
}

static char *client_id_gen(int *idlen, const char *auto_id_prefix, int auto_id_prefix_len)
{
	char *client_id;
	uint8_t rnd[16];
	int i;
	int pos;

	if(util__random_bytes(rnd, 16)) return NULL;

	*idlen = 36 + auto_id_prefix_len;

	client_id = (char *)mosquitto__calloc((*idlen) + 1, sizeof(char));
	if(!client_id){
		return NULL;
	}
	if(auto_id_prefix){
		memcpy(client_id, auto_id_prefix, auto_id_prefix_len);
	}

	pos = 0;
	for(i=0; i<16; i++){
		client_id[auto_id_prefix_len + pos + 0] = nibble_to_hex(rnd[i] & 0x0F);
		client_id[auto_id_prefix_len + pos + 1] = nibble_to_hex((rnd[i] >> 4) & 0x0F);
		pos += 2;
		if(pos == 8 || pos == 13 || pos == 18 || pos == 23){
			client_id[auto_id_prefix_len + pos] = '-';
			pos++;
		}
	}

	return client_id;
}


static int will__read(struct mosquitto *context, struct mosquitto_message_all **will, uint8_t will_qos, int will_retain)
{
	int rc = MOSQ_ERR_SUCCESS;
	int slen;
	struct mosquitto_message_all *will_struct = NULL;
	char *will_topic_mount = NULL;
	uint16_t payloadlen;
	mosquitto_property *properties = NULL;

	will_struct = mosquitto__calloc(1, sizeof(struct mosquitto_message_all));
	if(!will_struct){
		rc = MOSQ_ERR_NOMEM;
		goto error_cleanup;
	}
	if(context->protocol == PROTOCOL_VERSION_v5){
		rc = property__read_all(CMD_WILL, &context->in_packet, &properties);
		if(rc) goto error_cleanup;

		rc = property__process_will(context, will_struct, &properties);
		mosquitto_property_free_all(&properties);
		if(rc) goto error_cleanup;
	}
	rc = packet__read_string(&context->in_packet, &will_struct->msg.topic, &slen);
	if(rc) goto error_cleanup;
	if(!slen){
		rc = MOSQ_ERR_PROTOCOL;
		goto error_cleanup;
	}

	if(context->listener->mount_point){
		slen = strlen(context->listener->mount_point) + strlen(will_struct->msg.topic) + 1;
		will_topic_mount = mosquitto__malloc(slen+1);
		if(!will_topic_mount){
			rc = MOSQ_ERR_NOMEM;
			goto error_cleanup;
		}

		snprintf(will_topic_mount, slen, "%s%s", context->listener->mount_point, will_struct->msg.topic);
		will_topic_mount[slen] = '\0';

		mosquitto__free(will_struct->msg.topic);
		will_struct->msg.topic = will_topic_mount;
	}

	rc = mosquitto_pub_topic_check(will_struct->msg.topic);
	if(rc) goto error_cleanup;

	rc = packet__read_uint16(&context->in_packet, &payloadlen);
	if(rc) goto error_cleanup;

	will_struct->msg.payloadlen = payloadlen;
	if(will_struct->msg.payloadlen > 0){
		will_struct->msg.payload = mosquitto__malloc(will_struct->msg.payloadlen);
		if(!will_struct->msg.payload){
			rc = MOSQ_ERR_NOMEM;
			goto error_cleanup;
		}

		rc = packet__read_bytes(&context->in_packet, will_struct->msg.payload, will_struct->msg.payloadlen);
		if(rc) goto error_cleanup;
	}

	will_struct->msg.qos = will_qos;
	will_struct->msg.retain = will_retain;

	*will = will_struct;
	return MOSQ_ERR_SUCCESS;

error_cleanup:
	if(will_struct){
		mosquitto__free(will_struct->msg.topic);
		mosquitto__free(will_struct->msg.payload);
		mosquitto_property_free_all(&will_struct->properties);
		mosquitto__free(will_struct);
	}
	return rc;
}



int mosquitto_security_auth_start(struct mosquitto_db *db, struct mosquitto *context, bool reauth, const void *data_in, uint16_t data_in_len, void **data_out, uint16_t *data_out_len)
{
	int rc = MOSQ_ERR_PLUGIN_DEFER;
	int i;
	struct mosquitto__security_options *opts;

	if(!context || !context->listener || !context->auth_method) return MOSQ_ERR_INVAL;
	if(!data_out || !data_out_len) return MOSQ_ERR_INVAL;

	if(db->config->per_listener_settings){
		opts = &context->listener->security_options;
	}else{
		opts = &db->config->security_options;
	}

	for(i=0; i<opts->auth_plugin_config_count; i++){
		if(opts->auth_plugin_configs[i].plugin.auth_start_v4){
			*data_out = NULL;
			*data_out_len = 0;

			rc = opts->auth_plugin_configs[i].plugin.auth_start_v4(
					opts->auth_plugin_configs[i].plugin.user_data,
					context,
					context->auth_method,
					reauth,
					data_in, data_in_len,
					data_out, data_out_len);

			if(rc == MOSQ_ERR_SUCCESS){
				return MOSQ_ERR_SUCCESS;
			}else if(rc == MOSQ_ERR_AUTH_CONTINUE){
				return MOSQ_ERR_AUTH_CONTINUE;
			}else if(rc != MOSQ_ERR_NOT_SUPPORTED){
				return rc;
			}
		}
	}

	return MOSQ_ERR_NOT_SUPPORTED;
}

void db__msg_store_remove(struct mosquitto_db *db, struct mosquitto_msg_store *store)
{
	int i;

	if(store->prev){
		store->prev->next = store->next;
		if(store->next){
			store->next->prev = store->prev;
		}
	}else{
		db->msg_store = store->next;
		if(store->next){
			store->next->prev = NULL;
		}
	}
	db->msg_store_count--;
	db->msg_store_bytes -= store->payloadlen;

	mosquitto__free(store->source_id);
	mosquitto__free(store->source_username);
	if(store->dest_ids){
		for(i=0; i<store->dest_id_count; i++){
			mosquitto__free(store->dest_ids[i]);
		}
		mosquitto__free(store->dest_ids);
	}
	mosquitto__free(store->topic);
	mosquitto_property_free_all(&store->properties);
	UHPA_FREE_PAYLOAD(store);
	mosquitto__free(store);
}

void util__decrement_receive_quota(struct mosquitto *mosq)
{
	if(mosq->msgs_in.inflight_quota > 0){
		mosq->msgs_in.inflight_quota--;
	}
}

void db__msg_store_ref_dec(struct mosquitto_db *db, struct mosquitto_msg_store **store)
{
	(*store)->ref_count--;
	if((*store)->ref_count == 0){
		db__msg_store_remove(db, *store);
		*store = NULL;
	}
}

static void db__message_remove(struct mosquitto_db *db, struct mosquitto_msg_data *msg_data, struct mosquitto_client_msg *item)
{
	if(!msg_data || !item){
		return;
	}

	DL_DELETE(msg_data->inflight, item);
	if(item->store){
		msg_data->msg_count--;
		msg_data->msg_bytes -= item->store->payloadlen;
		if(item->qos > 0){
			msg_data->msg_count12--;
			msg_data->msg_bytes12 -= item->store->payloadlen;
		}
		db__msg_store_ref_dec(db, &item->store);
	}

	mosquitto_property_free_all(&item->properties);
	mosquitto__free(item);
}


void db__message_dequeue_first(struct mosquitto *context, struct mosquitto_msg_data *msg_data)
{
	struct mosquitto_client_msg *msg;

	msg = msg_data->queued;
	DL_DELETE(msg_data->queued, msg);
	DL_APPEND(msg_data->inflight, msg);
	if(msg_data->inflight_quota > 0){
		msg_data->inflight_quota--;
	}
}

/**
 * Is this context ready to take more in flight messages right now?
 * @param context the client context of interest
 * @param qos qos for the packet of interest
 * @return true if more in flight are allowed.
 */
static bool db__ready_for_flight(struct mosquitto_msg_data *msgs, int qos)
{
	bool valid_bytes;
	bool valid_count;

	if(qos == 0 || (msgs->inflight_maximum == 0 && max_inflight_bytes == 0)){
		return true;
	}

	valid_bytes = msgs->msg_bytes12 < max_inflight_bytes;
	valid_count = msgs->inflight_quota > 0;

	if(msgs->inflight_maximum == 0){
		return valid_bytes;
	}
	if(max_inflight_bytes == 0){
		return valid_count;
	}

	return valid_bytes && valid_count;
}

/* Called on reconnect to set outgoing messages to a sensible state and force a
 * retry, and to set incoming messages to expect an appropriate retry. */
int db__message_reconnect_reset_outgoing(struct mosquitto_db *db, struct mosquitto *context)
{
	struct mosquitto_client_msg *msg, *tmp;

	context->msgs_out.msg_bytes = 0;
	context->msgs_out.msg_bytes12 = 0;
	context->msgs_out.msg_count = 0;
	context->msgs_out.msg_count12 = 0;
	context->msgs_out.inflight_quota = context->msgs_out.inflight_maximum;

	DL_FOREACH_SAFE(context->msgs_out.inflight, msg, tmp){
		context->msgs_out.msg_count++;
		context->msgs_out.msg_bytes += msg->store->payloadlen;
		if(msg->qos > 0){
			context->msgs_out.msg_count12++;
			context->msgs_out.msg_bytes12 += msg->store->payloadlen;
			util__decrement_receive_quota(context);
		}

		switch(msg->qos){
			case 0:
				msg->state = mosq_ms_publish_qos0;
				break;
			case 1:
				msg->state = mosq_ms_publish_qos1;
				break;
			case 2:
				if(msg->state == mosq_ms_wait_for_pubcomp){
					msg->state = mosq_ms_resend_pubrel;
				}else{
					msg->state = mosq_ms_publish_qos2;
				}
				break;
		}
	}
	/* Messages received when the client was disconnected are put
	 * in the mosq_ms_queued state. If we don't change them to the
	 * appropriate "publish" state, then the queued messages won't
	 * get sent until the client next receives a message - and they
	 * will be sent out of order.
	 */
	DL_FOREACH_SAFE(context->msgs_out.queued, msg, tmp){
		context->msgs_out.msg_count++;
		context->msgs_out.msg_bytes += msg->store->payloadlen;
		if(msg->qos > 0){
			context->msgs_out.msg_count12++;
			context->msgs_out.msg_bytes12 += msg->store->payloadlen;
		}
		if(db__ready_for_flight(&context->msgs_out, msg->qos)){
			switch(msg->qos){
				case 0:
					msg->state = mosq_ms_publish_qos0;
					break;
				case 1:
					msg->state = mosq_ms_publish_qos1;
					break;
				case 2:
					msg->state = mosq_ms_publish_qos2;
					break;
			}
			db__message_dequeue_first(context, &context->msgs_out);
		}
	}

	return MOSQ_ERR_SUCCESS;
}


/* Called on reconnect to set incoming messages to expect an appropriate retry. */
int db__message_reconnect_reset_incoming(struct mosquitto_db *db, struct mosquitto *context)
{
	struct mosquitto_client_msg *msg, *tmp;

	context->msgs_in.msg_bytes = 0;
	context->msgs_in.msg_bytes12 = 0;
	context->msgs_in.msg_count = 0;
	context->msgs_in.msg_count12 = 0;
	context->msgs_in.inflight_quota = context->msgs_in.inflight_maximum;

	DL_FOREACH_SAFE(context->msgs_in.inflight, msg, tmp){
		context->msgs_in.msg_count++;
		context->msgs_in.msg_bytes += msg->store->payloadlen;
		if(msg->qos > 0){
			context->msgs_in.msg_count12++;
			context->msgs_in.msg_bytes12 += msg->store->payloadlen;
			util__decrement_receive_quota(context);
		}

		if(msg->qos != 2){
			/* Anything <QoS 2 can be completely retried by the client at
			 * no harm. */
			db__message_remove(db, &context->msgs_in, msg);
		}else{
			/* Message state can be preserved here because it should match
			 * whatever the client has got. */
		}
	}

	/* Messages received when the client was disconnected are put
	 * in the mosq_ms_queued state. If we don't change them to the
	 * appropriate "publish" state, then the queued messages won't
	 * get sent until the client next receives a message - and they
	 * will be sent out of order.
	 */
	DL_FOREACH_SAFE(context->msgs_in.queued, msg, tmp){
		context->msgs_in.msg_count++;
		context->msgs_in.msg_bytes += msg->store->payloadlen;
		if(msg->qos > 0){
			context->msgs_in.msg_count12++;
			context->msgs_in.msg_bytes12 += msg->store->payloadlen;
		}
		if(db__ready_for_flight(&context->msgs_in, msg->qos)){
			switch(msg->qos){
				case 0:
					msg->state = mosq_ms_publish_qos0;
					break;
				case 1:
					msg->state = mosq_ms_publish_qos1;
					break;
				case 2:
					msg->state = mosq_ms_publish_qos2;
					break;
			}
			db__message_dequeue_first(context, &context->msgs_in);
		}
	}

	return MOSQ_ERR_SUCCESS;
}


int db__message_reconnect_reset(struct mosquitto_db *db, struct mosquitto *context)
{
	int rc;

	rc = db__message_reconnect_reset_outgoing(db, context);
	if(rc) return rc;
	return db__message_reconnect_reset_incoming(db, context);
}


static void sub__remove_shared_leaf(struct mosquitto__subhier *subhier, struct mosquitto__subshared *shared, struct mosquitto__subleaf *leaf)
{
	DL_DELETE(shared->subs, leaf);
	if(shared->subs == NULL){
		HASH_DELETE(hh, subhier->shared, shared);
		mosquitto__free(shared->name);
		mosquitto__free(shared);
	}
	mosquitto__free(leaf);
}

/* Remove a subhier element, and return its parent if that needs freeing as well. */
static struct mosquitto__subhier *tmp_remove_subs(struct mosquitto__subhier *sub)
{
	struct mosquitto__subhier *parent;

	if(!sub || !sub->parent){
		return NULL;
	}

	if(sub->children || sub->subs || sub->retained){
		return NULL;
	}

	parent = sub->parent;
	HASH_DELETE(hh, parent->children, sub);
	mosquitto__free(sub->topic);
	mosquitto__free(sub);

	if(parent->subs == NULL
			&& parent->children == NULL
			&& parent->retained == NULL
			&& parent->shared == NULL
			&& parent->parent){

		return parent;
	}else{
		return NULL;
	}
}


static int sub__clean_session_shared(struct mosquitto_db *db, struct mosquitto *context)
{
	int i;
	struct mosquitto__subleaf *leaf;
	struct mosquitto__subhier *hier;

	for(i=0; i<context->shared_sub_count; i++){
		if(context->shared_subs[i] == NULL){
			continue;
		}
		leaf = context->shared_subs[i]->shared->subs;
		while(leaf){
			if(leaf->context==context){
#ifdef WITH_SYS_TREE
				db->shared_subscription_count--;
#endif
				sub__remove_shared_leaf(context->shared_subs[i]->hier, context->shared_subs[i]->shared, leaf);
				break;
			}
			leaf = leaf->next;
		}
		if(context->shared_subs[i]->hier->subs == NULL
				&& context->shared_subs[i]->hier->children == NULL
				&& context->shared_subs[i]->hier->retained == NULL
				&& context->shared_subs[i]->hier->shared == NULL
				&& context->shared_subs[i]->hier->parent){

			hier = context->shared_subs[i]->hier;
			context->shared_subs[i]->hier = NULL;
			do{
				hier = tmp_remove_subs(hier);
			}while(hier);
		}
		mosquitto__free(context->shared_subs[i]);
	}
	mosquitto__free(context->shared_subs);
	context->shared_subs = NULL;
	context->shared_sub_count = 0;

	return MOSQ_ERR_SUCCESS;
}

/* Remove all subscriptions for a client.
 */
int sub__clean_session(struct mosquitto_db *db, struct mosquitto *context)
{
	int i;
	struct mosquitto__subleaf *leaf;
	struct mosquitto__subhier *hier;

	for(i=0; i<context->sub_count; i++){
		if(context->subs[i] == NULL){
			continue;
		}
		leaf = context->subs[i]->subs;
		while(leaf){
			if(leaf->context==context){
#ifdef WITH_SYS_TREE
				db->subscription_count--;
#endif
				DL_DELETE(context->subs[i]->subs, leaf);
				mosquitto__free(leaf);
				break;
			}
			leaf = leaf->next;
		}
		if(context->subs[i]->subs == NULL
				&& context->subs[i]->children == NULL
				&& context->subs[i]->retained == NULL
				&& context->subs[i]->shared == NULL
				&& context->subs[i]->parent){

			hier = context->subs[i];
			context->subs[i] = NULL;
			do{
				hier = tmp_remove_subs(hier);
			}while(hier);
		}
	}
	mosquitto__free(context->subs);
	context->subs = NULL;
	context->sub_count = 0;

	return sub__clean_session_shared(db, context);
}


void session_expiry__remove(struct mosquitto *context)
{
	if(context->expiry_list_item){
		DL_DELETE(expiry_list, context->expiry_list_item);
		mosquitto__free(context->expiry_list_item);
		context->expiry_list_item = NULL;
	}
}


void will_delay__remove(struct mosquitto *mosq)
{
	if(mosq->will_delay_entry != NULL){
		DL_DELETE(delay_list, mosq->will_delay_entry);
		mosquitto__free(mosq->will_delay_entry);
		mosq->will_delay_entry = NULL;
	}
}

void do_disconnect(struct mosquitto_db *db, struct mosquitto *context, int reason)
{
	char *id;
#ifdef WITH_EPOLL
	struct epoll_event ev;
#endif
#ifdef WITH_WEBSOCKETS
	bool is_duplicate = false;
#endif

	if(context->state == mosq_cs_disconnected){
		return;
	}
#ifdef WITH_WEBSOCKETS
	if(context->wsi){
		if(context->state == mosq_cs_duplicate){
			is_duplicate = true;
		}

		if(context->state != mosq_cs_disconnecting && context->state != mosq_cs_disconnect_with_will){
			mosquitto__set_state(context, mosq_cs_disconnect_ws);
		}
		if(context->wsi){
			libwebsocket_callback_on_writable(context->ws_context, context->wsi);
		}
		if(context->sock != INVALID_SOCKET){
			HASH_DELETE(hh_sock, db->contexts_by_sock, context);
#ifdef WITH_EPOLL
			if (epoll_ctl(db->epollfd, EPOLL_CTL_DEL, context->sock, &ev) == -1) {
				log__printf(NULL, MOSQ_LOG_DEBUG, "Error in epoll disconnecting websockets: %s", strerror(errno));
			}
#endif		
			context->sock = INVALID_SOCKET;
			context->pollfd_index = -1;
		}
		if(is_duplicate){
			/* This occurs if another client is taking over the same client id.
			 * It is important to remove this from the by_id hash here, so it
			 * doesn't leave us with multiple clients in the hash with the same
			 * id. Websockets doesn't actually close the connection here,
			 * unlike for normal clients, which means there is extra time when
			 * there could be two clients with the same id in the hash. */
			context__remove_from_by_id(db, context);
		}
	}else
#endif
	{
		if(db->config->connection_messages == true){
			if(context->id){
				id = context->id;
			}else{
				id = "<unknown>";
			}
			if(context->state != mosq_cs_disconnecting && context->state != mosq_cs_disconnect_with_will){
				switch(reason){
					case MOSQ_ERR_SUCCESS:
						break;
					case MOSQ_ERR_PROTOCOL:
						//	log__printf(NULL, MOSQ_LOG_NOTICE, "Client %s disconnected due to protocol error.", id);
						break;
					case MOSQ_ERR_CONN_LOST:
						// log__printf(NULL, MOSQ_LOG_NOTICE, "Socket error on client %s, disconnecting.", id);
						break;
					case MOSQ_ERR_AUTH:
						// log__printf(NULL, MOSQ_LOG_NOTICE, "Client %s disconnected, no longer authorised.", id);
						break;
					case MOSQ_ERR_KEEPALIVE:
						// log__printf(NULL, MOSQ_LOG_NOTICE, "Client %s has exceeded timeout, disconnecting.", id);
						break;
					default:
						//	log__printf(NULL, MOSQ_LOG_NOTICE, "Socket error on client %s, disconnecting.", id);
						break;
				}
			}else{
				// log__printf(NULL, MOSQ_LOG_NOTICE, "Client %s disconnected.", id);
			}
		}
#ifdef WITH_EPOLL
		if (context->sock != INVALID_SOCKET && epoll_ctl(db->epollfd, EPOLL_CTL_DEL, context->sock, &ev) == -1) {
			if(db->config->connection_messages == true){
				log__printf(NULL, MOSQ_LOG_DEBUG, "Error in epoll disconnecting: %s", strerror(errno));
			}
		}
#endif		
		context__disconnect(db, context);
	}
}


/* Remove any queued messages that are no longer allowed through ACL,
 * assuming a possible change of username. */
void connection_check_acl(struct mosquitto_db *db, struct mosquitto *context, struct mosquitto_client_msg **head)
{
	struct mosquitto_client_msg *msg_tail, *tmp;

	DL_FOREACH_SAFE((*head), msg_tail, tmp){
		if(msg_tail->direction == mosq_md_out){
			if(mosquitto_acl_check(db, context, msg_tail->store->topic,
								   msg_tail->store->payloadlen, UHPA_ACCESS(msg_tail->store->payload, msg_tail->store->payloadlen),
								   msg_tail->store->qos, msg_tail->store->retain, MOSQ_ACL_READ) != MOSQ_ERR_SUCCESS){

				DL_DELETE((*head), msg_tail);
				db__msg_store_ref_dec(db, &msg_tail->store);
				mosquitto_property_free_all(&msg_tail->properties);
				mosquitto__free(msg_tail);
			}
		}
	}
}

int acl__find_acls(struct mosquitto_db *db, struct mosquitto *context)
{
	struct mosquitto__acl_user *acl_tail;
	struct mosquitto__security_options *security_opts;

	/* Associate user with its ACL, assuming we have ACLs loaded. */
	if(db->config->per_listener_settings){
		if(!context->listener){
			return MOSQ_ERR_INVAL;
		}
		security_opts = &context->listener->security_options;
	}else{
		security_opts = &db->config->security_options;
	}

	if(security_opts->acl_list){
		acl_tail = security_opts->acl_list;
		while(acl_tail){
			if(context->username){
				if(acl_tail->username && !strcmp(context->username, acl_tail->username)){
					context->acl_list = acl_tail;
					break;
				}
			}else{
				if(acl_tail->username == NULL){
					context->acl_list = acl_tail;
					break;
				}
			}
			acl_tail = acl_tail->next;
		}
	}else{
		context->acl_list = NULL;
	}

	return MOSQ_ERR_SUCCESS;
}

int connect__on_authorised(struct mosquitto_db *db, struct mosquitto *context, void *auth_data_out, uint16_t auth_data_out_len)
{
	struct mosquitto *found_context;
	struct mosquitto__subleaf *leaf;
	mosquitto_property *connack_props = NULL;
	uint8_t connect_ack = 0;
	int i;
	int rc;

	/* Find if this client already has an entry. This must be done *after* any security checks. */
	HASH_FIND(hh_id, db->contexts_by_id, context->id, strlen(context->id), found_context);
	if(found_context){
		/* Found a matching client */
		if(found_context->sock == INVALID_SOCKET){
			/* Client is reconnecting after a disconnect */
			/* FIXME - does anything need to be done here? */
		}else{
			/* Client is already connected, disconnect old version. This is
			 * done in context__cleanup() below. */
			if(db->config->connection_messages == true){
				// log__printf(NULL, MOSQ_LOG_ERR, "Client %s already connected, closing old connection.", context->id);
			}
		}

		if(context->clean_start == false && found_context->session_expiry_interval > 0){
			if(context->protocol == mosq_p_mqtt311 || context->protocol == mosq_p_mqtt5){
				connect_ack |= 0x01;
			}

			if(found_context->msgs_in.inflight || found_context->msgs_in.queued
					|| found_context->msgs_out.inflight || found_context->msgs_out.queued){

				memcpy(&context->msgs_in, &found_context->msgs_in, sizeof(struct mosquitto_msg_data));
				memcpy(&context->msgs_out, &found_context->msgs_out, sizeof(struct mosquitto_msg_data));

				memset(&found_context->msgs_in, 0, sizeof(struct mosquitto_msg_data));
				memset(&found_context->msgs_out, 0, sizeof(struct mosquitto_msg_data));

				db__message_reconnect_reset(db, context);
			}
			context->subs = found_context->subs;
			found_context->subs = NULL;
			context->sub_count = found_context->sub_count;
			found_context->sub_count = 0;
			context->last_mid = found_context->last_mid;

			for(i=0; i<context->sub_count; i++){
				if(context->subs[i]){
					leaf = context->subs[i]->subs;
					while(leaf){
						if(leaf->context == found_context){
							leaf->context = context;
						}
						leaf = leaf->next;
					}
				}
			}
		}

		if(context->clean_start == true){
			sub__clean_session(db, found_context);
		}
		session_expiry__remove(found_context);
		will_delay__remove(found_context);
		will__clear(found_context);

		found_context->clean_start = true;
		found_context->session_expiry_interval = 0;
		mosquitto__set_state(found_context, mosq_cs_duplicate);
		do_disconnect(db, found_context, MOSQ_ERR_SUCCESS);
	}

	rc = acl__find_acls(db, context);
	if(rc){
		free(auth_data_out);
		return rc;
	}

	if(db->config->connection_messages == true){
		if(context->is_bridge){
			if(context->username){
				//	log__printf(NULL, MOSQ_LOG_NOTICE, "New bridge connected from %s as %s (p%d, c%d, k%d, u'%s').",
				//			context->address, context->id, context->protocol, context->clean_start, context->keepalive, context->username);
			}else{
				//	log__printf(NULL, MOSQ_LOG_NOTICE, "New bridge connected from %s as %s (p%d, c%d, k%d).",
				//			context->address, context->id, context->protocol, context->clean_start, context->keepalive);
			}
		}else{
			if(context->username){
				//	log__printf(NULL, MOSQ_LOG_NOTICE, "New client connected from %s as %s (p%d, c%d, k%d, u'%s').",
				//			context->address, context->id, context->protocol, context->clean_start, context->keepalive, context->username);
			}else{
				//	log__printf(NULL, MOSQ_LOG_NOTICE, "New client connected from %s as %s (p%d, c%d, k%d).",
				//			context->address, context->id, context->protocol, context->clean_start, context->keepalive);
			}
		}

		if(context->will) {
		//	log__printf(NULL, MOSQ_LOG_DEBUG, "Will message specified (%ld bytes) (r%d, q%d).",
		//			(long)context->will->msg.payloadlen,
		//			context->will->msg.retain,
		//			context->will->msg.qos);

			//	log__printf(NULL, MOSQ_LOG_DEBUG, "\t%s", context->will->msg.topic);
		} else {
			//	log__printf(NULL, MOSQ_LOG_DEBUG, "No will message specified.");
		}
	}

	context->ping_t = 0;
	context->is_dropping = false;

	connection_check_acl(db, context, &context->msgs_in.inflight);
	connection_check_acl(db, context, &context->msgs_in.queued);
	connection_check_acl(db, context, &context->msgs_out.inflight);
	connection_check_acl(db, context, &context->msgs_out.queued);

	HASH_ADD_KEYPTR(hh_id, db->contexts_by_id, context->id, strlen(context->id), context);

#ifdef WITH_PERSISTENCE
	if(!context->clean_start){
		db->persistence_changes++;
	}
#endif
	context->maximum_qos = context->listener->maximum_qos;

	if(context->protocol == mosq_p_mqtt5){
		if(context->maximum_qos != 2){
			if(mosquitto_property_add_byte(&connack_props, MQTT_PROP_MAXIMUM_QOS, context->maximum_qos)){
				rc = MOSQ_ERR_NOMEM;
				goto error;
			}
		}
		if(context->listener->max_topic_alias > 0){
			if(mosquitto_property_add_int16(&connack_props, MQTT_PROP_TOPIC_ALIAS_MAXIMUM, context->listener->max_topic_alias)){
				rc = MOSQ_ERR_NOMEM;
				goto error;
			}
		}
		if(context->keepalive > db->config->max_keepalive){
			context->keepalive = db->config->max_keepalive;
			if(mosquitto_property_add_int16(&connack_props, MQTT_PROP_SERVER_KEEP_ALIVE, context->keepalive)){
				rc = MOSQ_ERR_NOMEM;
				goto error;
			}
		}
		if(context->assigned_id){
			if(mosquitto_property_add_string(&connack_props, MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER, context->id)){
				rc = MOSQ_ERR_NOMEM;
				goto error;
			}
		}
		if(context->auth_method){
			if(mosquitto_property_add_string(&connack_props, MQTT_PROP_AUTHENTICATION_METHOD, context->auth_method)){
				rc = MOSQ_ERR_NOMEM;
				goto error;
			}

			if(auth_data_out && auth_data_out_len > 0){
				if(mosquitto_property_add_binary(&connack_props, MQTT_PROP_AUTHENTICATION_DATA, auth_data_out, auth_data_out_len)){
					rc = MOSQ_ERR_NOMEM;
					goto error;
				}
			}
		}
	}
	free(auth_data_out);

	mosquitto__set_state(context, mosq_cs_active);
	rc = send__connack(db, context, connect_ack, CONNACK_ACCEPTED, connack_props);
	mosquitto_property_free_all(&connack_props);
	return rc;
error:
	free(auth_data_out);
	mosquitto_property_free_all(&connack_props);
	return rc;
}

int handle__connect(struct mosquitto_db *db, struct mosquitto *context)
{
    
	char protocol_name[7];
	uint8_t protocol_version;
	uint8_t connect_flags;
	char *client_id = NULL;
	struct mosquitto_message_all *will_struct = NULL;
	uint8_t will, will_retain, will_qos, clean_start;
	uint8_t username_flag, password_flag;
	char *username = NULL, *password = NULL;
	int rc;
	int slen;
	uint16_t slen16;
	mosquitto_property *properties = NULL;
	void *auth_data = NULL;
	uint16_t auth_data_len = 0;
	void *auth_data_out = NULL;
	uint16_t auth_data_out_len = 0;
	bool allow_zero_length_clientid;


	if(!context->listener){
		return MOSQ_ERR_INVAL;
	}

	/* Don't accept multiple CONNECT commands. */
	if(context->state != mosq_cs_new){
		// log__printf(NULL, MOSQ_LOG_NOTICE, "Bad client %s sending multiple CONNECT messages.", context->id);
        printf( "Bad client %s sending multiple CONNECT messages.", context->id);
		rc = MOSQ_ERR_PROTOCOL;
		goto handle_connect_error;
	}
    
	/* Read protocol name as length then bytes rather than with read_string
	 * because the length is fixed and we can check that. Removes the need
	 * for another malloc as well. */
    /* 获取协议名字长度 */ 
	if(packet__read_uint16(&context->in_packet, &slen16)){
		rc = 1;
		goto handle_connect_error;
	}
	slen = slen16;
	if(slen != 4 /* MQTT */ && slen != 6 /* MQIsdp */){
		rc = MOSQ_ERR_PROTOCOL;
		goto handle_connect_error;
	}
    /* 读取协议名 */
	if(packet__read_bytes(&context->in_packet, protocol_name, slen)){
		rc = MOSQ_ERR_PROTOCOL;
		goto handle_connect_error;
	}
	protocol_name[slen] = '\0';

	if(packet__read_byte(&context->in_packet, &protocol_version)){
		rc = 1;
		goto handle_connect_error;
	}

    /* 协议名字和版本判断 */
	if(!strcmp(protocol_name, PROTOCOL_NAME_v31)){
		if((protocol_version&0x7F) != PROTOCOL_VERSION_v31){
	   	    if(db->config->connection_messages == true){
                printf("Invalid protocol version %d in CONNECT from %s.", protocol_version, context->address);
	//  			log__printf(NULL, MOSQ_LOG_INFO, "Invalid protocol version %d in CONNECT from %s.",
	//  					protocol_version, context->address);
	  		}
	  		send__connack(db, context, 0, CONNACK_REFUSED_PROTOCOL_VERSION, NULL);
			rc = MOSQ_ERR_PROTOCOL;
			goto handle_connect_error;
		}
		context->protocol = mosq_p_mqtt31;
		if((protocol_version&0x80) == 0x80){
			context->is_bridge = true;
		}
	}else if(!strcmp(protocol_name, PROTOCOL_NAME)){
		if((protocol_version&0x7F) == PROTOCOL_VERSION_v311){
			context->protocol = mosq_p_mqtt311;

			if((protocol_version&0x80) == 0x80){
				context->is_bridge = true;
			}
		}else if((protocol_version&0x7F) == PROTOCOL_VERSION_v5){
			context->protocol = mosq_p_mqtt5;
		}else{
			if(db->config->connection_messages == true){
				//  log__printf(NULL, MOSQ_LOG_INFO, "Invalid protocol version %d in CONNECT from %s.",
				//  		protocol_version, context->address);
			}
	        send__connack(db, context, 0, CONNACK_REFUSED_PROTOCOL_VERSION, NULL);
			rc = MOSQ_ERR_PROTOCOL;
			goto handle_connect_error;
		}
		if((context->in_packet.command&0x0F) != 0x00){
			/* Reserved flags not set to 0, must disconnect. */
			rc = MOSQ_ERR_PROTOCOL;
			goto handle_connect_error;
		}
	}else{
		if(db->config->connection_messages == true){
		//  	log__printf(NULL, MOSQ_LOG_INFO, "Invalid protocol \"%s\" in CONNECT from %s.",
		//  			protocol_name, context->address);
		}
		rc = MOSQ_ERR_PROTOCOL;
		goto handle_connect_error;
	}

	/* check reversed bit */
	if(packet__read_byte(&context->in_packet, &connect_flags)){
		rc = 1;
		goto handle_connect_error;
	}
	if(context->protocol == mosq_p_mqtt311 || context->protocol == mosq_p_mqtt5){
		if((connect_flags & 0x01) != 0x00){
			rc = MOSQ_ERR_PROTOCOL;
			goto handle_connect_error;
		}
	}


	clean_start = (connect_flags & 0x02) >> 1;
	/* session_expiry_interval will be overriden if the properties are read later */
	if(clean_start == false && protocol_version != PROTOCOL_VERSION_v5){
		/* v3* has clean_start == false mean the session never expires */
		context->session_expiry_interval = UINT32_MAX;
	}else{
		context->session_expiry_interval = 0;
	}
	will = connect_flags & 0x04;
	will_qos = (connect_flags & 0x18) >> 3;
	if(will_qos == 3){
		//  log__printf(NULL, MOSQ_LOG_INFO, "Invalid Will QoS in CONNECT from %s.",
		//  		context->address);
		rc = MOSQ_ERR_PROTOCOL;
		goto handle_connect_error;
	}
	will_retain = ((connect_flags & 0x20) == 0x20);
	password_flag = connect_flags & 0x40;
	username_flag = connect_flags & 0x80;

	if(will && will_retain && db->config->retain_available == false){
		if(protocol_version == mosq_p_mqtt5){
			send__connack(db, context, 0, MQTT_RC_RETAIN_NOT_SUPPORTED, NULL);
		}
		rc = 1;
		goto handle_connect_error;
	}

	if(packet__read_uint16(&context->in_packet, &(context->keepalive))){
		rc = 1;
		goto handle_connect_error;
	}

    /* 属性处理 */
  	if(protocol_version == PROTOCOL_VERSION_v5){
  		rc = property__read_all(CMD_CONNECT, &context->in_packet, &properties);
  		if(rc) goto handle_connect_error;
  	}

	property__process_connect(context, &properties);

	if(mosquitto_property_read_string(properties, MQTT_PROP_AUTHENTICATION_METHOD, &context->auth_method, false)){
		mosquitto_property_read_binary(properties, MQTT_PROP_AUTHENTICATION_DATA, &auth_data, &auth_data_len, false);
	}

	mosquitto_property_free_all(&properties); /* FIXME - TEMPORARY UNTIL PROPERTIES PROCESSED */

	if(packet__read_string(&context->in_packet, &client_id, &slen)){
		rc = 1;
		goto handle_connect_error;
	}

	if(slen == 0){
		if(context->protocol == mosq_p_mqtt31){
			send__connack(db, context, 0, CONNACK_REFUSED_IDENTIFIER_REJECTED, NULL);
			rc = MOSQ_ERR_PROTOCOL;
			goto handle_connect_error;
		}else{ /* mqtt311/mqtt5 */
			mosquitto__free(client_id);
			client_id = NULL;

			if(db->config->per_listener_settings){
				allow_zero_length_clientid = context->listener->security_options.allow_zero_length_clientid;
			}else{
				allow_zero_length_clientid = db->config->security_options.allow_zero_length_clientid;
			}
			if((context->protocol == mosq_p_mqtt311 && clean_start == 0) || allow_zero_length_clientid == false){
				if(context->protocol == mosq_p_mqtt311){
				   	send__connack(db, context, 0, CONNACK_REFUSED_IDENTIFIER_REJECTED, NULL);
				}else{
				  	send__connack(db, context, 0, MQTT_RC_UNSPECIFIED, NULL);
				}
				rc = MOSQ_ERR_PROTOCOL;
				goto handle_connect_error;
			}else{
				if(db->config->per_listener_settings){
					client_id = client_id_gen(&slen, context->listener->security_options.auto_id_prefix, context->listener->security_options.auto_id_prefix_len);
				}else{
					client_id = client_id_gen(&slen, db->config->security_options.auto_id_prefix, db->config->security_options.auto_id_prefix_len);
				}
				if(!client_id){
					rc = MOSQ_ERR_NOMEM;
					goto handle_connect_error;
				}
				context->assigned_id = true;
			}
		}
	}


	/* clientid_prefixes check */
	if(db->config->clientid_prefixes){
		if(strncmp(db->config->clientid_prefixes, client_id, strlen(db->config->clientid_prefixes))){
			if(context->protocol == mosq_p_mqtt5){
				send__connack(db, context, 0, MQTT_RC_NOT_AUTHORIZED, NULL);
			}else{
				send__connack(db, context, 0, CONNACK_REFUSED_NOT_AUTHORIZED, NULL);
			}
			rc = 1;
			goto handle_connect_error;
		}
	}

	if(will){
		rc = will__read(context, &will_struct, will_qos, will_retain);
		if(rc) goto handle_connect_error;
	}else{
		if(context->protocol == mosq_p_mqtt311 || context->protocol == mosq_p_mqtt5){
			if(will_qos != 0 || will_retain != 0){
				rc = MOSQ_ERR_PROTOCOL;
				goto handle_connect_error;
			}
		}
	}


	if(username_flag){
		rc = packet__read_string(&context->in_packet, &username, &slen);
		if(rc == MOSQ_ERR_NOMEM){
			rc = MOSQ_ERR_NOMEM;
			goto handle_connect_error;
		}else if(rc != MOSQ_ERR_SUCCESS){
			if(context->protocol == mosq_p_mqtt31){
				/* Username flag given, but no username. Ignore. */
				username_flag = 0;
			}else{
				rc = MOSQ_ERR_PROTOCOL;
				goto handle_connect_error;
			}
		}
	}else{
		if(context->protocol == mosq_p_mqtt311 || context->protocol == mosq_p_mqtt31){
			if(password_flag){
				/* username_flag == 0 && password_flag == 1 is forbidden */
				printf("Protocol error from %s: password without username, closing connection.", client_id);
				// log__printf(NULL, MOSQ_LOG_ERR, "Protocol error from %s: password without username, closing connection.", client_id);
				rc = MOSQ_ERR_PROTOCOL;
				goto handle_connect_error;
			}
		}
	}

	if(password_flag){
		rc = packet__read_binary(&context->in_packet, (uint8_t **)&password, &slen);
		if(rc == MOSQ_ERR_NOMEM){
			rc = MOSQ_ERR_NOMEM;
			goto handle_connect_error;
		}else if(rc == MOSQ_ERR_PROTOCOL){
			if(context->protocol == mosq_p_mqtt31){
				/* Password flag given, but no password. Ignore. */
			}else{
				rc = MOSQ_ERR_PROTOCOL;
				goto handle_connect_error;
			}
		}
	}

	if(context->in_packet.pos != context->in_packet.remaining_length){
		/* Surplus data at end of packet, this must be an error. */
		rc = MOSQ_ERR_PROTOCOL;
		goto handle_connect_error;
	}



	if(username_flag || password_flag){
		/* FIXME - these ensure the mosquitto_client_id() and
		 * mosquitto_client_username() functions work, but is hacky */
		context->id = client_id;
		context->username = username;
		rc = mosquitto_unpwd_check(db, context, username, password);
		context->username = NULL;
		context->id = NULL;
		switch(rc){
			case MOSQ_ERR_SUCCESS:
				break;
			case MOSQ_ERR_AUTH:
				if(context->protocol == mosq_p_mqtt5){
					send__connack(db, context, 0, MQTT_RC_NOT_AUTHORIZED, NULL);
				}else{
					send__connack(db, context, 0, CONNACK_REFUSED_NOT_AUTHORIZED, NULL);
				}
				context__disconnect(db, context);
				rc = 1;
				goto handle_connect_error;
				break;
			default:
				context__disconnect(db, context);
				rc = 1;
				goto handle_connect_error;
				break;
		}
		context->username = username;
		context->password = password;
		username = NULL; /* Avoid free() in error: below. */
		password = NULL;
	}else{
		if((db->config->per_listener_settings && context->listener->security_options.allow_anonymous == false)
				|| (!db->config->per_listener_settings && db->config->security_options.allow_anonymous == false)){

			if(context->protocol == mosq_p_mqtt5){
				send__connack(db, context, 0, MQTT_RC_NOT_AUTHORIZED, NULL);
			}else{
				send__connack(db, context, 0, CONNACK_REFUSED_NOT_AUTHORIZED, NULL);
			}
			rc = 1;
			goto handle_connect_error;
		}
	}

	if(context->listener->use_username_as_clientid){
		if(context->username){
			mosquitto__free(client_id);
			client_id = mosquitto__strdup(context->username);
			if(!client_id){
				rc = MOSQ_ERR_NOMEM;
				goto handle_connect_error;
			}
		}else{
			if(context->protocol == mosq_p_mqtt5){
				send__connack(db, context, 0, MQTT_RC_NOT_AUTHORIZED, NULL);
			}else{
				send__connack(db, context, 0, CONNACK_REFUSED_NOT_AUTHORIZED, NULL);
			}
			rc = 1;
			goto handle_connect_error;
		}
	}
	context->clean_start = clean_start;
	context->id = client_id;
	context->will = will_struct;

	if(context->auth_method){
		rc = mosquitto_security_auth_start(db, context, false, auth_data, auth_data_len, &auth_data_out, &auth_data_out_len);
		mosquitto__free(auth_data);
		if(rc == MOSQ_ERR_SUCCESS){
			return connect__on_authorised(db, context, auth_data_out, auth_data_out_len);
		}else if(rc == MOSQ_ERR_AUTH_CONTINUE){
			mosquitto__set_state(context, mosq_cs_authenticating);
			rc = send__auth(db, context, MQTT_RC_CONTINUE_AUTHENTICATION, auth_data_out, auth_data_out_len);
			free(auth_data_out);
			return rc;
		}else{
			free(auth_data_out);
			will__clear(context);
			if(rc == MOSQ_ERR_AUTH){
				send__connack(db, context, 0, MQTT_RC_NOT_AUTHORIZED, NULL);
				mosquitto__free(context->id);
				context->id = NULL;
				return MOSQ_ERR_PROTOCOL;
			}else if(rc == MOSQ_ERR_NOT_SUPPORTED){
				/* Client has requested extended authentication, but we don't support it. */
				send__connack(db, context, 0, MQTT_RC_BAD_AUTHENTICATION_METHOD, NULL);
				mosquitto__free(context->id);
				context->id = NULL;
				return MOSQ_ERR_PROTOCOL;
			}else{
				mosquitto__free(context->id);
				context->id = NULL;
				return rc;
			}
		}
	}else{
		return connect__on_authorised(db, context, NULL, 0);
	}

handle_connect_error:
	mosquitto__free(auth_data);
	mosquitto__free(client_id);
	mosquitto__free(username);
	mosquitto__free(password);
	if(will_struct){
		mosquitto_property_free_all(&will_struct->properties);
		mosquitto__free(will_struct->msg.payload);
		mosquitto__free(will_struct->msg.topic);
		mosquitto__free(will_struct);
	}

	/* We return an error here which means the client is freed later on. */
	context->clean_start = true;
	context->session_expiry_interval = 0;

	return rc;
}
