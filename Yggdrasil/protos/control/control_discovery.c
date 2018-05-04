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

#include "protos/control/control_discovery.h"

static unsigned short n_neighbor;
static control_neighbor* neighbors;

void* control_discovery_init(void* arg) { //argument is the time for the anuncement

	n_neighbor = 0;
	neighbors = NULL;

	proto_args* pargs = arg;

	time_t anuncePeriod = (long) pargs->protoAttr;
	queue_t* inBox = pargs->inBox;
	short protoID = pargs->protoID;

	LKMessage msg;
	LKMessage_initBcast(&msg, protoID);
	WLANAddr *myAddr = malloc(sizeof(WLANAddr));
	setMyAddr(myAddr);
	LKMessage_addPayload(&msg, (char*) myAddr->data, WLAN_ADDR_LEN);
	free(myAddr);
	char* myIP = NULL;
	while(myIP == NULL || myIP[0] == '\0')
		myIP = (char*) getChannelIpAddress();
	lk_log("CONTROL_DISCOVERY", "INIT", myIP);
	LKMessage_addPayload(&msg, myIP, 16);

	dispatch(&msg);

	LKTimer announce;
	LKTimer_init(&announce, protoID, protoID);
	LKTimer_set(&announce, anuncePeriod * 1000 * 1000, 0);

	time_t lastPeriod = anuncePeriod;

	setupTimer(&announce);

	queue_t_elem elem;

	int continue_operation = 1;

	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_TIMER){
			//lk_log("CONTROL_DISCOVERY", "ANOUUNCE", "");
			if(continue_operation) {
				dispatch(&msg);
				if(lastPeriod * 2 < 3600)
					lastPeriod = lastPeriod * 2;
				LKTimer_set(&announce, lastPeriod * 1000 * 1000 + (rand() % 2 * 1000 * 1000), 0);
				setupTimer(&announce);
			}

		}else if(elem.type == LK_MESSAGE) {
			LKMessage* recvMsg = &(elem.data.msg);
			control_neighbor* candidate = malloc(sizeof(control_neighbor));

			memcpy(candidate->mac_addr.data, recvMsg->data, WLAN_ADDR_LEN);
			memcpy(candidate->ip_addr, recvMsg->data+WLAN_ADDR_LEN, 16);

			int knownAddr = 0;
			control_neighbor* n = neighbors;
			while(knownAddr == 0 && n != NULL) {
				if(memcmp(n->mac_addr.data, candidate->mac_addr.data, WLAN_ADDR_LEN) == 0)
					knownAddr = 1;
				n = n->next;
			}

			if(knownAddr == 0) {
				candidate->next = neighbors;
				neighbors = candidate;
				n_neighbor++;

				if(continue_operation) {
					//cancel the times
					LKTimer_set(&announce, 0, 0);
					setupTimer(&announce);

					//setup the times for new times
					lastPeriod = anuncePeriod;
					LKTimer_set(&announce, (rand() % lastPeriod * 1000 * 1000) + 2 * 1000 * 1000, 0);
					setupTimer(&announce);
				}

				LKEvent event;
				event.proto_origin = protoID;
				event.notification_id = NEW_NEIGHBOR_IP_NOTIFICATION;
				event.length = 16;
				event.payload = candidate->ip_addr;

				deliverEvent(&event);
			}

		}else if(elem.type == LK_REQUEST){
			if(elem.data.request.request == REQUEST) {
				if(elem.data.request.request_type == DISABLE_DISCOVERY) {
					lk_log("CONTROL_DISCOVERY", "LK_REQUEST RECEIVED", "DISABLE DISCOVERY");
					if(continue_operation != 0) {
						continue_operation = 0;
						LKTimer_set(&announce, 0, 0);
						setupTimer(&announce);

						elem.data.request.request_type = DISPATCH_SHUTDOWN;
						elem.data.request.proto_dest = 0; //Dispatch
						deliverRequest(&elem.data.request);
					}

				} else if (elem.data.request.request_type == ENABLE_DISCOVERY) {
					lk_log("CONTROL_DISCOVERY", "LK_REQUEST RECEIVED", "ENABLE DISCOVERY");
					if(continue_operation != 1) {
						continue_operation = 1;
						//setup the times for new times
						lastPeriod = anuncePeriod;
						LKTimer_set(&announce, (rand() % lastPeriod * 1000 * 1000) + 2 * 1000 * 1000, 0);
						setupTimer(&announce);
					}
				}
			}
		}
	}

	return NULL;
}
