#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <protocol/mqtt/mqtt_parser.h>
#include <nng.h>
#include <mqtt_db.h>

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
#define PARALLEL 50

// The server keeps a list of work items, sorted by expiration time,
// so that we can use this to set the timeout to the correct value for
// use in poll.

void
fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

/*objective: 1 input/output low latency
	     2 KV
	     3 tree 
*/
void
server_cb(void *arg)
{
	struct work *work  = arg;
	nng_ctx     ctx2;
	nng_msg     *msg;
	nng_msg     *smsg;
	nng_pipe    pipe;
	int         rv;
	uint32_t    when;
	uint8_t     buf[2] = {1, 2};
	reason_code reason;

	struct topic_and_node *tp_node = NULL;

	switch (work->state) {
		case INIT:
			printf("INIT ^^^^^^^^^^^^^^^^^^^^^ \n");
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);
			printf("INIT!!\n");
			break;
		case RECV:
			debug_msg("RECV  ^^^^^^^^^^^^^^^^^^^^^ %d\n", work->ctx.id);
			if ((rv = nng_aio_result(work->aio)) != 0) {
				break;
				fatal("nng_ctx_recv", rv);
			}
			msg     = nng_aio_get_msg(work->aio);
			pipe    = nng_msg_get_pipe(msg);
			debug_msg("pipe!!  ^^^^^^^^^^^^^^^^^^^^^ %d\n", pipe.id);
/*
                if ((rv = nng_msg_trim_u32(msg, &when)) != 0) {
                        // bad message, just ignore it.
                        nng_msg_free(msg);
                        nng_ctx_recv(work->sock, work->aio);
                        printf("debug: bad message, just ignore it.\n");
                        return;
                }
*/
			work->msg   = msg;
			work->state = WAIT;
			debug_msg("RECV ********************* msg: %s ******************************************\n",
			          (char *) nng_msg_body(work->msg));
			nng_sleep_aio(200, work->aio);
			break;
		case WAIT:
			// We could add more data to the message here.
			work->cparam = nng_msg_get_conn_param(work->msg);
			debug_msg("WAIT  ^^^^^^^^^^^^^^^^^^^^^ %x %s %d pipe: %d\n", nng_msg_cmd_type(work->msg),
			          conn_param_get_clentid(work->cparam), work->ctx.id, work->pid.id);
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
			} else if (nng_msg_cmd_type(work->msg) == CMD_PUBLISH) {
				debug_msg("handle CMD_PUBLISH\n");

				work->pub_packet = (struct pub_packet_struct *) nng_alloc(sizeof(struct pub_packet_struct));
				tp_node = (struct topic_and_node *) nng_alloc(sizeof(struct topic_and_node));

				reason = handle_pub(work, tp_node);

				if (SUCCESS == reason) {

					if (work->pub_packet->fixed_header.qos == 0) {
						work->pub_packet->fixed_header.dup = 0;

					} else if (work->pub_packet->fixed_header.qos == 1 || work->pub_packet->fixed_header.qos == 2) {
						struct pub_packet_struct pub_response = {
								.fixed_header.packet_type =work->pub_packet->fixed_header.qos == 1 ? PUBACK : PUBREC,
								.fixed_header.qos = 0,
								.fixed_header.dup = 0,
								.fixed_header.retain = 0,
								.fixed_header.remain_len = 2,
								.variable_header.pub_arrc.packet_identifier = work->pub_packet->variable_header.publish.packet_identifier
						};

						encode_pub_message(smsg, &pub_response, work);

						//response PUBACK/PUBREC to client
						work->state = SEND;
						work->msg   = smsg;
						nng_aio_set_msg(work->aio, work->msg);
						work->msg = NULL;
//						nng_aio_set_pipeline(work->aio, work->pid.id);
						nng_ctx_send(work->ctx, work->aio); //FIXME Bug!

						work->pub_packet->fixed_header.dup = 0;//if publish first time
					}

					if (tp_node != NULL && tp_node->topic == NULL) {
//TODO					struct clients *client_list = search_client(work->db->root, topic_queue);
//						struct client  *sub_clients = client_list->sub_client;

						struct client *sub_clients = tp_node->node->sub_client;
						emq_work      *client_work;

						while (sub_clients) { //FIXME
							debug_msg("current client pointer: [%p], id: %s, next: %p", sub_clients, sub_clients->id,
							          sub_clients->next);
							encode_pub_message(smsg, work->pub_packet, work);
							client_work = (emq_work *) sub_clients->ctxt;

							debug_msg("client id: [%s], ctx: [%d] aio: [%p], pipe_id: [%d], aio result: [%d]",
							          sub_clients->id, client_work->ctx.id, client_work->aio, client_work->pid.id,
							          nng_aio_result(client_work->aio));

							work->state = SEND;
							work->msg   = smsg;
							nng_aio_set_msg(work->aio, smsg);
							work->msg = NULL;

							nng_aio_set_pipeline(work->aio, client_work->pid.id);
							nng_ctx_send(work->ctx, work->aio);

							sub_clients = sub_clients->next;
						}

					} else {
						debug_msg("can not find topic [%s] info",
						          work->pub_packet->variable_header.publish.topic_name.str_body);
					}
				} else {
					//TODO send DISCONNECT with reason_code if MQTT Version=5.0
				}

				if (work->pub_packet->variable_header.publish.topic_name.str_body != NULL) {
					nng_free(work->pub_packet->variable_header.publish.topic_name.str_body,
					         work->pub_packet->variable_header.publish.topic_name.str_len + 1);
					work->pub_packet->variable_header.publish.topic_name.str_body = NULL;
					debug_msg("free memory topic");
				}

				if (work->pub_packet->payload_body.payload != NULL) {
					nng_free(work->pub_packet->payload_body.payload, work->pub_packet->payload_body.payload_len + 1);
					work->pub_packet->payload_body.payload = NULL;
					debug_msg("free memory payload");
				}

				if (work->pub_packet != NULL) {
					nng_free(work->pub_packet, sizeof(struct pub_packet_struct));
					work->pub_packet = NULL;
					debug_msg("free memory payload");
				}

				if (tp_node != NULL) {
					nng_free(tp_node, sizeof(struct topic_and_node));
					tp_node = NULL;
					debug_msg("free memory topic_and_node");
				}

				if (work->state != SEND) {
					work->msg   = NULL;
					work->state = RECV;
					nng_ctx_recv(work->ctx, work->aio);
				}

			} else if (nng_msg_cmd_type(work->msg) == CMD_PUBACK) {
				debug_msg("handle CMD_PUBACK\n");
				work->pub_packet = (struct pub_packet_struct *) nng_alloc(sizeof(struct pub_packet_struct));
				reason = handle_pub(work, tp_node);
				if (SUCCESS == reason) {

				}

				if (work->pub_packet != NULL) {
					nng_free(work->pub_packet, sizeof(struct pub_packet_struct));
					work->pub_packet = NULL;
					debug_msg("free memory payload");
				}
			} else if (nng_msg_cmd_type(work->msg) == CMD_PUBREC) {
				debug_msg("handle CMD_PUBREC\n");
				work->pub_packet = (struct pub_packet_struct *) nng_alloc(sizeof(struct pub_packet_struct));
				reason = handle_pub(work, tp_node);
				if (SUCCESS == reason) {
					struct pub_packet_struct pub_response = {
							.fixed_header.packet_type = PUBREL,
							.fixed_header.qos = 0,
							.fixed_header.dup = 0,
							.fixed_header.retain = 0,
							.fixed_header.remain_len = 2,
							.variable_header.pubrel.packet_identifier = work->pub_packet->variable_header.publish.packet_identifier
					};

					encode_pub_message(smsg, &pub_response, work);

					//response PUBREL to client

					work->state = SEND;
					work->msg   = smsg;
					nng_aio_set_msg(work->aio, work->msg);
					work->msg = NULL;
//						nng_aio_set_pipeline(work->aio, work->pid.id);
					nng_ctx_send(work->ctx, work->aio); //FIXME Bug!

				}

				if (work->pub_packet != NULL) {
					nng_free(work->pub_packet, sizeof(struct pub_packet_struct));
					work->pub_packet = NULL;
					debug_msg("free memory payload");
				}
			} else if (nng_msg_cmd_type(work->msg) == CMD_PUBREL) {
				debug_msg("handle CMD_PUBREL\n");
				work->pub_packet = (struct pub_packet_struct *) nng_alloc(sizeof(struct pub_packet_struct));
				reason = handle_pub(work, tp_node);
				if (SUCCESS == reason) {
					struct pub_packet_struct pub_response = {
							.fixed_header.packet_type = PUBCOMP,
							.fixed_header.dup = 0,
							.fixed_header.qos = 1,//bit1 = 1
							.fixed_header.retain = 0,
							.fixed_header.remain_len = 2,
							.variable_header.pubcomp.packet_identifier = work->pub_packet->variable_header.publish.packet_identifier
					};

					encode_pub_message(smsg, &pub_response, work);
					//response PUBREL to client

					work->state = SEND;
					work->msg   = smsg;
					nng_aio_set_msg(work->aio, work->msg);
					work->msg = NULL;
//						nng_aio_set_pipeline(work->aio, work->pid.id);
					nng_ctx_send(work->ctx, work->aio); //FIXME Bug!
				}

				if (work->pub_packet != NULL) {
					nng_free(work->pub_packet, sizeof(struct pub_packet_struct));
					work->pub_packet = NULL;
					debug_msg("free memory payload");
				}

			} else if (nng_msg_cmd_type(work->msg) == CMD_PUBCOMP) {
				debug_msg("handle CMD_PUBCOMP\n");
				work->pub_packet = (struct pub_packet_struct *) nng_alloc(sizeof(struct pub_packet_struct));
				reason = handle_pub(work, tp_node);

				if (SUCCESS == reason) {

				}

				if (work->pub_packet != NULL) {
					nng_free(work->pub_packet, sizeof(struct pub_packet_struct));
					work->pub_packet = NULL;
					debug_msg("free memory payload");
				}

			} else {
				debug_msg("broker has nothing to do");
				work->msg   = NULL;
				work->state = RECV;
				nng_ctx_recv(work->ctx, work->aio);
				break;
			}
			break;

		case SEND:
			debug_msg("SEND  ^^^^^^^^^^^^^^^^^^^^^\n");
			if ((rv = nng_aio_result(work->aio)) != 0) {
				nng_msg_free(work->msg);
				fatal("nng_ctx_send", rv);
			}
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);
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
