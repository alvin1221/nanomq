#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <protocol/mqtt/mqtt_parser.h>
#include <nng.h>
#include <mqtt_db.h>
#include <hash.h>

#include "include/nanomq.h"
#include "include/pub_handler.h"
#include "include/subscribe_handle.h"
#include "include/unsubscribe_handle.h"

// Parallel is the maximum number of outstanding requests we can handle.
// This is *NOT* the number of threads in use, but instead represents
// outstanding work items.  Select a small number to reduce memory size.
// (Each one of these can be thought of as a request-reply loop.)  Note
// that you will probably run into limitations on the number of open file
// descriptors if you set this too high. (If not for that limit, this could
// be set in the thousands, each context consumes a couple of KB.)
// #ifndef PARALLEL
// #define PARALLEL 128
// #endif
#define PARALLEL 100

// The server keeps a list of work items, sorted by expiration time,
// so that we can use this to set the timeout to the correct value for
// use in poll.

void
fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

void transmit_msgs_cb(nng_msg *send_msg, emq_work *work, uint32_t *pipes)
{
	work->state = SEND;
	work->msg   = send_msg;//FIXME should be encode message for each sub cilent due to different minimum QoS
	nng_aio_set_msg(work->aio, send_msg);
	work->msg = NULL;

	nng_aio_set_pipeline(work->aio, pipes);
	//nng_ctx_send(work->ctx, work->aio);
}


