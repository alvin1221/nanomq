#ifndef MOSQUITTO_BASE_H
#define MOSQUITTO_BASE_H

#include <stdio.h>
#include <string.h>   /* memcmp,strlen */
#include <stddef.h>   /* ptrdiff_t */
#include <stdlib.h>   /* exit() */
#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include "uthash.h"
#include "utlist.h"
#include "uhpa.h"
/* Macros for accessing the MSB and LSB of a uint16_t */
#define MOSQ_MSB(A) (uint8_t)((A & 0xFF00) >> 8)
#define MOSQ_LSB(A) (uint8_t)(A & 0x00FF)

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define COMPAT_CLOSE(a) close(a)
#define MOSQ_ACL_NONE 0x00
#define MOSQ_ACL_READ 0x01
#define MOSQ_ACL_WRITE 0x02
#define MOSQ_ACL_SUBSCRIBE 0x04

#define UNUSED(A) (void)(A)


/* ========================================
 * UHPA data types
 * ======================================== */

/* See uhpa.h
 *
 * The idea here is that there is potentially a lot of wasted space (and time)
 * in malloc calls for frequent, small heap allocations. This can happen if
 * small payloads are used by clients or if individual topic elements are
 * small.
 *
 * In both cases, a struct is used that includes a void* or char* pointer to
 * point to the dynamically allocated memory used. To allocate and store a
 * single byte needs the size of the pointer (8 bytes on a 64 bit
 * architecture), the malloc overhead and the memory allocated itself (which
 * will often be larger than the memory requested, on 64 bit Linux this can be
 * a minimum of 24 bytes). To allocate and store 1 byte of heap memory we need
 * in this example 32 bytes.
 *
 * UHPA uses a union to either store data in an array, or to allocate memory on
 * the heap, depending on the size of the data being stored (this does mean
 * that the size of the data must always be known). Setting the size of the
 * array changes the point at which heap allocation starts. Using the example
 * above, this means that an array size of 32 bytes should not result in any
 * wasted space, and should be quicker as well. Certainly in the case of topic
 * elements (e.g. "bar" out of "foo/bar/baz") it is likely that an array size
 * of 32 bytes will mean that the majority of heap allocations are removed.
 *
 * You can change the size of MOSQ_PAYLOAD_UNION_SIZE and
 * MOSQ_TOPIC_ELEMENT_UNION_SIZE to change the size of the uhpa array used for
 * the payload (i.e. the published part of a message) and for topic elements
 * (e.g. "foo", "bar" or "baz" in the topic "foo/bar/baz"), and so control the
 * heap allocation threshold for these data types. You should look at your
 * application to decide what values to set, but don't set them too high
 * otherwise your overall memory usage will increase.
 *
 * You could use something like heaptrack
 * http://milianw.de/blog/heaptrack-a-heap-memory-profiler-for-linux to
 * profile heap allocations.
 *
 * I would suggest that values for MOSQ_PAYLOAD_UNION_SIZE and
 * MOSQ_TOPIC_UNION_SIZE that are equivalent to
 * sizeof(void*)+malloc_usable_size(malloc(1)) are a safe value that should
 * reduce calls to malloc without increasing memory usage at all.
 */
#define MOSQ_PAYLOAD_UNION_SIZE 8
typedef union {
	void *ptr;
	char array[MOSQ_PAYLOAD_UNION_SIZE];
} mosquitto__payload_uhpa;
#define UHPA_ALLOC_PAYLOAD(A) UHPA_ALLOC((A)->payload, (A)->payloadlen)
#define UHPA_ACCESS_PAYLOAD(A) UHPA_ACCESS((A)->payload, (A)->payloadlen)
#define UHPA_FREE_PAYLOAD(A) UHPA_FREE((A)->payload, (A)->payloadlen)
#define UHPA_MOVE_PAYLOAD(DEST, SRC) UHPA_MOVE((DEST)->payload, (SRC)->payload, (SRC)->payloadlen)



struct mosquitto;
static struct will_delay_list *delay_list = NULL;

struct mosquitto_opt {
	char *key;
	char *value;
};

struct mosquitto_auth_opt {
	char *key;
	char *value;
};

