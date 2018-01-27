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

#include "discoveryPartial.h"


extern neighbors neighs;
extern time_t lastPeriod;


void* discoveryPartial_init(void* arg) { //argument is the time for the anuncement

	proto_args* pargs = arg;

	time_t anuncePeriod = (long)pargs->protoAttr;
	queue_t* inBox = pargs->inBox;
	short protoID = pargs->protoID;

	neighs.known = 0;
	neighs.neighbors = malloc(sizeof(neigh)*DEFAULT_NEIGHS_SIZE);
	neighs.maxNeigh = DEFAULT_NEIGHS_SIZE;
	memcpy(neighs.myuuid,pargs->myuuid, UUID_SIZE);

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

				lastPeriod = lastPeriod * 2;
				LKTimer_set(&anunce, lastPeriod * 1000 * 1000 + (rand() % 2 * 1000 * 1000), 0);
				memcpy(&elem.data.timer, &anunce, sizeof(LKTimer));
				setupTimer(&elem.data.timer);

			}else if(elem.type == LK_MESSAGE) {

				int knowAddr = 0;
				int i;
				for(i = 0; i < neighs.known; i ++){
					if(uuid_compare((const unsigned char *) elem.data.msg.data, (neighs.neighbors + i)->uuid) == 0){
						knowAddr = 1;
						break;
					}
				}

				if(knowAddr != 1) {
					if(neighs.known < neighs.maxNeigh){

						memcpy(&(neighs.neighbors + neighs.known)->addr, &elem.data.msg.srcAddr, WLAN_ADDR_LEN);
						memcpy(&(neighs.neighbors + neighs.known)->uuid, elem.data.msg.data, UUID_SIZE);
						(neighs.neighbors + neighs.known)->id = neighs.known;
						neighs.known = neighs.known + 1;

					} else {

						int r = rand() % 2;
						if(r){

							r = rand() % neighs.known;
							memcpy((neighs.neighbors + r)->addr.data, &elem.data.msg.srcAddr.data, WLAN_ADDR_LEN);
							memcpy(&(neighs.neighbors + r)->uuid, elem.data.msg.data, UUID_SIZE);

						}

					}
					//cancel the times
					elem.type = LK_TIMER;
					LKTimer_set(&anunce, 0, 0);
					memcpy(&elem.data.timer, &anunce, sizeof(LKTimer));
					setupTimer(&elem.data.timer);

					//setup the times for new times
					lastPeriod = anuncePeriod;
					LKTimer_set(&anunce, (rand() % lastPeriod * 1000 * 1000) + 2 * 1000 * 1000, 0);
					setupTimer(&elem.data.timer);
				}
			}else if(elem.type == LK_REQUEST){
				LKRequest req = elem.data.request;
				processLKRequest(&req, protoID);
			}
		}
	}

	return NULL;
}
