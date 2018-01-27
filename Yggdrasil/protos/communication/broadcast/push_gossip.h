/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2017
 *********************************************************/

#ifndef PUSH_GOSSIP_H_
#define PUSH_GOSSIP_H_

#define PGOSSIP_BCAST_PROTO 101

#include <uuid/uuid.h>
#include <stdlib.h>
#include <time.h>

#include "core/lk_runtime.h"
#include "lightkone.h"

#include "core/utils/utils.h"

#include "core/proto_data_struct.h"
#include "core/utils/queue.h"

typedef enum pg_requests_ {
	PG_REQ = 0,
} pg_requests;

typedef struct _push_gossip_args {
	int gManSchedule;
	unsigned short avoidBCastStorm;
} push_gossip_args;

void * push_gossip_init(void * args);

#endif /* PUSH_GOSSIP_H_ */
