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

#include "lightkone.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"
#include "protos/faultdetection/faultdetectorLinkBlocker.h"
#include "protos/discovery/disc_ctr/discovery_CTR.h"
#include "protos/membership/gls/global_membership.h"
#include "protos/faultdetection/faultdetector.h"
#include "core/utils/utils.h"
#include "protos/utils/topologyManager.h"

#include <stdlib.h>


int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument
	int t = atoi(argv[1]);
	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);

	queue_t *inBox;

	//Init lk_runtime and protocols
	lk_runtime_init(ntconf, 1, 3, 1);

	addLKProtocol(PROTO_TOPOLOGY_MANAGER, &topologyManager_init, NULL, 0);

	short myId = registerApp(&inBox, 0);
	addProtocol(PROTO_DISCOV_CTR, &discovery_Full_FT_CTR_init,(void*) 2, DISC_CTR_EV_MAX);
	addProtocol(PROTO_GLOBAL, &global_membership_init, NULL, GLS_MAX);
	addProtocol(PROTO_FT_LB, &faultdetectorLinkBlocker_init, (void*) 10000, FT_LB_EV_MAX);

	//Configure protocols
	queue_t* inter = interceptProtocolQueue(PROTO_DISPATCH, PROTO_FT_LB);
	ft_lb_attr fdattr;
	fdattr.intercept = inter;
	fdattr.timeout_failure_ms = 10000;
	updateProtoAttr(PROTO_FT_LB, (void *) &fdattr);

	short ev[1];
	ev[0] = FT_LB_SUSPECT;
	registInterestEvents(PROTO_FT_LB,ev,1,PROTO_DISCOV_CTR);
	registInterestEvents(PROTO_FT_LB,ev,1,PROTO_GLOBAL);
	ev[0] = DISC_CTR_NEIGHBOR_UP;
	registInterestEvents(PROTO_DISCOV_CTR,ev,1,PROTO_GLOBAL);
	ev[0] = GLS_ME_DEAD;
	registInterestEvents(PROTO_GLOBAL, ev, 1, PROTO_DISCOV_CTR);

	//other option (use faultdetector from lk protocols
	//addLKProtocol(PROTO_FAULT_DECT, &faultdetector_init, (void*) 10000, FT_EV_MAX);

	//	queue_t* inter = interceptProtocolQueue(PROTO_DISPATCH, PROTO_FAULT_DECT);
	//	ft_attr fdattr;
	//	fdattr.intercept = inter;
	//	fdattr.timeout_failure_ms = 10000;
	//	updateProtoAttr(PROTO_FAULT_DECT, (void *) &fdattr);

	//	short ev[1];
	//	ev[0] = FT_SUSPECT;
	//	registInterestEvents(PROTO_FAULT_DECT,ev,1,PROTO_DISCOV_CTR);
	//	registInterestEvents(PROTO_FAULT_DECT,ev,1,PROTO_GLOBAL);
	//	ev[0] = DISC_CTR_NEIGHBOR_UP;
	//	registInterestEvents(PROTO_DISCOV_CTR,ev,1,PROTO_GLOBAL);
	//	ev[0] = GLS_ME_DEAD;
	//	registInterestEvents(PROTO_GLOBAL, ev, 1, PROTO_DISCOV_CTR);


	//Start lk_runtime
	lk_runtime_start();

	//Start your business
	LKMessage message;

	int sequence_number = 0;

	struct timespec nextoperation;
	gettimeofday((struct timeval *) &nextoperation,NULL);
	nextoperation.tv_sec += t / 1000000;
	nextoperation.tv_nsec += (t % 1000000) * 1000;

	while(1) {

		queue_t_elem elem;
		memset(&elem,0,sizeof(queue_t_elem));
		memset(&message, 0, sizeof(LKMessage));

		int obtained = queue_try_timed_pop(inBox, &nextoperation, &elem);


		if(obtained > 0) {
			if(elem.type == LK_MESSAGE) {

				lk_log("APP", "WARNING", "Received a message");

			} else if(elem.type == LK_REQUEST) {
				if(elem.data.request.proto_origin == PROTO_GLOBAL){
					if(elem.data.request.request == REPLY){
						if(elem.data.request.request_type == GLS_QUERY_RES){
							gls_query_reply gqr;
							gqr.members = NULL;
							GLS_deserialiazeReply(&gqr, &elem.data.request);

							int i = 0;
							for(; i < gqr.nmembers; i++){

								char msg[2000];
								memset(msg, 0, 2000);
								char out[37];
								uuid_unparse((gqr.members+i)->uuid, out);
								sprintf(msg, "Member %d of %d; uuid: %s; status %d; counter %d", i, gqr.nmembers, out, (gqr.members+i)->attr, (gqr.members+i)->counter);
								lk_log("APP", "MEMBERSHIP STATUS", msg);
							}
							if(gqr.members != NULL)
								free(gqr.members);
						}
					}
				}
			} else {
				lk_log("APP","WARNING","Got something unexepted");
			}
		} else {

			LKRequest req;
			req.proto_origin = myId;
			req.proto_dest = PROTO_GLOBAL;
			req.request = REQUEST;
			req.request_type = GLS_QUERY_REQ;
			req.length = 0;
			req.payload = NULL;
			deliverRequest(&req);

			lk_log("APP", "QUERIED GLS", "");

			//Reset maximum waiting time...
			gettimeofday((struct timeval *)&nextoperation,NULL);
			nextoperation.tv_sec += t / 1000000;
			nextoperation.tv_nsec += (t % 1000000) * 1000;
		}
	}

	return 0;
}

