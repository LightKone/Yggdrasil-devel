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

#ifndef DISCOVERY_H_
#define DISCOVERY_H_

#include <stdio.h>
#include <uuid/uuid.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>


#include "lightkone.h"

#include "core/utils/utils.h"
#include "core/lk_runtime.h"
#include "core/protos/timer.h"
#include "core/utils/queue.h"

#define DEFAULT_NEIGHS_SIZE 10

#define UUID_SIZE sizeof(uuid_t)

typedef enum discovery_events_ {
	DISC_NEIGHBOR_UP = 0,
	DISC_NEIGHBOR_DOWN = 1,
	DISC_EV_MAX = 2
} discovery_events;

enum discovery_requests_ {
	DISC_REQ_EXIST_NEIGH, //replies with a short
	DISC_REQ_GET_N_NEIGHS, //replies with a int
	DISC_REQ_GET_NEIGH, //replies with a int (number of neigh in reply) a struct* neigh
	DISC_REQ_GET_ALL_NEIGHS, //replies with a int (number of neigh in reply) a struct* neigh
	DISC_REQ_GET_RND_NEIGH //replies with a int (number of neigh in reply) a struct* neigh
};


typedef struct _neighbor {
	short id;
	WLANAddr addr;
	uuid_t uuid;

} neigh;

typedef struct _neighbors {
	uuid_t myuuid;
	int known;
	int maxNeigh;
	neigh* neighbors;

} neighbors;

neighbors neighs;
time_t lastPeriod;


int existNeight(short id);
neigh* getNeighbor(short id);
int getKnownNeighbors();
neigh* getNeighbors();
short destroyNeigh(short id);


neigh* getRandomNeighbor();

void processLKRequest(LKRequest* req, short protoID);

#endif /* DISCOVERY_H_ */