struct mosquitto_acl_msg {
	const char *topic;
	const void *payload;
	long payloadlen;
	int qos;
	bool retain;
};

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef signed char int8_t;
typedef int mosq_sock_t;
typedef uint64_t dbid_t;
typedef struct mqtt5__property mosquitto_property;

uint64_t last_retained;

typedef int (*FUNC_auth_plugin_init_v4)(void **, struct mosquitto_opt *, int);
typedef int (*FUNC_auth_plugin_cleanup_v4)(void *, struct mosquitto_opt *, int);
typedef int (*FUNC_auth_plugin_security_init_v4)(void *, struct mosquitto_opt *, int, bool);
typedef int (*FUNC_auth_plugin_security_cleanup_v4)(void *, struct mosquitto_opt *, int, bool);
typedef int (*FUNC_auth_plugin_acl_check_v4)(void *, int, struct mosquitto *, struct mosquitto_acl_msg *);
typedef int (*FUNC_auth_plugin_unpwd_check_v4)(void *, struct mosquitto *, const char *, const char *);
typedef int (*FUNC_auth_plugin_psk_key_get_v4)(void *, struct mosquitto *, const char *, const char *, char *, int);
typedef int (*FUNC_auth_plugin_auth_start_v4)(void *, struct mosquitto *, const char *, bool, const void *, uint16_t, void **, uint16_t *);
typedef int (*FUNC_auth_plugin_auth_continue_v4)(void *, struct mosquitto *, const char *, const void *, uint16_t, void **, uint16_t *);

typedef int (*FUNC_auth_plugin_init_v3)(void **, struct mosquitto_opt *, int);
typedef int (*FUNC_auth_plugin_cleanup_v3)(void *, struct mosquitto_opt *, int);
typedef int (*FUNC_auth_plugin_security_init_v3)(void *, struct mosquitto_opt *, int, bool);
typedef int (*FUNC_auth_plugin_security_cleanup_v3)(void *, struct mosquitto_opt *, int, bool);
typedef int (*FUNC_auth_plugin_acl_check_v3)(void *, int, const struct mosquitto *, struct mosquitto_acl_msg *);
typedef int (*FUNC_auth_plugin_unpwd_check_v3)(void *, const struct mosquitto *, const char *, const char *);
typedef int (*FUNC_auth_plugin_psk_key_get_v3)(void *, const struct mosquitto *, const char *, const char *, char *, int);

typedef int (*FUNC_auth_plugin_init_v2)(void **, struct mosquitto_auth_opt *, int);
typedef int (*FUNC_auth_plugin_cleanup_v2)(void *, struct mosquitto_auth_opt *, int);
typedef int (*FUNC_auth_plugin_security_init_v2)(void *, struct mosquitto_auth_opt *, int, bool);
typedef int (*FUNC_auth_plugin_security_cleanup_v2)(void *, struct mosquitto_auth_opt *, int, bool);
typedef int (*FUNC_auth_plugin_acl_check_v2)(void *, const char *, const char *, const char *, int);
typedef int (*FUNC_auth_plugin_unpwd_check_v2)(void *, const char *, const char *);
typedef int (*FUNC_auth_plugin_psk_key_get_v2)(void *, const char *, const char *, char *, int);


enum mosquitto_msg_direction {
	mosq_md_in = 0,
	mosq_md_out = 1
};


enum mosquitto_msg_origin{
	mosq_mo_client = 0,
	mosq_mo_broker = 1
};

