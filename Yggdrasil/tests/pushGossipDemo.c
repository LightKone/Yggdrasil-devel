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

#include "lightkone.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"
#include "core/utils/utils.h"
#include "protos/communication/broadcast/push_gossip.h"

#include <stdlib.h>


int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument
	int t = atoi(argv[1]);
	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);

	queue_t *inBox;

	//Init lk_runtime and protocols
	lk_runtime_init(ntconf, 0, 1, 1);

	short myId = registerApp(&inBox, 0);
	push_gossip_args pgossipargs;
	pgossipargs.avoidBCastStorm = 1; //TRUE
	pgossipargs.gManSchedule = 60; //1 minute
	addProtocol(PGOSSIP_BCAST_PROTO, &push_gossip_init, (void*) &pgossipargs, 0);

	//Start lk_runtime
	lk_runtime_start();

	//Start your business
	int sequence_number = 0;

	struct timespec nextoperation;
	gettimeofday((struct timeval *) &nextoperation,NULL);
	nextoperation.tv_sec += t / 1000000;
	nextoperation.tv_nsec += (t % 1000000) * 1000;

	while(1) {

		queue_t_elem elem;
		memset(&elem,0,sizeof(queue_t_elem));

		int obtained = queue_try_timed_pop(inBox, &nextoperation, &elem);

		if(obtained > 0) {
			if(elem.type == LK_MESSAGE) {
				char origin[33], dest[33];
				memset(origin,0,33);
				memset(dest,0,33);
				wlan2asc(&elem.data.msg.srcAddr, origin);
				wlan2asc(&elem.data.msg.destAddr, dest);
				char msg[1000];
				memset(msg, 0, 1000);
				sprintf(msg, "SRC: %s, DEST: %s, %s", origin, dest, elem.data.msg.data);
				lk_log("PushGossipDemo", "RECEIVE", msg);
			} else {
				lk_log("PushGossipDemo", "RECEIVE", "Got something unexpected message");
			}
		} else {
			LKRequest request;
			request.proto_origin = myId;
			request.proto_dest = PGOSSIP_BCAST_PROTO;
			request.request = REQUEST;
			request.request_type = PG_REQ;
			request.length = 500;
			request.payload = malloc(request.length);
			memset(request.payload, 0, 500);
			sprintf(request.payload, "%s PUSH GOSSIP BCAST MESSAGE %d", getHostname(), ++sequence_number);

			deliverRequest(&request);

			lk_log("PushGossipDemo", "BCAST", request.payload);

			//Release memory
			free(request.payload);

			//Reset maximum waiting time...
			gettimeofday((struct timeval *)&nextoperation,NULL);
			nextoperation.tv_sec += t / 1000000;
			nextoperation.tv_nsec += (t % 1000000) * 1000;
		}
	}

	return 0;
}

