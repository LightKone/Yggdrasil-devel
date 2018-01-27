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
	LKMessage_addPayload(&msg, myIP, 16);

	dispatch(&msg);

	LKTimer announce;
	LKTimer_init(&announce, protoID, protoID);
	LKTimer_set(&announce, anuncePeriod * 1000 * 1000, 0);

	time_t lastPeriod = anuncePeriod;

	setupTimer(&announce);

	queue_t_elem elem;

	while(1){
		if(queue_totalSize(inBox) > 0) {
			queue_pop(inBox, &elem);

			if(elem.type == LK_TIMER){
				dispatch(&msg);

				lk_log("CONTROL_DISCOVERY", "ANNOUNCE", "");

				lastPeriod = lastPeriod * 2;
				LKTimer_set(&announce, lastPeriod * 1000 * 1000 + (rand() % 2 * 1000 * 1000), 0);
				setupTimer(&announce);

			}else if(elem.type == LK_MESSAGE) {
#ifdef DEBUG
				char s[33];
				memset(s, 0, 33);
				lk_log("CONTROL_DISCOVERY", "RECVANUNCE", wlan2asc(&elem.data.msg.srcAddr, s));
#endif

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

					//cancel the times
					LKTimer_set(&announce, 0, 0);
					setupTimer(&announce);

					//setup the times for new times
					lastPeriod = anuncePeriod;
					LKTimer_set(&announce, (rand() % lastPeriod * 1000 * 1000) + 2 * 1000 * 1000, 0);
					setupTimer(&announce);
				}

			}else if(elem.type == LK_REQUEST){
				if(elem.data.request.request == REQUEST && elem.data.request.request_type == CONTROL_DISC_GET_NEIGHBORS) {
					LKRequest* request = &(elem.data.request);
					request->proto_dest = request->proto_origin;
					request->proto_origin = protoID;
					request->request = REPLY;
					request->length = sizeof(unsigned short) + (n_neighbor * (WLAN_ADDR_LEN + 16));
					request->payload = malloc(request->length);
					memcpy(request->payload, &n_neighbor, sizeof(unsigned short));
					control_neighbor* n = neighbors;
					void* data = request->payload + sizeof(unsigned short);
					while(n != NULL) {
						memcpy(data, n->mac_addr.data, WLAN_ADDR_LEN);
						data += WLAN_ADDR_LEN;
						memcpy(data, n->ip_addr, 16);
						data += 16;
						n = n->next;
					}
					deliverReply(request);
					free(request->payload);
				}
			}
		}
	}

	return NULL;
}