/* Error values */
enum mosq_err_t {
	MOSQ_ERR_AUTH_CONTINUE = -4,
	MOSQ_ERR_NO_SUBSCRIBERS = -3,
	MOSQ_ERR_SUB_EXISTS = -2,
	MOSQ_ERR_CONN_PENDING = -1,
	MOSQ_ERR_SUCCESS = 0,
	MOSQ_ERR_NOMEM = 1,
	MOSQ_ERR_PROTOCOL = 2,
	MOSQ_ERR_INVAL = 3,
	MOSQ_ERR_NO_CONN = 4,
	MOSQ_ERR_CONN_REFUSED = 5,
	MOSQ_ERR_NOT_FOUND = 6,
	MOSQ_ERR_CONN_LOST = 7,
	MOSQ_ERR_TLS = 8,
	MOSQ_ERR_PAYLOAD_SIZE = 9,
	MOSQ_ERR_NOT_SUPPORTED = 10,
	MOSQ_ERR_AUTH = 11,
	MOSQ_ERR_ACL_DENIED = 12,
	MOSQ_ERR_UNKNOWN = 13,
	MOSQ_ERR_ERRNO = 14,
	MOSQ_ERR_EAI = 15,
	MOSQ_ERR_PROXY = 16,
	MOSQ_ERR_PLUGIN_DEFER = 17,
	MOSQ_ERR_MALFORMED_UTF8 = 18,
	MOSQ_ERR_KEEPALIVE = 19,
	MOSQ_ERR_LOOKUP = 20,
	MOSQ_ERR_MALFORMED_PACKET = 21,
	MOSQ_ERR_DUPLICATE_PROPERTY = 22,
	MOSQ_ERR_TLS_HANDSHAKE = 23,
	MOSQ_ERR_QOS_NOT_SUPPORTED = 24,
	MOSQ_ERR_OVERSIZE_PACKET = 25,
	MOSQ_ERR_OCSP = 26,
};

enum mosquitto__protocol {
	mosq_p_invalid = 0,
	mosq_p_mqtt31 = 1,
	mosq_p_mqtt311 = 2,
	mosq_p_mqtts = 3,
	mosq_p_mqtt5 = 5,
};

enum mosquitto_msg_state {
	mosq_ms_invalid = 0,
	mosq_ms_publish_qos0 = 1,
	mosq_ms_publish_qos1 = 2,
	mosq_ms_wait_for_puback = 3,
	mosq_ms_publish_qos2 = 4,
	mosq_ms_wait_for_pubrec = 5,
	mosq_ms_resend_pubrel = 6,
	mosq_ms_wait_for_pubrel = 7,
	mosq_ms_resend_pubcomp = 8,
	mosq_ms_wait_for_pubcomp = 9,
	mosq_ms_send_pubrec = 10,
	mosq_ms_queued = 11
};


enum mosquitto_client_state {
	mosq_cs_new = 0,
	mosq_cs_connected = 1,
	mosq_cs_disconnecting = 2,
	mosq_cs_active = 3,
	mosq_cs_connect_pending = 4,
	mosq_cs_connect_srv = 5,
	mosq_cs_disconnect_ws = 6,
	mosq_cs_disconnected = 7,
	mosq_cs_socks5_new = 8,
	mosq_cs_socks5_start = 9,
	mosq_cs_socks5_request = 10,
	mosq_cs_socks5_reply = 11,
	mosq_cs_socks5_auth_ok = 12,
	mosq_cs_socks5_userpass_reply = 13,
	mosq_cs_socks5_send_userpass = 14,
	mosq_cs_expiring = 15,
	mosq_cs_duplicate = 17, /* client that has been taken over by another with the same id */
	mosq_cs_disconnect_with_will = 18,
	mosq_cs_disused = 19, /* client that has been added to the disused list to be freed */
	mosq_cs_authenticating = 20, /* Client has sent CONNECT but is still undergoing extended authentication */
	mosq_cs_reauthenticating = 21, /* Client is undergoing reauthentication and shouldn't do anything else until complete */
};


static struct session_expiry_list *expiry_list = NULL;

struct session_expiry_list {
	struct mosquitto *context;
	struct session_expiry_list *prev;
	struct session_expiry_list *next;
};

struct mosquitto_message{
	int mid;
	char *topic;
	void *payload;
	int payloadlen;
	int qos;
	bool retain;
};

struct mqtt__string {
	char *v;
	int len;
};

struct mqtt5__property {
	struct mqtt5__property *next;
	union {
		uint8_t i8;
		uint16_t i16;
		uint32_t i32;
		uint32_t varint;
		struct mqtt__string bin;
		struct mqtt__string s;
	} value;
	struct mqtt__string name;
	int32_t identifier;
	bool client_generated;
};

struct mosquitto__packet{
	uint8_t *payload;
	struct mosquitto__packet *next;
	uint32_t remaining_mult;
	uint32_t remaining_length;
	uint32_t packet_length;
	uint32_t to_process;
	uint32_t pos;
	uint16_t mid;
	uint8_t command;
	int8_t remaining_count;
};

struct mosquitto_message_all{
	struct mosquitto_message_all *next;
	struct mosquitto_message_all *prev;
    mosquitto_property *properties;
	time_t timestamp;
	enum mosquitto_msg_state state;
	bool dup;
    struct mosquitto_message msg;
	uint32_t expiry_interval;
};

