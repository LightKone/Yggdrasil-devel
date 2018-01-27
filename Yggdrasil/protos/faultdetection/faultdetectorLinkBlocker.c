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

#include "faultdetectorLinkBlocker.h"

static short myProtoID;

typedef struct _lk_ft_neighbors {
	uuid_t uuid;
	WLANAddr addr;
	long lastCall;
	short faults;
	struct _lk_ft_neighbors* next;
} lk_ft_neigh;

static lk_ft_neigh* blocked = NULL;

static lk_ft_neigh* quarentine = NULL;

static lk_ft_neigh* database = NULL;

static short threshold = 5;
static int off = 30;
static unsigned long retryTime = 60*30;

static void setTimer(LKTimer* timer, unsigned long time_ms, unsigned long repeat_ms){
	LKTimer_init(timer, myProtoID, myProtoID);
	LKTimer_set(timer, time_ms * 1000, repeat_ms * 1000);

	setupTimer(timer);
}

static void sendBlockRequest(lk_ft_neigh* elem){
	LKRequest req;
	req.proto_dest = PROTO_DISPATCH;
	req.proto_origin = myProtoID;
	req.request = REQUEST;
	req.request_type = DISPATCH_IGNORE_REG;
	dispatcher_serializeIgReq(IGNORE, elem->addr, &req);
	deliverRequest(&req);
	free(req.payload);
}

static void sendUnblockRequest(lk_ft_neigh* elem){
	LKRequest req;
	req.proto_dest = PROTO_DISPATCH;
	req.proto_origin = myProtoID;
	req.request = REQUEST;
	req.request_type = DISPATCH_IGNORE_REG;
	dispatcher_serializeIgReq(NOT_IGNORE, elem->addr, &req);
	deliverRequest(&req);
	free(req.payload);
}

static int checkTryAgain(lk_ft_neigh* n){
	return (n->lastCall + retryTime) - time(NULL);
}

static void checkBlocked(){

	while(blocked != NULL && checkTryAgain(blocked) <  0){
		lk_ft_neigh* tmp = blocked;
		blocked = tmp->next;
		tmp->next = NULL;

		sendUnblockRequest(tmp);
		free(tmp);

	}
	if(blocked != NULL){
		lk_ft_neigh* tmp = blocked;

		while(tmp->next != NULL){
			if(checkTryAgain(tmp->next) < 0){
				lk_ft_neigh* tmp2 = tmp->next;
				tmp->next = tmp2->next;
				tmp2->next = NULL;

				sendUnblockRequest(tmp2);
				free(tmp2);
			}else{
				tmp = tmp->next;
			}

		}

	}

}

static void blockNeigh(lk_ft_neigh* elem){

#ifdef DEBUG
	char msg[2000];
	memset(msg, 0, 2000);
	char u[37];
	uuid_unparse(elem->uuid, u);
	sprintf(msg, "blocking %s", u);
	lk_log("====> LINK BLOCKER", "DEBUG", msg);
#endif

	if(blocked == NULL){
		blocked = elem;
		blocked->lastCall = time(NULL);
		sendBlockRequest(elem);
	}else{
		lk_ft_neigh* tmp = blocked;
		if(uuid_compare(tmp->uuid, elem->uuid) == 0){

#ifdef DEBUG
			char msg[2000];
			memset(msg, 0, 2000);
			char u[37];
			uuid_unparse(elem->uuid, u);
			sprintf(msg, "Got a request to block a guy already blocked... %s", u);
			lk_log("====> LINK BLOCKER", "WARNING", msg);
#endif

			free(elem);
			return;
		}
		while(tmp->next != NULL){
			if(uuid_compare(tmp->next->uuid, elem->uuid) == 0){
				//got into quarantine and back alive
#ifdef DEBUG
				char msg[2000];
				memset(msg, 0, 2000);
				char u[37];
				uuid_unparse(elem->uuid, u);
				sprintf(msg, "Got a request to block a guy already blocked... %s", u);
				lk_log("====> LINK BLOCKER", "WARNING", msg);
#endif

				free(elem);
				return;
			}
			tmp = tmp->next;
		}
		if(tmp->next == NULL) {
			elem->lastCall = time(NULL);
			tmp->next = elem;
			sendBlockRequest(elem);
		}
	}

}

