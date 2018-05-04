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

#include "../protos/aggregation/LiMoSense_alg2.h"

#include "../protos/dispatcher/dispatcher_logger.h"
#include "../protos/discovery/connection_manager.h"
#include "../protos/logger/byte_msg_logger.h"


#include "protos/utils/tcpserver.h"
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
#define PROTO_LIMO 208

#define PROTO_MULTI 207

#define PROTO_DISCOV_FT_LB 210

#define PROTO_RELIABLE 101

#define LOGGER 105


int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument

	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);

	if(argc != 3)
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
	attr.discovery_period = atoi(argv[1])*1000*1000;//2*1000*1000; //2 s
	attr.black_list_links = 0;
	attr.msg_lost_per_fault = atoi(argv[2]);//3;
	updateProtoAttr(PROTO_DISCOV_FT_LB, (void*) &attr);

	addProtocol(LOGGER, &byte_logger_init, NULL, 0);
	queue_t* q2 = interceptProtocolQueue(PROTO_DISPATCH, LOGGER);
	updateProtoAttr(LOGGER, (void*) q2);

	//regist aggregation protocols
	short discov = PROTO_DISCOV_FT_LB;
	addProtocol(PROTO_LIMO, &LiMoSense_alg2_init, (void*) &discov, 0);

	//regist tcp server to accept remote connections
	//	addLKProtocol(PROTO_TCP_AGG_SERVER, &simpleTCPServer_init, NULL, TCP_SERVER_EVENT_MAX);
	//
	//	//regist interest in tcp server event to receive remote requests
	short ev[2];
	//	ev[0] = TCP_SERVER_EVENT_REQUEST;
	//	registInterestEvents(PROTO_TCP_AGG_SERVER,ev,1,myId);

	//regist interest in discovery events
	ev[0] = DISCOV_FT_LB_EV_NEIGH_DOWN;
	ev[1] = DISCOV_FT_LB_EV_NEIGH_UP;
	registInterestEvents(PROTO_DISCOV_FT_LB, ev, 2, PROTO_LIMO);

	ev[0] = CHANGE_VAL;
	registInterestEvents(control_app, ev, 1, PROTO_LIMO);


	//Start lk_runtime
	lk_runtime_start();

	//Start your business

	//begin with request to push_sum to start
	LKRequest req;

	LKTimer timer;
	LKTimer_init(&timer, myId, myId);
	LKTimer_set(&timer, 1*1000*1000, 1*1000*1000);
	setupTimer(&timer);

	char s[50];
	bzero(s, 50);
	sprintf(s, "%s %s", argv[1], argv[2]);

	lk_log("LIMOSENSE", "PARAMS", s);
	lk_log("LIMOSENSE", "SRDS18", "round my_val agg_val");

	unsigned int round = 1;
	while(1) {

		queue_t_elem elem;
		memset(&elem,0,sizeof(queue_t_elem));

		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE) {

			lk_log("APP", "WARNING", "Received a message");

		} else if(elem.type == LK_TIMER){

			req.proto_origin = myId;
			req.proto_dest = PROTO_LIMO;
			req.request = REQUEST;
			req.request_type = AGG_GET;

			req.payload = NULL;
			req.length = 0;

			deliverRequest(&req);

		} else if(elem.type == LK_EVENT) {
			if(elem.data.event.proto_origin == PROTO_TCP_AGG_SERVER && elem.data.event.notification_id == TCP_SERVER_EVENT_REQUEST){
				int command_code;
				int command_arg;

				int client;
				memcpy(&client, elem.data.event.payload, sizeof(int));
				memcpy(&command_code, elem.data.event.payload + sizeof(int),  sizeof(int));
				memcpy(&command_arg, elem.data.event.payload + sizeof(int) + sizeof(int), sizeof(int));

				switch(command_code){
				case 1: //change link;
					req.proto_dest = PROTO_TOPOLOGY_MANAGER;
					req.request = REQUEST;
					req.request_type = CHANGE_LINK;
					if(req.payload != NULL){
						free(req.payload);
					}
					req.payload = malloc(sizeof(int));
					memcpy(req.payload, &command_arg, sizeof(int));
					req.length = sizeof(int);

					deliverRequest(&req);

					break;
				default:
					lk_log("APP", "WARNING", "COMMAND NOT SUPPORTED");
					break;
				}

				if(req.payload != NULL){
					free(req.payload);
					req.payload = NULL;
				}

			}

			if(elem.data.event.payload != NULL){
				free(elem.data.event.payload);
				elem.data.event.payload = NULL;
			}



		} else if(elem.type == LK_REQUEST) {

			if(elem.data.request.proto_origin == PROTO_LIMO && elem.data.request.request == REPLY && elem.data.request.request_type == AGG_VAL){

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

					lk_log("LIMOSENSE", "SRDS18", s);
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
