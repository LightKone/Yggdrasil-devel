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

#include "discoveryFull.h"


extern neighbors neighs;
extern time_t lastPeriod;


int discoveryFull_init(void* arg) { //argument is the time for the anuncement

	proto_args* pargs = arg;

	time_t anuncePeriod = (long)pargs->protoAttr;
	queue_t* inBox = pargs->inBox;
	short protoID = pargs->protoID;
	memcpy(neighs.myuuid, pargs->myuuid, UUID_SIZE);

	neighs.known = 0;
	neighs.neighbors = malloc(sizeof(neigh)*1);
	neighs.maxNeigh = 1;

	LKMessage msg;

	setDestToBroadcast(&msg);

	msg.LKProto = protoID;

	msg.dataLen = UUID_SIZE;
	memcpy(msg.data, neighs.myuuid, UUID_SIZE);

	dispatch(&msg);

	LKTimer anunce;
	LKTimer_init(&anunce, protoID, protoID);
	LKTimer_set(&anunce, anuncePeriod * 1000 * 1000, 0);

	lastPeriod = anuncePeriod;

	setupTimer(&anunce);

	queue_t_elem elem;

	while(1){
		if(queue_totalSize(inBox) > 0) {
			memset(&msg, 0, sizeof(msg));
			memset(&elem, 0, sizeof(elem));
			queue_pop(inBox, &elem);

			if(elem.type == LK_TIMER){

				setDestToBroadcast(&msg);

				msg.LKProto = protoID;

				msg.dataLen = UUID_SIZE;
				memcpy(msg.data, &neighs.myuuid, UUID_SIZE);

				dispatch(&msg);
				lk_log("DISCOVERY", "ANUNCE", "");

				lastPeriod = lastPeriod * 2;
				anunce.config.first_notification = time(NULL) + lastPeriod + (rand() % 2);
				memcpy(&elem.data.timer, &anunce, sizeof(LKTimer));
				setupTimer(&elem.data.timer);

			}else if(elem.type == LK_MESSAGE) {

				char s[33];
				memset(s, 0, 33);
				lk_log("DISCOVERY", "RECVANUNCE", wlan2asc(&elem.data.msg.srcAddr, s));
				int knowAddr = 0;
				int i;
				for(i = 0; i < neighs.known; i ++){
					if(uuid_compare((const unsigned char *) elem.data.msg.data, (neighs.neighbors + i)->uuid) == 0){
						knowAddr = 1;
						break;
					}
				}

				if(knowAddr != 1) {
					lk_log("DISCOVERY", "NEWNEIGH", s);
					if(neighs.known < neighs.maxNeigh){

						memcpy(&(neighs.neighbors + neighs.known)->addr, &elem.data.msg.srcAddr, WLAN_ADDR_LEN);
						memcpy(&(neighs.neighbors + neighs.known)->uuid, elem.data.msg.data, UUID_SIZE);
						(neighs.neighbors + neighs.known)->id = neighs.known;
						neighs.known = neighs.known + 1;

					} else {

						neighs.maxNeigh = neighs.maxNeigh*2;
						neighs.neighbors = realloc(neighs.neighbors, sizeof(neigh)*(neighs.maxNeigh));
						memcpy(&(neighs.neighbors + neighs.known)->addr, &elem.data.msg.srcAddr, WLAN_ADDR_LEN);
						memcpy(&(neighs.neighbors + neighs.known)->uuid, elem.data.msg.data, UUID_SIZE);
						(neighs.neighbors + neighs.known)->id = neighs.known;
						neighs.known = neighs.known + 1;


					}
					char k[12];
					memset(k, 0, 12);
					sprintf(k, "%d", neighs.known);
					lk_log("DISCOVERY", "NNEIGH", k);
					//cancel the times
					elem.type = LK_TIMER;
					LKTimer_set(&anunce, 0, 0);
					memcpy(&elem.data.timer, &anunce, sizeof(LKTimer));
					setupTimer(&elem.data.timer);

					//setup the times for new times
					lastPeriod = anuncePeriod;
					LKTimer_set(&elem.data.timer, (rand() % lastPeriod * 1000 * 1000) + 2 * 1000 * 1000, 0);
					setupTimer(&elem.data.timer);
				}
			}else if(elem.type == LK_REQUEST){
				LKRequest req = elem.data.request;
				processLKRequest(&req, protoID);
			}
		}
	}
}
