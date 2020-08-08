#include "broker.h"
#include "nanolib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nng/nng.h>
#include <nng/protocol/mqtt/emq_tcp.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/supplemental/util/platform.h>
#include <nng/protocol/mqtt/mqtt.h>
#include <nng/nng.h>
#include "include/mqtt_db.h"

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
#define PARALLEL 2

// The server keeps a list of work items, sorted by expiration time,
// so that we can use this to set the timeout to the correct value for
// use in poll.
struct work {	
	enum { INIT, RECV, WAIT, SEND } state;
	nng_aio *aio;
	nng_msg *msg;
	nng_ctx  ctx;
	struct db_tree *db;
	conn_param *cparam;
};

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
	struct work *work = arg;
	nng_msg *    msg;
	nng_msg *    smsg;
	int          rv;
	uint32_t     when;
	uint8_t      buf[2] = {1,2};
	conn_param	*cp;

	switch (work->state) {
	case INIT:
		printf("INIT ^^^^^^^^^^^^^^^^^^^^^ \n");
                work->state = RECV;
		nng_ctx_recv(work->ctx, work->aio);
		printf("INIT!!\n");
		break;
	case RECV:
		printf("RECV  ^^^^^^^^^^^^^^^^^^^^^\n");
                if ((rv = nng_aio_result(work->aio)) != 0) {
                        fatal("nng_ctx_recv", rv);
                }
                msg = nng_aio_get_msg(work->aio);
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
                printf("RECV ********************* msg: %s ******************************************\n", (char *)nng_msg_body(work->msg));
                nng_sleep_aio(200, work->aio);
                break;
	case WAIT:
		// We could add more data to the message here.
	  cp = (conn_param *)nng_msg_get_conn_param(work->msg);
		printf("WAIT  ^^^^^^^^^^^^^^^^^^^^^ %x %s\n", nng_msg_cmd_type(work->msg), conn_param_get_clentid(cp));
/*
        if ((rv = nng_msg_append_u32(msg, msec)) != 0) {
                fatal("nng_msg_append_u32", rv);
        }
*/
                //reply to client if needed. nng_send_aio vs nng_sendmsg? async or sync? BETTER sync due to realtime requirement
                //TODO
        if ((rv = nng_msg_alloc(&smsg, 0)) != 0) {
            printf("error nng_msg_alloc^^^^^^^^^^^^^^^^^^^^^");
        }
        if (nng_msg_cmd_type(work->msg) == CMD_PINGREQ) {

			buf[0] = CMD_PINGRESP;
			buf[1] = 0x00;
			debug_msg("reply PINGRESP\n");

			if ((rv = nng_msg_header_append(smsg, buf, 2)) != 0) {
				printf("error nng_msg_append^^^^^^^^^^^^^^^^^^^^^");
			}
		}
		else if(nng_msg_cmd_type(work->msg) == CMD_SUBSCRIBE){
			debug_msg("reply Subscribe. \n");
			smsg = work->msg;

			// prevent we got the ctx_sub
			// insert ctx_sub into treeDB
//			Ctx_sub * ctx_sub = nni_alloc(sizeof(Ctx_sub));
//			ctx_sub->id = 5; // clientid; // id?????
/*
			struct client * client = nni_alloc(sizeof(struct client));
			char clientid[6] = "66665";
			client->id = clientid; // client id should be uint32 ????
>>>>>>> wangha/sub
//			client->ctxt = ; // wait the ctx??????
		        //struct topic_and_node *tan = nni_alloc(sizeof(struct topic_and_node));
//			search_node(db, ctx_sub->topic_with_option->topic_filter->str, &tan);
			//search_node(work->db, "a/b/t", &tan);
			//add_node(tan, client);
//			search_node(db, ctx_sub->topic_with_option->topic_filter->str, &tan);
//			add_client(tan, client->id);
			*/
			printf("FINISH ADD ctx & clientid. ");
		}
		else {
			work->msg   = NULL;
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);
			break;
		}

		work->msg   = smsg;
		// We could add more data to the message here.
		nng_aio_set_msg(work->aio, work->msg);

		printf("before send aio msg %s\n",(char *)nng_msg_body( work->msg));
		work->msg   = NULL;
		work->state = SEND;
		nng_ctx_send(work->ctx, work->aio);
		//nng_ctx_recv(work->ctx, work->aio);
		printf("after send aio\n");
		//work->state = RECV;
		//nng_recv_aio(work->sock, work->aio);          //tcp message -> internel IO pipe -> nng_recv_aio here
		//printf("WAIT ********************* msg: %s ******************************************\n", (char *)nng_msg_body(work->msg));

		break;
	case SEND:
		printf("SEND  ^^^^^^^^^^^^^^^^^^^^^\n");
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
	int          rv;

	if ((w = nng_alloc(sizeof(*w))) == NULL) {
		fatal("nng_alloc", NNG_ENOMEM);
	}
	if ((rv = nng_aio_alloc(&w->aio, server_cb, w)) != 0) {
		fatal("nng_aio_alloc", rv);
	}
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
	nng_socket   sock;
	struct work *works[PARALLEL];
	int          rv;
	int          i;
	// init tree
	struct db_tree *db;
	create_db_tree(&db);

	/*  Create the socket. */
	rv = nng_rep0_open(&sock);
	rv = nng_emq_tcp0_open(&sock);
	if (rv != 0) {
		fatal("nng_rep0_open", rv);
	}

	printf("PARALLEL: %d\n", PARALLEL);
	for (i = 0; i < PARALLEL; i++) {
		works[i] = alloc_work(sock);
		works[i]->db = db;
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