static int checkTime(lk_ft_neigh* n){
	return (n->lastCall + off) - time(NULL);
}

static void checkQuarantine(){
	while(quarentine != NULL && checkTime(quarentine) <  0){
		lk_ft_neigh* tmp = quarentine;
		quarentine = tmp->next;
		tmp->next = NULL;

		if(tmp->faults > threshold){
			blockNeigh(tmp);
		}else{
#ifdef DEBUG
			char msg[2000];
			memset(msg, 0, 2000);
			char u[37];
			uuid_unparse(tmp->uuid, u);
			sprintf(msg, "removing %s from quarantine", u);
			lk_log("====> LINK BLOCKER", "DEBUG", msg);
#endif
			free(tmp);
		}
	}
	if(quarentine != NULL){
		lk_ft_neigh* tmp = quarentine;

		while(tmp->next != NULL){
			if(checkTime(tmp->next) < 0){
				lk_ft_neigh* tmp2 = tmp->next;
				tmp->next = tmp2->next;
				tmp2->next = NULL;

				if(tmp2->faults > threshold)
					blockNeigh(tmp2);
				else{
#ifdef DEBUG
					char msg[2000];
					memset(msg, 0, 2000);
					char u[37];
					uuid_unparse(tmp2->uuid, u);
					sprintf(msg, "removing %s from quarantine", u);
					lk_log("====> LINK BLOCKER", "DEBUG", msg);
#endif
					free(tmp2);
				}
			}else{
				tmp = tmp->next;
			}

		}

	}
}

static void putInQuarantine(lk_ft_neigh* elem) {

#ifdef DEBUG
	char msg[2000];
	memset(msg, 0, 2000);
	char u[37];
	uuid_unparse(elem->uuid, u);
	sprintf(msg, "putting %s in quarantine", u);
	lk_log("====> LINK BLOCKER", "DEBUG", msg);
#endif

	if(quarentine == NULL){
		elem->next = quarentine;
		elem->faults = elem->faults + 1;
		quarentine = elem;
		return;
	}else{
		lk_ft_neigh* tmp = quarentine;
		if(uuid_compare(tmp->uuid, elem->uuid) == 0){
			tmp->faults = tmp->faults + 1;

#ifdef DEBUG
			memset(msg, 0, 2000);
			sprintf(msg, "incrementing %s now with %d", u, tmp->faults);
			lk_log("====> LINK BLOCKER", "DEBUG", msg);
#endif

			free(elem); //Was duplicated in the process can be freed;
			if(tmp->faults > threshold){
				lk_ft_neigh* toBlock = tmp;
				quarentine = toBlock->next;
				toBlock->next = NULL;
				blockNeigh(toBlock);
			}
			return;
		}else{
			while(tmp->next != NULL){
				if(uuid_compare(tmp->next->uuid, elem->uuid) == 0){
					//got into quarantine and back alive
					tmp->next->faults ++;

#ifdef DEBUG
					memset(msg, 0, 2000);
					sprintf(msg, "incrementing %s now with %d", u, tmp->next->faults);
					lk_log("====> LINK BLOCKER", "DEBUG", msg);
#endif

					free(elem); //Was duplicated in the process can be freed;
					if(tmp->next->faults > threshold){
						lk_ft_neigh* toBlock = tmp->next;
						tmp->next = toBlock->next;
						toBlock->next = NULL;
						blockNeigh(toBlock);
					}
					return;
				}
				tmp = tmp->next;
			}
		}

		if(tmp->next == NULL) {
			elem->faults = elem->faults + 1;
			elem->next = NULL;
			tmp->next = elem;
		}
	}
}


