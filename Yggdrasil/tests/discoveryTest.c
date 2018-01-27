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
#include "protos/discovery/discoveryPartial.h"
#include "protos/discovery/discoveryFull.h"
#include "protos/discovery/discoveryFullFT.h"
#include "protos/faultdetection/faultdetector.h"

#include <stdlib.h>


int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument
	int t = atoi(argv[1]);
	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);


	printf("Setup done\n");
	queue_t *inBox;

	//Init lk_runtime and protocols
	lk_runtime_init(ntconf, 1, 0, 1);

	short myId = registerApp(&inBox, 0);

	//choose your flavor
	addLKProtocol(PROTO_DISCOV, discoveryPartial_init,(void*) 2, 0); //only supports up to 10 neighbors
	//	addLKProtocol(PROTO_DISCOV, &discoveryFull_init,(void*) 2, 0); //supports infinite neighbors
	//	addLKProtocol(PROTO_DISCOV, &discoveryFullFT_init,(void*) 2, 0); //supports infinite neighbors, and works with faultdetector to remove suspect neighbors

	//configuration for faultdetector:
	//	addLKProtocol(PROTO_FAULT_DECT, &faultdetector_init, (void*) 10000, FT_EV_MAX);
	//	queue_t* inter = interceptProtocolQueue(PROTO_DISPATCH, PROTO_FAULT_DECT);
	//	ft_attr fdattr;
	//	fdattr.intercept = inter;
	//	fdattr.timeout_failure_ms = 10000;
	//	updateProtoAttr(PROTO_FAULT_DECT, (void *) &fdattr);
	//
	//	short ev[1];
	//	ev[0] = FT_SUSPECT;
	//	registInterestEvents(PROTO_FAULT_DECT,ev,1,PROTO_DISCOV);

	//Start lk_runtime
	lk_runtime_start();

	LKMessage message;

	struct timespec nextoperation;
	gettimeofday((struct timeval *) &nextoperation,NULL);
	nextoperation.tv_sec += t / 1000000;
	nextoperation.tv_nsec += (t % 1000000) * 1000;

	int sequence_number = 0;
	while(1) {

		queue_t_elem elem;
		memset(&elem,0,sizeof(queue_t_elem));
		memset(&message, 0, sizeof(LKMessage));

		int obtained = queue_try_timed_pop(inBox, &nextoperation, &elem);


		if(obtained > 0) {
			if(elem.type == LK_MESSAGE) {
				char origin[33], dest[33];
				memset(origin,0,33);
				memset(dest,0,33);

				char m[2000];
				memset(m, 0, 2000);
				sprintf(m, "Got a message from %s to %s with content: %s", wlan2asc(&elem.data.msg.srcAddr, origin), wlan2asc(&elem.data.msg.destAddr, dest), elem.data.msg.data);
				lk_log("APP", "RECVMSG", m);

			} else if(elem.type == LK_REQUEST) {
				if(elem.data.request.proto_origin == PROTO_DISCOV){
					if(elem.data.request.request == REPLY){
						if(elem.data.request.request_type == DISC_REQ_GET_RND_NEIGH){

							LKRequest req = elem.data.request;
							int reply = 0;
							if(req.length < sizeof(int) || req.payload == NULL){
								lk_log("APP", "WARNING", "Received invalid format request from discovery");
							}else{
								memcpy(&reply, req.payload, sizeof(int));
								if(reply > 0){

									neigh* n = malloc(sizeof(neigh)*reply);
									neigh* tmp = n;
									int nneighs = 0;
									void* payload = (req.payload + sizeof(int));
									int len = req.length - sizeof(int);
									int i;
									for(i = 0; i < reply && (len - sizeof(neigh)) >= 0; i++){
										memcpy(tmp, payload, sizeof(neigh));
										tmp++;
										payload += sizeof(neigh);
										len -= sizeof(neigh);
										nneighs ++;
									}

									sequence_number++;
									char buffer[MAX_PAYLOAD];
									memset(buffer, 0, MAX_PAYLOAD);
									sprintf(buffer,"This is a lightkone message with sequence: %d",sequence_number);

									message.LKProto = myId;
									len = strlen(buffer);
									message.dataLen = len+1;
									memcpy(message.data, buffer, len+1);
									tmp = n;
									for(i = 0; i < nneighs; i++){
										setDestToAddr(&message, (char*) tmp->addr.data);
										dispatch(&message);

										char s[33];
										memset(s, 0, 33);
										char m[2000];
										memset(m, 0, 2000);
										sprintf(m, "Sent msg %s, to %s\n", message.data, wlan2asc(&message.destAddr, s));
										lk_log("APP", "SENDMSG", m);
									}

									free(req.payload);
									req.payload = NULL;

								}else{
									lk_log("APP", "DISCOVERY REPLY", "No neighbors");
								}

							}
						}else if(elem.data.request.request_type == DISC_REQ_GET_N_NEIGHS){
							int known = 0;
							LKRequest req = elem.data.request;
							if(req.length < sizeof(int) || req.payload == NULL){
								lk_log("APP", "WARNING", "Received invalid format request from discovery");
							}else{

								memcpy(&known, req.payload, sizeof(int));

								char m[2000];
								memset(m, 0, 2000);
								sprintf(m,"Known neighbors: %d\n", known);

								lk_log("APP", "DISCOVERY REPLY", m);

								free(req.payload);
								req.payload = NULL;
							}
						}
					}
				}
			} else {
				lk_log("APP","WARNING","Got something unexepted");
			}
		} else {

			LKRequest req;
			req.proto_origin = myId;
			req.proto_dest = PROTO_DISCOV;
			req.request = REQUEST;
			req.request_type = DISC_REQ_GET_RND_NEIGH;
			req.length = 0;
			req.payload = NULL;
			deliverRequest(&req);

			req.request_type = DISC_REQ_GET_N_NEIGHS;
			deliverRequest(&req);

			lk_log("APP", "QUERIED DISCOVERY", "");

			//Reset maximum waiting time...
			gettimeofday((struct timeval *)&nextoperation,NULL);
			nextoperation.tv_sec += t / 1000000;
			nextoperation.tv_nsec += (t % 1000000) * 1000;
		}
	}

	return 0;
}

