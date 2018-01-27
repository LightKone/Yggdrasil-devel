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

#include "protos/discovery/discovery_ft_lb.h"

#include "protos/aggregation/aggregation_interface.h"
#include "protos/aggregation/simple_aggregation.h"
#include "protos/aggregation/simple_bloom_aggregation.h"
#include "protos/aggregation/single_tree_aggregation.h"

#include "protos/utils/tcpserver.h"
#include "protos/utils/topologyManager.h"

#include <stdlib.h>
#include <uuid/uuid.h>


#define PROTO_SIMPLE_AGGREGATION 200
#define PROTO_SIMPLE_BLOOM_AGGREGATION 201
#define PROTO_SINGLE_TREE_AGGREGATION 202

#define PROTO_DISCOV_FT_LB 210

typedef struct agg_proto_cache {
	double value[4]; //last value result for each op;
}agg_cache;

agg_cache cache[3]; //one for each protocol

int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument

	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);

	queue_t *inBox;

	int k, j;
	for(k = 0; k < 3; k++){

		for(j = 0; j < 4; j++){
			cache[k].value[j] = -1;
		}
	}

	//Init lk_runtime and protocols
	lk_runtime_init(ntconf, 2, 4, 1);

	short myId = registerApp(&inBox, 0);

	//regist topology manager to have virtual multihop topology
	addLKProtocol(PROTO_TOPOLOGY_MANAGER, &topologyManager_init, NULL, 0);

	//regist discov
	addProtocol(PROTO_DISCOV_FT_LB, &discovery_ft_lb_init, NULL, DISCOV_FT_LB_EV_MAX);
	queue_t* q = interceptProtocolQueue(PROTO_DISPATCH, PROTO_DISCOV_FT_LB);
	discov_ft_lb_attr attr;
	attr.intercept = q;
	attr.timeout_failure_ms = 10000; //10 seconds?
	updateProtoAttr(PROTO_DISCOV_FT_LB, (void*) &attr);

	//regist aggregation protocols
	addProtocol(PROTO_SIMPLE_AGGREGATION, &simple_aggregation_init, NULL, 0);
	addProtocol(PROTO_SIMPLE_BLOOM_AGGREGATION, &simple_bloom_aggregation_init, NULL, 0);
	addProtocol(PROTO_SINGLE_TREE_AGGREGATION, &stree_init, NULL, 0);

	//regist tcp server to accept remote connections
	addLKProtocol(PROTO_TCP_AGG_SERVER, &simpleTCPServer_init, NULL, TCP_SERVER_EVENT_MAX);

	//regist interest in tcp server event to receive remote requests
	short ev[2];
	ev[0] = TCP_SERVER_EVENT_REQUEST;
	registInterestEvents(PROTO_TCP_AGG_SERVER,ev,1,myId);

	//regist interest in discovery events
	ev[0] = DISCOV_FT_LB_EV_NEIGH_UP;
	ev[1] = DISCOV_FT_LB_EV_NEIGH_DOWN;
	registInterestEvents(PROTO_DISCOV_FT_LB, ev, 2, PROTO_SINGLE_TREE_AGGREGATION);


	//Start lk_runtime
	lk_runtime_start();

	//Start your business

	while(1) {

		queue_t_elem elem;
		memset(&elem,0,sizeof(queue_t_elem));

		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE) {

			lk_log("APP", "WARNING", "Received a message");

		} else if(elem.type == LK_EVENT) {

			if(elem.data.event.proto_origin == PROTO_TCP_AGG_SERVER) {

				if(elem.data.event.notification_id == TCP_SERVER_EVENT_REQUEST){

					int op = -1;
					int proto = -1;
					int client;
					//remote request <client_id, proto_id, operation_id> <x, 1, 1>
					memcpy(&client, elem.data.event.payload, sizeof(int));
					memcpy(&proto, elem.data.event.payload + sizeof(int), sizeof(int));
					memcpy(&op, elem.data.event.payload+sizeof(int) + sizeof(int), sizeof(int));

					if(op > -1 && op < AGG_REPLY_FOR_VALUE){
						LKRequest req;
						req.proto_origin = myId;
						req.proto_dest = 200 + proto; //200; 201; 202; ...
						req.request = REQUEST;

						req.payload = malloc(sizeof(int)); //send client id
						memcpy(req.payload, &client, sizeof(int));
						req.length = sizeof(int);

						if(op > AGG_REQ_DO_AVG){
							if(cache[proto].value[op - AGG_REQ_GET_SUM] != -1){
								//reply to client (value was cached)
								req.proto_dest = PROTO_TCP_AGG_SERVER;
								req.request = REPLY;
								req.request_type = TCP_SERVER_RESPONSE;
								req.length = sizeof(int)+sizeof(int);
								req.payload = malloc(sizeof(int)*2);
								memcpy(req.payload, &client, sizeof(int));

								memcpy(req.payload + sizeof(int), &cache[proto].value[op-AGG_REQ_GET_SUM], sizeof(int));
								deliverReply(&req);

								free(req.payload);
							}else{
								//send request to agg proto
								req.request_type = op - AGG_REQ_GET_SUM; //agg_req_do_*
								deliverRequest(&req);
							}
						}else{
							//send request to agg proto
							req.request_type = op; //agg_req_do_*
							deliverRequest(&req);
						}
					}else{
						char s[2000];
						memset(s, 0, 2000);
						sprintf(s, "Got invalid operation request for simple aggregation, op %d", op);
						lk_log("APP", "WARNING", s);
					}

				}
			}
			if(elem.data.event.payload != NULL)
				free(elem.data.event.payload);

		} else if(elem.type == LK_REQUEST) {

			lk_log("APP", "ALIVE", "Got a reply!");

			LKRequest reply = elem.data.request;
			reply.request = REPLY;
			reply.proto_dest = PROTO_TCP_AGG_SERVER;
			reply.request_type = TCP_SERVER_RESPONSE;
			short proto_origin = reply.proto_origin;

			switch(proto_origin){
			case PROTO_SIMPLE_AGGREGATION:
				//cache[0].value[reply.request_type] =
				deliverReply(&reply);
				break;
			case PROTO_SIMPLE_BLOOM_AGGREGATION:
				deliverReply(&reply);
				break;
			case PROTO_SINGLE_TREE_AGGREGATION:
				deliverReply(&reply);
				break;
			default:
				lk_log("APP", "GOT UNDEFINED", "");
				break;
			}

			free(reply.payload);

		} else {
			lk_log("APP","WARNING","Got something unexepted");
		}
	}


	return 0;
}