struct mosquitto__alias{
	char *topic;
	uint16_t alias;
};

struct will_delay_list {
	struct mosquitto *context;
	struct will_delay_list *prev;
	struct will_delay_list *next;
};


enum mosquitto__bridge_direction{
	bd_out = 0,
	bd_in = 1,
	bd_both = 2
};

enum mosquitto_bridge_start_type{
	bst_automatic = 0,
	bst_lazy = 1,
	bst_manual = 2,
	bst_once = 3
};

struct mosquitto__bridge_topic{
	char *topic;
	int qos;
	enum mosquitto__bridge_direction direction;
	char *local_prefix;
	char *remote_prefix;
	char *local_topic; /* topic prefixed with local_prefix */
	char *remote_topic; /* topic prefixed with remote_prefix */
};

struct bridge_address{
	char *address;
	int port;
};

struct mosquitto__bridge{
	char *name;
	struct bridge_address *addresses;
	int cur_address;
	int address_count;
	time_t primary_retry;
	mosq_sock_t primary_retry_sock;
	bool round_robin;
	bool try_private;
	bool try_private_accepted;
	bool clean_start;
	int keepalive;
	struct mosquitto__bridge_topic *topics;
	int topic_count;
	bool topic_remapping;
	enum mosquitto__protocol protocol_version;
	time_t restart_t;
	char *remote_clientid;
	char *remote_username;
	char *remote_password;
	char *local_clientid;
	char *local_username;
	char *local_password;
	char *notification_topic;
	bool notifications;
	bool notifications_local_only;
	enum mosquitto_bridge_start_type start_type;
	int idle_timeout;
	int restart_timeout;
	int backoff_base;
	int backoff_cap;
	int threshold;
	bool lazy_reconnect;
	bool attempt_unsubscribe;
	bool initial_notification_done;
//  #ifdef WITH_TLS
//  	bool tls_insecure;
//  	bool tls_ocsp_required;
//  	char *tls_cafile;
//  	char *tls_capath;
//  	char *tls_certfile;
//  	char *tls_keyfile;
//  	char *tls_version;
//  	char *tls_alpn;
//  #  ifdef FINAL_WITH_TLS_PSK
//  	char *tls_psk_identity;
//  	char *tls_psk;
//  #  endif
//  #endif
};

struct mosquitto__acl{
	struct mosquitto__acl *next;
	char *topic;
	int access;
	int ucount;
	int ccount;
};

struct mosquitto__acl_user{
	struct mosquitto__acl_user *next;
	char *username;
	struct mosquitto__acl *acl;
};

struct mosquitto__auth_plugin{
	void *lib;
	void *user_data;
	int (*plugin_version)(void);

	FUNC_auth_plugin_init_v4 plugin_init_v4;
	FUNC_auth_plugin_cleanup_v4 plugin_cleanup_v4;
	FUNC_auth_plugin_security_init_v4 security_init_v4;
	FUNC_auth_plugin_security_cleanup_v4 security_cleanup_v4;
	FUNC_auth_plugin_acl_check_v4 acl_check_v4;
	FUNC_auth_plugin_unpwd_check_v4 unpwd_check_v4;
	FUNC_auth_plugin_psk_key_get_v4 psk_key_get_v4;
	FUNC_auth_plugin_auth_start_v4 auth_start_v4;
	FUNC_auth_plugin_auth_continue_v4 auth_continue_v4;

	FUNC_auth_plugin_init_v3 plugin_init_v3;
	FUNC_auth_plugin_cleanup_v3 plugin_cleanup_v3;
	FUNC_auth_plugin_security_init_v3 security_init_v3;
	FUNC_auth_plugin_security_cleanup_v3 security_cleanup_v3;
	FUNC_auth_plugin_acl_check_v3 acl_check_v3;
	FUNC_auth_plugin_unpwd_check_v3 unpwd_check_v3;
	FUNC_auth_plugin_psk_key_get_v3 psk_key_get_v3;

