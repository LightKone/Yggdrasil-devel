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
#include "core/utils/utils.h"

//#include "protos/discovery/discovery_ft_lb.h"

#include "protos/aggregation/aggregation_gen_interface.h"

#include "protos/aggregation/gap_gen.h"

#include "protos/aggregation/average/gap_avg_bcast.h"

#include "protos/dispatcher/dispatcher_logger.h"
#include "protos/discovery/connection_manager.h"
#include "protos/logger/byte_msg_logger.h"

#include "protos/utils/topologyManager.h"

#include "protos/apps/control_app.h"

#include <stdlib.h>
#include <uuid/uuid.h>
#include <pthread.h>

#define PROTO_SIMPLE_AGGREGATION 200
#define PROTO_SIMPLE_BLOOM_AGGREGATION 201
#define PROTO_SINGLE_TREE_AGGREGATION 202
#define PROTO_PUSH_SUM 203
#define PROTO_FLOW_UPDATING 204
#define PROTO_DRG 205
#define PROTO_GAP 206

#define PROTO_DISCOV_FT_LB 210

#define PROTO_RELIABLE 101

#define LOGGER 105

int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument

	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);

	if(argc != 4)
		return -1;


	queue_t *inBox;

	//Init lk_runtime and protocols
	lk_runtime_init_static(ntconf, 1, 3, 2);

	changeDispatcherFunction(&dispatcher_logger_init);

	short myId = registerApp(&inBox, 0);

	queue_t* control_queue;
	short control_app = registerApp(&control_queue, 1);
	contro_app_attr capp;
	capp.inBox = control_queue;
	capp.protoId = control_app;
	pthread_t control;
	pthread_attr_t control_attr;
	pthread_attr_init(&control_attr);
	pthread_create(&control, &control_attr, &control_app_init, (void*) &capp);

	//regist topology manager to have virtual multihop topology
	addLKProtocol(PROTO_TOPOLOGY_MANAGER, &topologyManager_init, NULL, 0);

	//regist discov
	addProtocol(PROTO_DISCOV_FT_LB, &connection_manager_init, NULL, DISCOV_FT_LB_EV_MAX);
	queue_t* q = interceptProtocolQueue(PROTO_DISPATCH, PROTO_DISCOV_FT_LB);
	connection_attr attr;
	attr.intercept = q;
	attr.discovery_period = atoi(argv[2])*1000*1000;//2*1000*1000; //2 s
	attr.black_list_links = 0;
	attr.msg_lost_per_fault = atoi(argv[3]);//3;
	updateProtoAttr(PROTO_DISCOV_FT_LB, (void*) &attr);

	addProtocol(LOGGER, &byte_logger_init, NULL, 0);
	queue_t* q2 = interceptProtocolQueue(PROTO_DISPATCH, LOGGER);
	updateProtoAttr(LOGGER, (void*) q2);

	//regist aggregation protocols
	gap_args gargs;
	gargs.discoveryId = PROTO_DISCOV_FT_LB;
	gargs.policy_type = POLICY_DEFAULT;

	char* root_node = argv[1];
	if(strcmp(root_node, getHostname()) == 0)
		gargs.root = 1;
	else
		gargs.root = 0;

	addProtocol(PROTO_GAP, &gap_avg_bcast_init, (void*) &gargs, 0);


	//regist interest in tcp server event to receive remote requests
	short ev[2];

	//regist interest in discovery events
	ev[0] = DISCOV_FT_LB_EV_NEIGH_UP;
	ev[1] = DISCOV_FT_LB_EV_NEIGH_DOWN;
	registInterestEvents(PROTO_DISCOV_FT_LB, ev, 2, PROTO_GAP);

	ev[0] = CHANGE_VAL;
	registInterestEvents(control_app, ev, 1, PROTO_GAP);


	//Start lk_runtime
	lk_runtime_start();

	//Start your business

	//begin with request to push_sum to start
	LKRequest req;

	LKTimer timer;
	LKTimer_init(&timer, myId, myId);
	LKTimer_set(&timer, 1*1000*1000, 1*1000*1000);
	setupTimer(&timer);

	char s[70];
	bzero(s, 70);
	sprintf(s, "%s %s %s", argv[1], argv[2], argv[3]);

	lk_log("GAP BCAST", "PARAMS", s);
	lk_log("GAP BCAST", "SRDS18", "round my_val agg_val");
	unsigned int round = 1;
	while(1) {

		queue_t_elem elem;
		memset(&elem,0,sizeof(queue_t_elem));

		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE) {

			lk_log("APP", "WARNING", "Received a message");

		} else if(elem.type == LK_TIMER){

			req.proto_origin = myId;
			req.proto_dest = PROTO_GAP;
			req.request = REQUEST;
			req.request_type = AGG_GET;

			req.payload = NULL;
			req.length = 0;

			deliverRequest(&req);

		} else if(elem.type == LK_EVENT) {


		} else if(elem.type == LK_REQUEST) {

			if(elem.data.request.proto_origin == PROTO_GAP && elem.data.request.request == REPLY && elem.data.request.request_type == AGG_VAL){

				if(elem.data.request.payload != NULL){
					double myval;
					double result;
					void* payload = elem.data.request.payload;
					memcpy(&myval, payload, sizeof(double));
					payload += sizeof(double);
					memcpy(&result, payload, sizeof(double));
					char s[200];
					bzero(s, 200);
					sprintf(s, "%d %f %f",round, myval, result);

					lk_log("GAP BCAST", "SRDS18", s);
					round ++;
					free(elem.data.request.payload);
					elem.data.request.payload = NULL;
				}else{
					lk_log("APP","WARNING","Got unexepted reply");
				}
			}

			//			lk_log("APP", "ALIVE", "Got a reply!");


		} else {
			lk_log("APP","WARNING","Got something unexepted");
		}
	}


	return 0;
}