/*objective: 1 input/output low latency
	     2 KV
	     3 tree 
*/
void
server_cb(void *arg)
{
	struct work *work = arg;
	nng_ctx     ctx2;
	nng_msg     *msg;
	nng_msg     *smsg;
	nng_pipe    pipe;
	int         rv;
	uint32_t    when;

	uint32_t          *pipes      = NULL;
	volatile uint32_t total_pipes = 0;
//	struct topic_and_node *tp_node    = NULL;
	reason_code       reason;
	uint8_t buf[2];


	switch (work->state) {
		case INIT:
			printf("INIT ^^^^^^^^^^^^^^^^^^^^^ \n");
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);
			printf("INIT!!\n");
			break;
		case RECV:
			debug_msg("RECV  ^^^^^^^^^^^^^^^^^^^^^ %d ^^^^\n", work->ctx.id);
			if ((rv = nng_aio_result(work->aio)) != 0) {
				debug_msg("RECV nng aio result error: %d", rv);
				//nng_aio_wait(work->aio);
				//break;
				fatal("nng_ctx_recv", rv);
			}
			msg     = nng_aio_get_msg(work->aio);
			if (msg == NULL) {		//BUG
				debug_msg("RECV NULL msg");
			}
			pipe    = nng_msg_get_pipe(msg);
			debug_msg("RECVIED %d %x\n", work->ctx.id,  nng_msg_cmd_type(msg));

			if(nng_msg_cmd_type(msg) == CMD_DISCONNECT){
				work->cparam = nng_msg_get_conn_param(msg);
				char * clientid = conn_param_get_clentid(work->cparam);
				struct topic_and_node * tan = nng_alloc(sizeof(struct topic_and_node));
				char ** topics;
				struct client * cli = NULL;
				struct topic_queue * tq = NULL;

				debug_msg("########DISCONNECT########clientid: %s", clientid);
				if(check_id(clientid)){
					debug_msg("This client is in hash table.");
					tq = get_topic(clientid);
					while(tq){
						topics = topic_parse(tq->topic);
						search_node(work->db, topics, tan);
						cli = del_client(tan, clientid);
						del_node(tan->node);
						destroy_sub_ctx(cli->ctxt); // only free work->sub_pkt
						nng_free(cli, sizeof(struct client));
						tq = tq->next;
					}
					del_topic_all(clientid);
					del_pipe_id(pipe.id);
					debug_msg("INHASH: clientid [%s] exist?: %d; pipeid [%d] exist?: %d", clientid, check_id(clientid), pipe.id, check_pipe_id(pipe.id));
				}

				nng_free(tan, sizeof(struct topic_and_node));
				work->state = RECV;
				nng_msg_free(msg);
				nng_aio_abort(work->aio, 31);
				nng_ctx_recv(work->ctx, work->aio);
				break;
			}

			work->msg   = msg;
			work->state = WAIT;
			debug_msg("RECV ********************* msg: %s %x******************************************\n",
			          (char *) nng_msg_body(work->msg), nng_msg_cmd_type(work->msg));
			nng_sleep_aio(200, work->aio);
			break;
		case WAIT:
			debug_msg("WAIT ^^^^^^^^^^^^^^^^^^^^^ %d ^^^^", work->ctx.id);
			// We could add more data to the message here.
			work->cparam = nng_msg_get_conn_param(work->msg);
			//debug_msg("WAIT   %x %s %d pipe: %d\n", nng_msg_cmd_type(work->msg),
			          //conn_param_get_clentid(work->cparam), work->ctx.id, work->pid.id);
/*
        if ((rv = nng_msg_append_u32(msg, msec)) != 0) {
                fatal("nng_msg_append_u32", rv);
        }
*/
			//reply to client if needed. nng_send_aio vs nng_sendmsg? async or sync? BETTER sync due to realtime requirement
			//TODO
			if ((rv = nng_msg_alloc(&smsg, 0)) != 0) {
				debug_msg("error nng_msg_alloc^^^^^^^^^^^^^^^^^^^^^");
			}
			if (nng_msg_cmd_type(work->msg) == CMD_PINGREQ) {
				buf[0] = CMD_PINGRESP;
				buf[1] = 0x00;
				debug_msg("reply PINGRESP\n");

				if ((rv = nng_msg_header_append(smsg, buf, 2)) != 0) {
					debug_msg("error nng_msg_append^^^^^^^^^^^^^^^^^^^^^");
				}
				work->msg = smsg;
				// We could add more data to the message here.
				nng_aio_set_msg(work->aio, work->msg);

				printf("before send aio msg %s\n", (char *) nng_msg_body(work->msg));
				work->msg   = NULL;
				work->state = SEND;
				nng_ctx_send(work->ctx, work->aio);
				break;

			} else if (nng_msg_cmd_type(work->msg) == CMD_SUBSCRIBE) {
				pipe = nng_msg_get_pipe(work->msg);
				work->pid = pipe;
				printf("get pipe!!  ^^^^^^^^^^^^^^^^^^^^^ %d %d\n", pipe.id, work->pid.id);
				work->sub_pkt = nng_alloc(sizeof(struct packet_subscribe));
				if ((reason = decode_sub_message(work->msg, work->sub_pkt)) != SUCCESS) {
					debug_msg("ERROR in decode: %x.", reason);
				}
				// Handle the sub_ctx & ops to tree
				debug_msg("In sub_pkt: pktid:%d, topicLen: %d", work->sub_pkt->packet_id,
				          work->sub_pkt->node->it->topic_filter.len);
				sub_ctx_handle(work);

				if ((reason = encode_suback_message(smsg, work->sub_pkt)) != SUCCESS) {
					debug_msg("ERROR in encode: %x.", reason);
				}
				debug_msg("Header Len: %d, Body Len: %d.", nng_msg_header_len(smsg), nng_msg_len(smsg));
				debug_msg("In Body. TYPE:%x LEN:%x PKTID: %x %x.", *((uint8_t *) nng_msg_header(smsg)),
				          *((uint8_t *) nng_msg_header(smsg) + 1), *((uint8_t *) nng_msg_body(smsg)),
				          *((uint8_t *) nng_msg_body(smsg) + 1));
				work->msg = smsg;
				// We could add more data to the message here.
				nng_aio_set_msg(work->aio, work->msg);
				work->msg   = NULL;
				work->state = SEND;
				nng_ctx_send(work->ctx, work->aio);
				//nng_send_aio
				printf("after send aio\n");
			} else if (nng_msg_cmd_type(work->msg) == CMD_UNSUBSCRIBE) {
				debug_msg("handle CMD_UNSUBSCRIBE\n");
				work->unsub_pkt = nng_alloc(sizeof(struct packet_unsubscribe));
				if ((reason = decode_unsub_message(work->msg, work->unsub_pkt)) != SUCCESS) {
					debug_msg("ERROR in decode: %x.", reason);
				}
				debug_msg("In unsub_pktkt: pktid:%d, topicLen: %d", work->unsub_pkt->packet_id,
				          work->unsub_pkt->node->it->topic_filter.len);
				// Handle the sub_ctx & ops to tree
				unsub_ctx_handle(work);

				if ((reason = encode_unsuback_message(smsg, work->unsub_pkt)) != SUCCESS) {
					debug_msg("ERROR in encode: %x.", reason);
				}
				debug_msg("Header Len: %d, Body Len: %d.", nng_msg_header_len(smsg), nng_msg_len(smsg));
				debug_msg("In Body. TYPE:%x LEN:%x PKTID: %x %x.", *((uint8_t *) nng_msg_header(smsg)),
				          *((uint8_t *) nng_msg_header(smsg) + 1), *((uint8_t *) nng_msg_body(smsg)),
				          *((uint8_t *) nng_msg_body(smsg) + 1));
				work->msg = smsg;
				// We could add more data to the message here.
				nng_aio_set_msg(work->aio, work->msg);
				work->msg   = NULL;
				work->state = SEND;
				nng_ctx_send(work->ctx, work->aio);
				printf("after send aio\n");
			} else if (nng_msg_cmd_type(work->msg) == CMD_PUBLISH ||
			           nng_msg_cmd_type(work->msg) == CMD_PUBACK ||
			           nng_msg_cmd_type(work->msg) == CMD_PUBREC ||
			           nng_msg_cmd_type(work->msg) == CMD_PUBREL ||
			           nng_msg_cmd_type(work->msg) == CMD_PUBCOMP) {

//				nng_mtx_lock(work->mutex);

				handle_pub(work, smsg, pipes, transmit_msgs_cb);
				if ((rv = nng_aio_result(work->aio)) != 0) {
					debug_msg("WAIT nng aio result error: %d", rv);
					//nng_aio_wait(work->aio);
					//break;
				}

				if (work->state == SEND) {
					nng_ctx_send(work->ctx, work->aio);
				} else {
					work->msg   = NULL;
					work->state = RECV;
					nng_ctx_recv(work->ctx, work->aio);
				}

//				nng_mtx_unlock(work->mutex);

			} else {
				debug_msg("broker has nothing to do");
				work->msg   = NULL;
				work->state = RECV;
				nng_ctx_recv(work->ctx, work->aio);
				break;
			}
			break;

		case SEND:
			debug_msg("SEND  ^^^^^^^^^^^^^^^^^^^^^ %d ^^^^\n", work->ctx.id);
			if ((rv = nng_aio_result(work->aio)) != 0) {
				nng_msg_free(work->msg);
				debug_msg("SEND nng aio result error: %d", rv);
				fatal("nng_ctx_send", rv);
			}
			work->msg   = NULL;
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);

			if (pipes != NULL) {
				free(pipes);
				pipes = NULL;
			}
			break;
		default:
			fatal("bad state!", NNG_ESTATE);
			break;
	}
}