	FUNC_auth_plugin_init_v2 plugin_init_v2;
	FUNC_auth_plugin_cleanup_v2 plugin_cleanup_v2;
	FUNC_auth_plugin_security_init_v2 security_init_v2;
	FUNC_auth_plugin_security_cleanup_v2 security_cleanup_v2;
	FUNC_auth_plugin_acl_check_v2 acl_check_v2;
	FUNC_auth_plugin_unpwd_check_v2 unpwd_check_v2;
	FUNC_auth_plugin_psk_key_get_v2 psk_key_get_v2;
	int version;
};

 struct mosquitto__auth_plugin_config
{
	char *path;
	struct mosquitto_opt *options;
	int option_count;
	bool deny_special_chars;

	struct mosquitto__auth_plugin plugin;
};

struct mosquitto__security_options {
	/* Any options that get added here also need considering
	 * in config__read() with regards whether allow_anonymous
	 * should be disabled when these options are set.
	 */
	struct mosquitto__acl_user *acl_list;
	struct mosquitto__acl *acl_patterns;
	char *password_file;
	char *psk_file;
	char *acl_file;
	struct mosquitto__auth_plugin_config *auth_plugin_configs;
	int auth_plugin_config_count;
	int8_t allow_anonymous;
	bool allow_zero_length_clientid;
	char *auto_id_prefix;
	int auto_id_prefix_len;
};


enum mosquitto_protocol {
	mp_mqtt,
	mp_mqttsn,
	mp_websockets
};


struct mosquitto__listener {
	int fd;
	uint16_t port;
	char *host;
	char *bind_interface;
	int max_connections;
	char *mount_point;
	mosq_sock_t *socks;
	int sock_count;
	int client_count;
	enum mosquitto_protocol protocol;
	int socket_domain;
	bool use_username_as_clientid;
	uint8_t maximum_qos;
	uint16_t max_topic_alias;
//  #ifdef WITH_TLS
//  	char *cafile;
//  	char *capath;
//  	char *certfile;
//  	char *keyfile;
//  	char *tls_engine;
//  	char *tls_engine_kpass_sha1;
//  	char *ciphers;
//  	char *psk_hint;
//  	SSL_CTX *ssl_ctx;
//  	char *crlfile;
//  	char *tls_version;
//  	char *dhparamfile;
//  	bool use_identity_as_username;
//  	bool use_subject_as_username;
//  	bool require_certificate;
//  	enum mosquitto__keyform tls_keyform;
//  #endif
//  #ifdef WITH_WEBSOCKETS
//  	struct libwebsocket_context *ws_context;
//  	char *http_dir;
//  	struct libwebsocket_protocols *ws_protocol;
//  #endif
	struct mosquitto__security_options security_options;
	struct mosquitto__unpwd *unpwd;
	struct mosquitto__unpwd *psk_id;
};


struct mosquitto_msg_data{
//  #ifdef WITH_BROKER
	struct mosquitto_client_msg *inflight;
	struct mosquitto_client_msg *queued;
	unsigned long msg_bytes;
	unsigned long msg_bytes12;
	int msg_count;
	int msg_count12;
//  #else
//  	struct mosquitto_message_all *inflight;
//  	int queue_len;
//  #  ifdef WITH_THREADING
//  	pthread_mutex_t mutex;
//  #  endif
//  #endif
	int inflight_quota;
	uint16_t inflight_maximum;
};

struct mosquitto__subshared_ref {
	struct mosquitto__subhier *hier;
	struct mosquitto__subshared *shared;
};


