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

#include <stdlib.h>


typedef struct addr_list_{
	WLANAddr src;
	struct addr_list_* next;
}addr_lst;

addr_lst* all = NULL;
short all_len = 0;

addr_lst* blocked = NULL;
short blocked_len = 0;

void addToBlocked(WLANAddr src){
	addr_lst* elem = malloc(sizeof(addr_lst));
	memcpy(elem->src.data, src.data, WLAN_ADDR_LEN);
	elem->next = blocked;
	blocked = elem;
	blocked_len ++;
}

addr_lst* rmFromBlocked(){
	addr_lst* elem = blocked;

	if(blocked != NULL){
		blocked = blocked->next;
	}

	blocked_len --;

	return elem;
}

int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument
	int t = atoi(argv[1]);
	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);

	queue_t *inBox;

	//Init lk_runtime and protocols
	lk_runtime_init(ntconf, 0, 0, 1);

	short myId = registerApp(&inBox, 0);

	//Start lk_runtime
	lk_runtime_start();

	//Start your business
	LKMessage message;

	int sequence_number = 0;

	struct timespec nextoperation;
	gettimeofday((struct timeval *) &nextoperation,NULL);
	nextoperation.tv_sec += t / 1000000;
	nextoperation.tv_nsec += (t % 1000000) * 1000;


	LKTimer timer;
	LKTimer_init(&timer, myId, myId);
	LKTimer_set(&timer, 3 * 1000 * 1000, 0);
	setupTimer(&timer);


	while(1) {

		queue_t_elem elem;
		memset(&elem,0,sizeof(queue_t_elem));
		memset(&message, 0, sizeof(LKMessage));


		int obtained = queue_try_timed_pop(inBox, &nextoperation, &elem);

		if(obtained > 0) {
			if(elem.type == LK_TIMER) {

				addr_lst* src = rmFromBlocked();

				char msg[200];
				char s[33];
				memset(msg, 0, 200);
				if(src != NULL){
					sprintf(msg, "%s", wlan2asc(&src->src, s));

					lk_log("APP", "UNBLOCKING SRC", msg);
					LKRequest req;
					req.proto_dest = PROTO_DISPATCH;
					req.proto_origin = myId;
					req.request = REQUEST;
					req.request_type = DISPATCH_IGNORE_REG;
					dispatcher_serializeIgReq(NOT_IGNORE, src->src, &req);
					deliverRequest(&req);
					free(req.payload);
					free(src);
				}

				LKTimer_set(&timer, 3 * 1000 * 1000, 0);
				setupTimer(&timer);
			} else if(elem.type == LK_MESSAGE) {
				char origin[33], dest[33];
				memset(origin,0,33);
				memset(dest,0,33);
				wlan2asc(&elem.data.msg.srcAddr, origin);
				wlan2asc(&elem.data.msg.destAddr, dest);
				char buffer[MAX_PAYLOAD+1];
				memset(buffer, 0, MAX_PAYLOAD+1);

				char desc[33+33+11+15+14+1];
				memset(desc, 0, 33+33+11+15+14+1);
				sprintf(desc, "src: %s :: dest: %s :: sequence: %d",origin, dest, atoi(elem.data.msg.data));
				lk_log("APP", "RECVDMSG", desc);

				lk_log("APP", "BLOCKING SRC", "");
				LKRequest req;
				req.proto_dest = PROTO_DISPATCH;
				req.proto_origin = myId;
				req.request = REQUEST;
				req.request_type = DISPATCH_IGNORE_REG;
				dispatcher_serializeIgReq(IGNORE, elem.data.msg.srcAddr, &req);
				deliverRequest(&req);
				free(req.payload);

				addToBlocked(elem.data.msg.srcAddr);

			} else {
				lk_log("APP","WARNIG","Got something unexepted");
			}
		} else {

			sequence_number++;
			char buffer[MAX_PAYLOAD];
			memset(buffer, 0, MAX_PAYLOAD);
			sprintf(buffer,"%d",sequence_number);
			message.LKProto = myId;
			int len = strlen(buffer);
			message.dataLen = len+1;
			memcpy(message.data, buffer, len+1);
			setDestToBroadcast(&message);
			char s[33];
			memset(s, 0, 33);
			char s2[33];
			memset(s2, 0, 33);

			dispatch(&message);
			char desc[33+33+11+15+14+1];
			memset(desc, 0, 33+33+11+15+14+1);
			sprintf(desc, "src: %s :: dest: %s :: sequence: %d",getMyAddr(s2), wlan2asc(&message.destAddr, s), sequence_number);
			lk_log("APP", "SENDMSG", desc);


			//Reset maximum waiting time...
			gettimeofday((struct timeval *)&nextoperation,NULL);
			nextoperation.tv_sec += t / 1000000;
			nextoperation.tv_nsec += (t % 1000000) * 1000;
		}
	}

	return 0;
}