struct work *
alloc_work(nng_socket sock)
{
	struct work *w;
	int         rv;

	if ((w  = nng_alloc(sizeof(*w))) == NULL) {
		fatal("nng_alloc", NNG_ENOMEM);
	}
	if ((rv = nng_aio_alloc(&w->aio, server_cb, w)) != 0) {
		fatal("nng_aio_alloc", rv);
	}
	//not pipe id; max id = uint32_t
	if ((rv = nng_ctx_open(&w->ctx, sock)) != 0) {
		fatal("nng_ctx_open", rv);
	}
	if ((rv = nng_mtx_alloc(&w->mutex)) != 0) {
		fatal("nng_mtx_alloc", rv);
	}

	w->state = INIT;
	return (w);
}

// The server runs forever.
int
server(const char *url)
{
	nng_socket     sock;
	nng_pipe       pipe_id;
	struct work    *works[PARALLEL];
	int            rv;
	int            i;
	// init tree
	struct db_tree *db = NULL;
	create_db_tree(&db);

	/*  Create the socket. */
	rv = nng_nano_tcp0_open(&sock);
	if (rv != 0) {
		fatal("nng_nano_tcp0_open", rv);
	}

	printf("PARALLEL: %d\n", PARALLEL);
	for (i = 0; i < PARALLEL; i++) {
		works[i] = alloc_work(sock);
		works[i]->db = db;
		nng_aio_set_dbtree(works[i]->aio, db);
//		works[i]->pid = pipe_id;
	}

	if ((rv = nng_listen(sock, url, NULL, 0)) != 0) {
		fatal("nng_listen", rv);
	}

	for (i = 0; i < PARALLEL; i++) {
		server_cb(works[i]); // this starts them going (INIT state)
	}

	for (;;) {
		nng_msleep(3600000); // neither pause() nor sleep() portable
	}
}

int broker_start(int argc, char **argv)
{
	int rc;
	if (argc != 1) {
		fprintf(stderr, "Usage: broker start <url>\n");
		exit(EXIT_FAILURE);
	}
	rc = server(argv[0]);
	exit(rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

int broker_dflt(int argc, char **argv)
{
	debug_msg("i dont know what to do here yet");
	return 0;
}