struct mosquitto {
	mosq_sock_t sock;
// #ifndef WITH_BROKER
    mosq_sock_t sockpairR, sockpairW;
// #endif
//  #if defined(__GLIBC__) && defined(WITH_ADNS)
//  	struct gaicb *adns; /* For getaddrinfo_a */
//  #endif
	enum mosquitto__protocol protocol;
	char *address;
	char *id;
	char *username;
	char *password;
	uint16_t keepalive;
	uint16_t last_mid;
	enum mosquitto_client_state state;
	time_t last_msg_in;
	time_t next_msg_out;
	time_t ping_t;
	struct mosquitto__packet in_packet;
	struct mosquitto__packet *current_out_packet;
	struct mosquitto__packet *out_packet;
	struct mosquitto_message_all *will;
	struct mosquitto__alias *aliases;
	struct will_delay_list *will_delay_entry;
	uint32_t maximum_packet_size;
	int alias_count;
	uint32_t will_delay_interval;
	time_t will_delay_time;
	bool want_write;
	bool want_connect;
//  #if defined(WITH_THREADING) && !defined(WITH_BROKER)
  	pthread_mutex_t callback_mutex;
  	pthread_mutex_t log_callback_mutex;
  	pthread_mutex_t msgtime_mutex;
  	pthread_mutex_t out_packet_mutex;
  	pthread_mutex_t current_out_packet_mutex;
  	pthread_mutex_t state_mutex;
  	pthread_mutex_t mid_mutex;
  	pthread_t thread_id;
// #endif
	bool clean_start;
	uint32_t session_expiry_interval;
	time_t session_expiry_time;
//  #ifdef WITH_BROKER
  	bool removed_from_by_id; /* True if removed from by_id hash */
  	bool is_dropping;
  	bool is_bridge;
  	struct mosquitto__bridge *bridge;
  	struct mosquitto_msg_data msgs_in;
  	struct mosquitto_msg_data msgs_out;
  	struct mosquitto__acl_user *acl_list;
  	struct mosquitto__listener *listener;
  	//  struct mosquitto__packet *out_packet_last;
  	struct mosquitto__subhier **subs;
  	struct mosquitto__subshared_ref **shared_subs;
  	char *auth_method;
  	int sub_count;
  	int shared_sub_count;
  	int pollfd_index;
//  #  ifdef WITH_WEBSOCKETS
//  #    if defined(LWS_LIBRARY_VERSION_NUMBER)
//  	struct lws *wsi;
//  #    else
//  	struct libwebsocket_context *ws_context;
//  	struct libwebsocket *wsi;
//  #    endif
//  #  endif
	bool ws_want_write;
	bool assigned_id;
//  #else
//  #  ifdef WITH_SOCKS
//  	char *socks5_host;
//  	int socks5_port;
//  	char *socks5_username;
//  	char *socks5_password;
//  #  endif
  	void *userdata;
  	bool in_callback;
  	//  struct mosquitto_msg_data msgs_in;
  	//  struct mosquitto_msg_data msgs_out;
  	void (*on_connect)(struct mosquitto *, void *userdata, int rc);
  	void (*on_connect_with_flags)(struct mosquitto *, void *userdata, int rc, int flags);
  	void (*on_connect_v5)(struct mosquitto *, void *userdata, int rc, int flags, const mosquitto_property *props);
  	void (*on_disconnect)(struct mosquitto *, void *userdata, int rc);
  	void (*on_disconnect_v5)(struct mosquitto *, void *userdata, int rc, const mosquitto_property *props);
  	void (*on_publish)(struct mosquitto *, void *userdata, int mid);
  	void (*on_publish_v5)(struct mosquitto *, void *userdata, int mid, int reason_code, const mosquitto_property *props);
  	void (*on_message)(struct mosquitto *, void *userdata, const struct mosquitto_message *message);
  	void (*on_message_v5)(struct mosquitto *, void *userdata, const struct mosquitto_message *message, const mosquitto_property *props);
  	void (*on_subscribe)(struct mosquitto *, void *userdata, int mid, int qos_count, const int *granted_qos);
  	void (*on_subscribe_v5)(struct mosquitto *, void *userdata, int mid, int qos_count, const int *granted_qos, const mosquitto_property *props);
  	void (*on_unsubscribe)(struct mosquitto *, void *userdata, int mid);
  	void (*on_unsubscribe_v5)(struct mosquitto *, void *userdata, int mid, const mosquitto_property *props);
  	void (*on_log)(struct mosquitto *, void *userdata, int level, const char *str);
//    	void (*on_error)();*/
  	char *host;
  	int port;
  	char *bind_address;
  	unsigned int reconnects;
  	unsigned int reconnect_delay;
  	unsigned int reconnect_delay_max;
  	bool reconnect_exponential_backoff;
  	char threaded;
  	//  struct mosquitto__packet *out_packet_last;
//  #  ifdef WITH_SRV
//  	ares_channel achan;
//  #  endif
//  #endif
	uint8_t maximum_qos;

// #ifdef WITH_BROKER
  	UT_hash_handle hh_id;
  	UT_hash_handle hh_sock;
  	struct mosquitto *for_free_next;
  	struct session_expiry_list *expiry_list_item;
// #endif
//  #ifdef WITH_EPOLL
  	uint32_t events;
  //#endif
};