void* faultdetectorLinkBlocker_init(void* args) {
	proto_args* pargs = args;
	myProtoID = pargs->protoID;

	ft_lb_attr* attr = (ft_lb_attr*) pargs->protoAttr;
	queue_t* dispatcher_queue = attr->intercept;
	unsigned long timeout_failure_ms = attr->timeout_failure_ms;

	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;

	unsigned short sentInLastInterval = 0;

	off = (timeout_failure_ms/1000) * threshold * 2;

	LKTimer timer;

	setTimer(&timer, timeout_failure_ms / 10, timeout_failure_ms / 10);

	LKTimer gman;

	setTimer(&gman, off * 1000, off * 1000);

	LKTimer unblocker;

	setTimer(&unblocker, retryTime * 1000, retryTime * 1000);

	WLANAddr bcastAddress;
	str2wlan((char*) bcastAddress.data, WLAN_BROADCAST);


	while(1) {
		queue_pop(inBox, &elem);
		if(elem.type == LK_MESSAGE) {
			if(elem.data.msg.LKProto != myProtoID) { //This message is not for me therefore is going to the network
#ifdef DEBUG
				char s[200];
				memset(s,0,200);
				sprintf(s, "Proto src: %d payload size: %d", elem.data.msg.LKProto, elem.data.msg.dataLen);
#endif
				if(sentInLastInterval == 0) {
#ifdef DEBUG
					lk_log("FAULTDETECTOR", "MODIFY", s);
#endif
					pushPayload(&elem.data.msg, (char*) pargs->myuuid, sizeof(uuid_t), myProtoID, &bcastAddress);
					sentInLastInterval = 1;
				} else if (memcmp(bcastAddress.data, elem.data.msg.destAddr.data, WLAN_ADDR_LEN) == 0) {
#ifdef DEBUG
					lk_log("FAULTDETECTOR", "MODIFY BCAST", s);
#endif
					pushPayload(&elem.data.msg, (char*) pargs->myuuid, sizeof(uuid_t), myProtoID, &bcastAddress);
				}else{
#ifdef DEBUG
					lk_log("FAULTDETECTOR", "NOT MODIFY", s);
#endif
				}

			} else { //This message is for me, therefore is comming from the network.
				lk_ft_neigh* temp = malloc(sizeof(lk_ft_neigh));
				temp->next = NULL;
				unsigned short read = popPayload(&elem.data.msg, (char*) temp->uuid, sizeof(uuid_t));
				memcpy(temp->addr.data, elem.data.msg.srcAddr.data, WLAN_ADDR_LEN);

#ifdef DEBUG
				char s[200];
				memset(s,0,200);
				sprintf(s, "Proto src: %d payload size: %d", elem.data.msg.LKProto, elem.data.msg.dataLen);
				lk_log("FAULTDETECTOR", "UNPACK", s);
				char uuid_str2[37];
				memset(uuid_str2,0,37);
				uuid_unparse(temp->uuid, uuid_str2);
				memset(s, 0, 200);
				sprintf(s, "Got a presence message from %s with (mypayload) lenght %u\n", uuid_str2, read);
				lk_log("FAULTDETECTOR", "PRESENCE",s);
#endif

				if(read == sizeof(uuid_t)) {
					if(database == NULL) {
						database = temp;
						temp->lastCall = time(NULL);

						char uuid_str[37];
						memset(uuid_str,0,37);
						uuid_unparse(temp->uuid, uuid_str);

						lk_log("FAULTDETECTOR", "Found a new node", uuid_str);
					} else {
						lk_ft_neigh* t = database;
						if(uuid_compare(t->uuid, temp->uuid) == 0) {
							t->lastCall = time(NULL);
							free(temp);
						} else {
							while(t->next != NULL) {
								if(uuid_compare(t->next->uuid, temp->uuid) == 0) {
									t->next->lastCall = time(NULL);
									free(temp);
									break;
								}
								t = t->next;
							}
							if(t->next == NULL) {
								temp->next = t->next;
								t->next = temp;
								temp->lastCall = time(NULL);

								char uuid_str[37];
								memset(uuid_str,0,37);
								uuid_unparse(temp->uuid, uuid_str);

								lk_log("FAULTDETECTOR", "Found a new node", uuid_str);
							}
						}
					}
				}

				if(elem.data.msg.LKProto != myProtoID) {

					filterAndDeliver(&elem.data.msg);
				}
			}
		} else if(elem.type == LK_TIMER) {

			if(uuid_compare(elem.data.timer.id, timer.id) == 0){
				if(sentInLastInterval != 0) {
					sentInLastInterval = 0;
				} else {
					queue_t_elem msg;
					msg.type = LK_MESSAGE;
					msg.data.msg.LKProto = myProtoID;
					setDestToBroadcast(&msg.data.msg);
					msg.data.msg.dataLen = 0;

					pushPayload(&msg.data.msg, (char*) pargs->myuuid, sizeof(uuid_t), myProtoID, &bcastAddress);

					queue_push(dispatcher_queue, &msg);
				}

				//Check which neighbors are suspected.
				long now = time(NULL);

				while(database != NULL && database->lastCall + (timeout_failure_ms/1000) < now) {
					lk_ft_neigh* toRemove = database;
					database = toRemove->next;
					toRemove->next = NULL;

					LKEvent neighborDownNotification;
					neighborDownNotification.proto_origin = myProtoID;
					neighborDownNotification.notification_id = FT_LB_SUSPECT;
					neighborDownNotification.length = sizeof(uuid_t);

					neighborDownNotification.payload = malloc(sizeof(uuid_t));

					memcpy(neighborDownNotification.payload, toRemove->uuid, sizeof(uuid_t));

					char uuid_str[37];
					memset(uuid_str,0,37);

					uuid_unparse((const unsigned char *) neighborDownNotification.payload, uuid_str);

					lk_log("FAULTDETECTOR", "Suspecting a node", uuid_str);

					deliverEvent(&neighborDownNotification);

					free(neighborDownNotification.payload);

					putInQuarantine(toRemove);
				}

				if(database != NULL) {

					lk_ft_neigh* db = database;
					while(db->next != NULL) {

						char uuid_str[37];
						memset(uuid_str,0,37);

						if(db->next->lastCall + (timeout_failure_ms/1000) < now) { //Timeout

							lk_ft_neigh* toRemove = db->next;
							db->next = toRemove->next;
							toRemove->next = NULL;

							LKEvent neighborDownNotification;
							neighborDownNotification.proto_origin = myProtoID;
							neighborDownNotification.notification_id = FT_LB_SUSPECT;
							neighborDownNotification.length = sizeof(uuid_t);

							neighborDownNotification.payload = malloc(sizeof(uuid_t));

							memcpy(neighborDownNotification.payload, toRemove->uuid, sizeof(uuid_t));

							uuid_unparse((const unsigned char *) neighborDownNotification.payload, uuid_str);

							lk_log("FAULTDETECTOR", "Suspecting a node", uuid_str);


							deliverEvent(&neighborDownNotification);

							free(neighborDownNotification.payload);

							putInQuarantine(toRemove);

						} else {
							db = db->next;
						}
					}
				}
			}else if(uuid_compare(elem.data.timer.id, gman.id) == 0){
				checkQuarantine();
			} else if(uuid_compare(elem.data.timer.id, unblocker.id) == 0){
				checkBlocked();
			}
		} else {
			char s[40];
			memset(s, 0, 40);
			sprintf(s,"Unexpected event type: %d received", elem.type);
			lk_log("FAULTDETECTOR", "WARNING", s);
			queue_push(dispatcher_queue, &elem);
		}
	}

}
