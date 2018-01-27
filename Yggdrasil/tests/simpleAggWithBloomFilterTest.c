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
#include "protos/aggregation/simpleAggregation.h"
#include "protos/aggregation/simpleAggregationWithBloomFilter.h"
#include "protos/utils/tcpserver.h"
#include "protos/utils/topologyManager.h"

#include <stdlib.h>

client_request client_req[8];

int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument

	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);

	queue_t *inBox;

	int k;
	for(k = 0; k < 8; k++){
		client_request_init(&client_req[k]);
	}

	//Init lk_runtime and protocols
	lk_runtime_init(ntconf, 3, 0, 1);

	short myId = registerApp(&inBox, 0);
	addLKProtocol(PROTO_SIMPLE_AGG, &simpleAggregationWithBloomFilter_init, NULL, SIMPLE_AGG_EVENT_MAX);
	addLKProtocol(PROTO_TCP_AGG_SERVER, &simpleTCPServer_init, NULL, TCP_SERVER_EVENT_MAX);
	addLKProtocol(PROTO_TOPOLOGY_MANAGER, &topologyManager_init, NULL, 0);

	short ev[1];
	ev[0] = SIMPLE_AGG_EVENT_REQUEST_VALUE;
	registInterestEvents(PROTO_SIMPLE_AGG,ev,1,myId);
	ev[0] = TCP_SERVER_EVENT_REQUEST;
	registInterestEvents(PROTO_TCP_AGG_SERVER,ev,1,myId);

	const char* hostname = getHostname();
	double value = atoi(hostname+(strlen(hostname)-2));

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

			if(elem.data.event.proto_origin == PROTO_SIMPLE_AGG){
				if(elem.data.event.notification_id == SIMPLE_AGG_EVENT_REQUEST_VALUE){
					LKRequest reply;
					reply.proto_dest = PROTO_SIMPLE_AGG;
					reply.proto_origin = myId;
					reply.request = REPLY;
					reply.request_type = SIMPLE_AGG_REPLY_FOR_VALUE;
					reply.payload = malloc(sizeof(double));
					memcpy(reply.payload, &value, sizeof(double));
					reply.length = sizeof(double);
					deliverReply(&reply);
					free(reply.payload);
				}
			} else if(elem.data.event.proto_origin == PROTO_TCP_AGG_SERVER) {

				if(elem.data.event.notification_id == TCP_SERVER_EVENT_REQUEST){

					int op = -1;
					int client;
					memcpy(&client, elem.data.event.payload, sizeof(int));
					memcpy(&op, elem.data.event.payload+sizeof(int), sizeof(int));
					if(op > -1 && op < SIMPLE_AGG_REPLY_FOR_VALUE){
						LKRequest req;
						req.proto_origin = myId;
						req.proto_dest = PROTO_SIMPLE_AGG;
						req.request = REQUEST;
						req.request_type = op;
						req.length = 0;
						deliverRequest(&req);

						if(op > SIMPLE_AGG_REQ_DO_AVG){

							client_request_addClient(&client_req[op], client);
						}else{
							req.proto_dest = PROTO_TCP_AGG_SERVER;
							req.request = REPLY;
							req.request_type = TCP_SERVER_RESPONSE;
							req.length = sizeof(int)+sizeof(int);
							req.payload = malloc(sizeof(int)*2);
							memcpy(req.payload, &client, sizeof(int));
							int p = SUCCESS;
							memcpy(req.payload + sizeof(int), &p, sizeof(int));
							deliverReply(&req);

							free(req.payload);
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
			if(elem.data.request.proto_origin == PROTO_SIMPLE_AGG){
				if(elem.data.request.request == REPLY){
					lk_log("APP", "ALIVE", "got reply from agg");
					switch(elem.data.request.request_type){
					case SIMPLE_AGG_REQ_GET_SUM:
						client_request_respondToClient(&client_req[SIMPLE_AGG_REQ_GET_SUM], elem.data.request.payload, elem.data.request.length);
						break;
					case SIMPLE_AGG_REQ_GET_MAX:
						client_request_respondToClient(&client_req[SIMPLE_AGG_REQ_GET_MAX], elem.data.request.payload, elem.data.request.length);
						break;
					case SIMPLE_AGG_REQ_GET_MIN:
						client_request_respondToClient(&client_req[SIMPLE_AGG_REQ_GET_MIN], elem.data.request.payload, elem.data.request.length);
						break;
					case SIMPLE_AGG_REQ_GET_AVG:
						client_request_respondToClient(&client_req[SIMPLE_AGG_REQ_GET_AVG], elem.data.request.payload, elem.data.request.length);
						break;
					default:
						lk_log("APP", "GOT UNDEFINED", "");
						break;
					}
				}
			}
		} else {
			lk_log("APP","WARNING","Got something unexepted");
		}
	}


	return 0;
}