struct mosquitto_msg_store{
	struct mosquitto_msg_store *next;
	struct mosquitto_msg_store *prev;
	dbid_t db_id;
	char *source_id;
	char *source_username;
	struct mosquitto__listener *source_listener;
	char **dest_ids;
	int dest_id_count;
	int ref_count;
	char* topic;
	mosquitto_property *properties;
	mosquitto__payload_uhpa payload;
	time_t message_expiry_time;
	uint32_t payloadlen;
	uint16_t source_mid;
	uint16_t mid;
	uint8_t qos;
	bool retain;
	uint8_t origin;
};


struct mosquitto_client_msg{
	struct mosquitto_client_msg *prev;
	struct mosquitto_client_msg *next;
	struct mosquitto_msg_store *store;
	mosquitto_property *properties;
	time_t timestamp;
	uint16_t mid;
	uint8_t qos;
	bool retain;
	enum mosquitto_msg_direction direction;
	enum mosquitto_msg_state state;
	bool dup;
};


struct mosquitto__subleaf {
	struct mosquitto__subleaf *prev;
	struct mosquitto__subleaf *next;
	struct mosquitto *context;
	uint32_t identifier;
	uint8_t qos;
	bool no_local;
	bool retain_as_published;
};

struct mosquitto__subshared {
	UT_hash_handle hh;
	char *name;
	struct mosquitto__subleaf *subs;
};

struct mosquitto__subhier {
	UT_hash_handle hh;
	struct mosquitto__subhier *parent;
	struct mosquitto__subhier *children;
	struct mosquitto__subleaf *subs;
	struct mosquitto__subshared *shared;
	struct mosquitto_msg_store *retained;
	char *topic;
	uint16_t topic_len;
};


struct mosquitto__unpwd{
	char *username;
	char *password;
	UT_hash_handle hh;
};


struct mosquitto__config {
	bool allow_duplicate_messages;
	int autosave_interval;
	bool autosave_on_changes;
	bool check_retain_source;
	char *clientid_prefixes;
	bool connection_messages;
	bool daemon;
	struct mosquitto__listener default_listener;
	struct mosquitto__listener *listeners;
	int listener_count;
	int log_dest;
	int log_facility;
	unsigned int log_type;
	bool log_timestamp;
	char *log_timestamp_format;
	char *log_file;
	FILE *log_fptr;
	uint16_t max_inflight_messages;
	uint16_t max_keepalive;
	uint32_t max_packet_size;
	uint32_t message_size_limit;
	bool persistence;
	char *persistence_location;
	char *persistence_file;
	char *persistence_filepath;
	time_t persistent_client_expiration;
	char *pid_file;
	bool queue_qos0_messages;
	bool per_listener_settings;
	bool retain_available;
	bool set_tcp_nodelay;
	int sys_interval;
	bool upgrade_outgoing_qos;
	char *user;
//  #ifdef WITH_WEBSOCKETS
//  	int websockets_log_level;
//  	int websockets_headers_size;
//  	bool have_websockets_listener;
//  #endif
//  #ifdef WITH_BRIDGE
//  	struct mosquitto__bridge *bridges;
//  	int bridge_count;
//  #endif
	struct mosquitto__security_options security_options;
};

struct mosquitto_db{
	dbid_t last_db_id;
	struct mosquitto__subhier *subs;
	struct mosquitto__unpwd *unpwd;
	struct mosquitto__unpwd *psk_id;
	struct mosquitto *contexts_by_id;
	struct mosquitto *contexts_by_sock;
	struct mosquitto *contexts_for_free;
//  #ifdef WITH_BRIDGE
//  	struct mosquitto **bridges;
//  #endif
	//  struct clientid__index_hash *clientid_index_hash;
	struct mosquitto_msg_store *msg_store;
	struct mosquitto_msg_store_load *msg_store_load;
//  #ifdef WITH_BRIDGE
//  	int bridge_count;
//  #endif
	int msg_store_count;
	unsigned long msg_store_bytes;
	char *config_file;
	struct mosquitto__config *config;
	int auth_plugin_count;
	bool verbose;
//  #ifdef WITH_SYS_TREE
//  	int subscription_count;
//  	int shared_subscription_count;
//  	int retained_count;
//  #endif
	int persistence_changes;
	struct mosquitto *ll_for_free;
//  #ifdef WITH_EPOLL
//  	int epollfd;
//  #endif
};


