//
// Copyright 2020 Staysail Systems, Inc. <info@staysail.tech>
// Copyright 2018 Capitar IT Group BV <info@capitar.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#ifndef NNG_PROTOCOL_REQREP0_REP_H
#define NNG_PROTOCOL_REQREP0_REP_H

#ifdef __cplusplus
extern "C" {
#endif

NNG_DECL int nng_emq_tcp0_open(nng_socket *);

#ifndef nng_emq_tcp_open
#define nng_emq_tcp_open nng_emq_tcp0_open
#endif


#define NNG_EMQ_TCP_SELF 0x31
#define NNG_EMQ_TCP_PEER 0x30
#define NNG_EMQ_TCP_SELF_NAME "emq_rep"
#define NNG_EMQ_TCP_PEER_NAME "emq_req"

#ifdef __cplusplus
}
#endif

#endif // NNG_PROTOCOL_REQREP0_REP_H
