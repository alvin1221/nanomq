#ifndef HANDLE_CONNECT_H
#define HANDLE_CONNECT_H

#include "mqtt_protocol.h"
#include "mosquitto_base.h"

static unsigned long max_inflight_bytes = 0;

static int will_delay__cmp(struct will_delay_list *i1, struct will_delay_list
        *i2);

int will_delay__add(struct mosquitto *context);


/* Does a topic match a subscription? */
int mosquitto_topic_matches_sub2(const char *sub, size_t sublen, const char
        *topic, size_t topiclen, bool *result);

int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool
        *result);


int mosquitto_acl_check_default(struct mosquitto_db *db, struct mosquitto
        *context, const char *topic, int access);

static int acl__check_dollar(const char *topic, int access);


/* int mosquitto_acl_check(struct mosquitto_db *db, struct mosquitto *context, const char *topic, int access) */
static int acl__check_single(struct mosquitto__auth_plugin_config *auth_plugin,
        struct mosquitto *context, struct mosquitto_acl_msg *msg, int access);


int mosquitto_acl_check(struct mosquitto_db *db, struct mosquitto *context,
        const char *topic, long payloadlen, void* payload, int qos, bool retain,
        int access);

int mosquitto_pub_topic_check(const char *str);


void db__msg_store_add(struct mosquitto_db *db, struct mosquitto_msg_store
        *store);


int sub__messages_queue(struct mosquitto_db *db, const char *source_id, const
        char *topic, int qos, int retain, struct mosquitto_msg_store **stored);

/* This function requires topic to be allocated on the heap. Once called, it owns topic and will free it on error. Likewise payload and properties. */
int db__message_store(struct mosquitto_db *db, const struct mosquitto *source,
        uint16_t source_mid, char *topic, int qos, uint32_t payloadlen,
        mosquitto__payload_uhpa *payload, int retain, struct mosquitto_msg_store
        **stored, uint32_t message_expiry_interval, mosquitto_property
        *properties, dbid_t store_id, enum mosquitto_msg_origin origin);


int db__messages_easy_queue(struct mosquitto_db *db, struct mosquitto *context,
        const char *topic, int qos, uint32_t payloadlen, const void *payload,
        int retain, uint32_t message_expiry_interval, mosquitto_property
        **properties);

/* Close a socket associated with a context and set it to -1.
 * Returns 1 on failure (context is NULL)
 * Returns 0 on success.
 */
// #ifdef WITH_BROKER
int net__socket_close(struct mosquitto_db *db, struct mosquitto *mosq);
//	#else
//	int net__socket_close(struct mosquitto *mosq)
//	#endif


int will__clear(struct mosquitto *mosq);

void context__send_will(struct mosquitto_db *db, struct mosquitto *ctxt);

void context__remove_from_by_id(struct mosquitto_db *db, struct mosquitto
        *context);


static int session_expiry__cmp(struct session_expiry_list *i1, struct
        session_expiry_list *i2);


int session_expiry__add(struct mosquitto_db *db, struct mosquitto *context);



void context__disconnect(struct mosquitto_db *db, struct mosquitto *context);

int mosquitto_unpwd_check_default(struct mosquitto_db *db, struct mosquitto
        *context, const char *username, const char *password);
		
int mosquitto_unpwd_check(struct mosquitto_db *db, struct mosquitto *context,
        const char *username, const char *password);

/* clientId 生成 */
static char nibble_to_hex(uint8_t value);

int util__random_bytes(void *bytes, int count);

static char *client_id_gen(int *idlen, const char *auto_id_prefix, int
        auto_id_prefix_len);


static int will__read(struct mosquitto *context, struct mosquitto_message_all
        **will, uint8_t will_qos, int will_retain);



int mosquitto_security_auth_start(struct mosquitto_db *db, struct mosquitto
        *context, bool reauth, const void *data_in, uint16_t data_in_len, void
        **data_out, uint16_t *data_out_len);
void db__msg_store_remove(struct mosquitto_db *db, struct mosquitto_msg_store
        *store);

void util__decrement_receive_quota(struct mosquitto *mosq);

void db__msg_store_ref_dec(struct mosquitto_db *db, struct mosquitto_msg_store
        **store);

static void db__message_remove(struct mosquitto_db *db, struct
        mosquitto_msg_data *msg_data, struct mosquitto_client_msg *item);


void db__message_dequeue_first(struct mosquitto *context, struct
        mosquitto_msg_data *msg_data);

/**
 * Is this context ready to take more in flight messages right now?
 * @param context the client context of interest
 * @param qos qos for the packet of interest
 * @return true if more in flight are allowed.
 */
static bool db__ready_for_flight(struct mosquitto_msg_data *msgs, int qos);

/* Called on reconnect to set outgoing messages to a sensible state and force a
 * retry, and to set incoming messages to expect an appropriate retry. */
int db__message_reconnect_reset_outgoing(struct mosquitto_db *db, struct
        mosquitto *context);


/* Called on reconnect to set incoming messages to expect an appropriate retry. */
int db__message_reconnect_reset_incoming(struct mosquitto_db *db, struct
        mosquitto *context);


int db__message_reconnect_reset(struct mosquitto_db *db, struct mosquitto
        *context);


static void sub__remove_shared_leaf(struct mosquitto__subhier *subhier, struct
        mosquitto__subshared *shared, struct mosquitto__subleaf *leaf);

/* Remove a subhier element, and return its parent if that needs freeing as well. */
static struct mosquitto__subhier *tmp_remove_subs(struct mosquitto__subhier
        *sub);


static int sub__clean_session_shared(struct mosquitto_db *db, struct mosquitto
        *context);

/* Remove all subscriptions for a client.
 */
int sub__clean_session(struct mosquitto_db *db, struct mosquitto *context);


void session_expiry__remove(struct mosquitto *context);


void will_delay__remove(struct mosquitto *mosq);

void do_disconnect(struct mosquitto_db *db, struct mosquitto *context, int
        reason);


/* Remove any queued messages that are no longer allowed through ACL,
 * assuming a possible change of username. */
void connection_check_acl(struct mosquitto_db *db, struct mosquitto *context,
        struct mosquitto_client_msg **head);

int acl__find_acls(struct mosquitto_db *db, struct mosquitto *context);

int connect__on_authorised(struct mosquitto_db *db, struct mosquitto *context,
        void *auth_data_out, uint16_t auth_data_out_len);

int handle__connect(struct mosquitto_db *db, struct mosquitto *context);

#endif