/* 内存处理 */
void *mosquitto__malloc(size_t len);

void *mosquitto__calloc(size_t nmemb, size_t len);

void mosquitto__free(void *p);

char *mosquitto__strdup(const char *s);


/* packet 解析 */
int packet__read_byte(struct mosquitto__packet *packet, uint8_t *byte);

int packet__read_uint16(struct mosquitto__packet *packet, uint16_t *word);


int packet__read_binary(struct mosquitto__packet *packet, uint8_t **data, int *length);

int packet__read_uint32(struct mosquitto__packet *packet, uint32_t *word);

int packet__read_bytes(struct mosquitto__packet *packet, void *bytes, uint32_t count);


int mosquitto_validate_utf8(const char *str, int len);

int packet__read_string(struct mosquitto__packet *packet, char **str, int *length);


int packet__read_varint(struct mosquitto__packet *packet, int32_t *word, int8_t *bytes);


enum mosquitto_client_state mosquitto__get_state(struct mosquitto *mosq);

int mosquitto__set_state(struct mosquitto *mosq, enum mosquitto_client_state state);

const char *mosquitto_client_username(const struct mosquitto *context);


int property__process_will(struct mosquitto *context, struct
        mosquitto_message_all *msg, mosquitto_property **props);

const mosquitto_property *property__get_property(const mosquitto_property
        *proplist, int identifier, bool skip_first);


const mosquitto_property *mosquitto_property_read_binary(const
        mosquitto_property *proplist, int identifier, void **value, uint16_t
        *len, bool skip_first);


const mosquitto_property *mosquitto_property_read_string(const
        mosquitto_property *proplist, int identifier, char **value, bool
        skip_first);


int property__process_connect(struct mosquitto *context, mosquitto_property
        **props);


int property__read(struct mosquitto__packet *packet, int32_t *len,
        mosquitto_property *property);


int mosquitto_property_check_command(int command, int identifier);


int mosquitto_property_check_all(int command, const mosquitto_property
        *properties);


void property__free(mosquitto_property **property);


void mosquitto_property_free_all(mosquitto_property **property);


int property__read_all(int command, struct mosquitto__packet *packet,
        mosquitto_property **properties);


static void property__add(mosquitto_property **proplist, struct mqtt5__property
        *prop);


int mosquitto_property_add_byte(mosquitto_property **proplist, int identifier,
        uint8_t value);


int mosquitto_property_copy_all(mosquitto_property **dest, const
        mosquitto_property *src);


int packet__varint_bytes(int32_t word);

void packet__write_byte(struct mosquitto__packet *packet, uint8_t byte);

void packet__write_bytes(struct mosquitto__packet *packet, const void *bytes,
        uint32_t count);


int packet__write_varint(struct mosquitto__packet *packet, int32_t word);


void packet__write_uint16(struct mosquitto__packet *packet, uint16_t word);


void packet__write_uint32(struct mosquitto__packet *packet, uint32_t word);


void packet__write_string(struct mosquitto__packet *packet, const char *str,
        uint16_t length);


int property__write(struct mosquitto__packet *packet, const mosquitto_property
        *property);

int property__get_length(const mosquitto_property *property);


int property__get_length_all(const mosquitto_property *property);


int property__write_all(struct mosquitto__packet *packet, const
        mosquitto_property *properties, bool write_len);

int mosquitto_property_add_int16(mosquitto_property **proplist, int identifier,
        uint16_t value);


int mosquitto_property_add_int32(mosquitto_property **proplist, int identifier,
        uint32_t value);


int mosquitto_property_add_binary(mosquitto_property **proplist, int identifier,
        const void *value, uint16_t len);


int mosquitto_property_add_string(mosquitto_property **proplist, int identifier,
        const char *value);


int packet__check_oversize(struct mosquitto *mosq, uint32_t remaining_length);


int packet__alloc(struct mosquitto__packet *packet);


void packet__cleanup(struct mosquitto__packet *packet);

int packet__write(struct mosquitto *mosq);


int packet__queue(struct mosquitto *mosq, struct mosquitto__packet *packet);































#endif
