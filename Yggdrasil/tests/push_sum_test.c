/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2018
 *********************************************************/

#include "lightkone.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"
#include "protos/faultdetection/faultdetectorLinkBlocker.h"
#include "protos/discovery/disc_ctr/discovery_CTR.h"
#include "protos/membership/gls/global_membership.h"
#include "protos/faultdetection/faultdetector.h"
#include "core/utils/utils.h"

#include "protos/discovery/discovery_ft_lb.h"
#include "../protos/discovery/discoveryFullFT.h"
#include "../protos/communication/point-to-point/reliable.h"

//#include "protos/aggregation/aggregation_interface.h"

#include "protos/aggregation/aggregation_gen_interface.h"

//#include "protos/aggregation/simple_aggregation.h"
//#include "protos/aggregation/simple_bloom_aggregation.h"
//#include "protos/aggregation/single_tree_aggregation.h"
#include "protos/aggregation/push_sum_single.h"

#include "protos/utils/tcpserver.h"
#include "protos/utils/topologyManager.h"

#include <stdlib.h>
#include <uuid/uuid.h>


#define PROTO_SIMPLE_AGGREGATION 200
#define PROTO_SIMPLE_BLOOM_AGGREGATION 201
#define PROTO_SINGLE_TREE_AGGREGATION 202
#define PROTO_PUSH_SUM 203

#define PROTO_DISCOV_FT_LB 210

#define PROTO_RELIABLE 101


int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument

	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);

	queue_t *inBox;

	//Init lk_runtime and protocols
	lk_runtime_init(ntconf, 0, 3, 1);

	short myId = registerApp(&inBox, 0);

	//regist topology manager to have virtual multihop topology
	//addLKProtocol(PROTO_TOPOLOGY_MANAGER, &topologyManager_init, NULL, 0);

	//regist reliable
	addProtocol(PROTO_RELIABLE, &reliable_point2point, NULL, MAX_EV);
	queue_t* q = interceptProtocolQueue(PROTO_DISPATCH, PROTO_RELIABLE);
	updateProtoAttr(PROTO_RELIABLE, (void*) q);

	//regist discov
	addProtocol(PROTO_DISCOV_FT_LB, &discoveryFullFT_init, 2, DISCOV_FT_LB_EV_MAX);
//	q = interceptProtocolQueue(PROTO_DISPATCH, PROTO_DISCOV_FT_LB);
//	discov_ft_lb_attr attr;
//	attr.intercept = q;
//	attr.timeout_failure_ms = 10000; //10 seconds?
//	updateProtoAttr(PROTO_DISCOV_FT_LB, (void*) &attr);

	//regist aggregation protocols
	//addProtocol(PROTO_SIMPLE_AGGREGATION, &simple_aggregation_init, NULL, 0);
	//addProtocol(PROTO_SIMPLE_BLOOM_AGGREGATION, &simple_bloom_aggregation_init, NULL, 0);
	//addProtocol(PROTO_SINGLE_TREE_AGGREGATION, &stree_init, NULL, 0);
	push_sum_attr ps_attr;
	ps_attr.discovery_proto_id = PROTO_DISCOV_FT_LB;
	ps_attr.reliable_proto_id = PROTO_RELIABLE;
	addProtocol(PROTO_PUSH_SUM, &push_sum_single_init, (void*) &ps_attr, 0);

	//regist tcp server to accept remote connections
	//addLKProtocol(PROTO_TCP_AGG_SERVER, &simpleTCPServer_init, NULL, TCP_SERVER_EVENT_MAX);

	//regist interest in tcp server event to receive remote requests
	//short ev[2];
	//ev[0] = TCP_SERVER_EVENT_REQUEST;
	//registInterestEvents(PROTO_TCP_AGG_SERVER,ev,1,myId);

	//regist interest in discovery events
	//ev[0] = DISCOV_FT_LB_EV_NEIGH_UP;
	//ev[1] = DISCOV_FT_LB_EV_NEIGH_DOWN;
	//registInterestEvents(PROTO_DISCOV_FT_LB, ev, 2, PROTO_SINGLE_TREE_AGGREGATION);


	//Start lk_runtime
	lk_runtime_start();

	//Start your business

	//begin with request to push_sum to start
	LKRequest req;
	req.proto_origin = myId;
	req.proto_dest = PROTO_PUSH_SUM;
	req.request = REQUEST;
	req.request_type = AGG_INIT;
	agg_meta* meta = aggregation_meta_init(OP_AVG, DEFAULT, 0);
	req.payload = malloc(sizeof(agg_meta));
	req.length = aggregation_serialize_agg_meta(req.payload, meta);

	deliverRequest(&req);

	aggregation_meta_destroy(&meta);
	free(req.payload);
	req.payload = NULL;

	LKTimer timer;
	LKTimer_init(&timer, myId, myId);
	LKTimer_set(&timer, 5*1000*1000, 5*1000*1000);
	setupTimer(&timer);

	int times_to_terminate = 30;
	while(1) {

		queue_t_elem elem;
		memset(&elem,0,sizeof(queue_t_elem));

		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE) {

			lk_log("APP", "WARNING", "Received a message");

		} else if(elem.type == LK_TIMER){
			req;
			req.proto_origin = myId;
			req.proto_dest = PROTO_PUSH_SUM;
			req.request = REQUEST;
			req.request_type = AGG_GET;

			req.payload = NULL;
			req.length = 0;

			deliverRequest(&req);

			times_to_terminate --;

		} else if(elem.type == LK_EVENT) {


		} else if(elem.type == LK_REQUEST) {

			if(elem.data.request.proto_origin == PROTO_PUSH_SUM && elem.data.request.request == REPLY && elem.data.request.request_type == AGG_VAL){

				if(elem.data.request.payload != NULL){
					double result;
					memcpy(&result, elem.data.request.payload, sizeof(double));
					char s[200];
					bzero(s, 200);
					sprintf(s, "value now is %f", result);

					lk_log("APP", "GOT VALUE", s);

					free(elem.data.request.payload);
					elem.data.request.payload = NULL;
				}else{
					lk_log("APP","WARNING","Got unexepted reply");
				}
			}

			lk_log("APP", "ALIVE", "Got a reply!");


		} else {
			lk_log("APP","WARNING","Got something unexepted");
		}
	}


	return 0;
}
