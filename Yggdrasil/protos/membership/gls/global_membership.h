/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * Guilherme Borges (g.borges@campus.fct.unl.pt)
 * (C) 2017
 * This code was produced in the context of the Algorithms
 * and Distributed Systems (2017/18) at DI, FCT, U. NOVA
 * Lectured by João Leitão
 *********************************************************/

#ifndef GLOBAL_MEMBERSHIP_H_
#define GLOBAL_MEMBERSHIP_H_

#include <uuid/uuid.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "core/lk_runtime.h"
#include "lightkone.h"

#include "core/utils/queue.h"
#include "core/utils/utils.h"
#include "protos/utils/user_util.h"
#include "members.h"
#include "protos/discovery/disc_ctr/discovery_CTR.h"
#include "protos/faultdetection/faultdetectorLinkBlocker.h"

#include "core/proto_data_struct.h"

#define PROTO_GLOBAL 101

typedef enum gls_events_ {
	GLS_ME_DEAD = 0,
	GLS_MAX = 1
} gls_events;

typedef enum gls_requests_ {
	GLS_QUERY_REQ = 1,
	GLS_QUERY_RES = 2
} gls_requests;

typedef struct gls_query_reply_ {
	short nmembers; //number of known members
	member* members; //members
} gls_query_reply;

void * global_membership_init(void * args);

void GLS_deserialiazeReply(gls_query_reply* gqr, LKRequest* req);

#endif /* GLOBAL_MEMBERSHIP_H_ */
