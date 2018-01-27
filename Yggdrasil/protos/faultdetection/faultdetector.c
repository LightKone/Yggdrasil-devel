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

#include "faultdetector.h"


typedef struct _lk_ft_neighbors {
	uuid_t uuid;
	long lastCall;
	struct _lk_ft_neighbors* next;
} lk_ft_neigh;



void* faultdetector_init(void* args) {
	proto_args* pargs = args;
	short myProtoID = pargs->protoID;

	ft_attr* attr = (ft_attr*) pargs->protoAttr;
	queue_t* dispatcher_queue = attr->intercept;
	unsigned long timeout_failure_ms = attr->timeout_failure_ms;

	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;

	unsigned short sentInLastInterval = 0;

	lk_ft_neigh* database = NULL;

	elem.type = LK_TIMER;
	LKTimer_init(&elem.data.timer, myProtoID, myProtoID);
	LKTimer_set(&elem.data.timer, timeout_failure_ms * 1000 * 10, timeout_failure_ms * 1000);

	setupTimer(&elem.data.timer);

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
				queue_push(dispatcher_queue, &elem);

			} else { //This message is for me, therefore is comming from the network.
				lk_ft_neigh* temp = malloc(sizeof(lk_ft_neigh));
				temp->next = NULL;
				unsigned short read = popPayload(&elem.data.msg, (char*) temp->uuid, sizeof(uuid_t));
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
			lk_ft_neigh** db = &database;
			while(*db != NULL) {

				char uuid_str[37];
				memset(uuid_str,0,37);

				if((*db)->lastCall + (timeout_failure_ms/1000) < now) { //Timeout
					lk_ft_neigh* toRemove = (*db);
					LKEvent neighborDownNotification;
					neighborDownNotification.proto_origin = myProtoID;
					neighborDownNotification.notification_id = FT_SUSPECT;
					neighborDownNotification.length = sizeof(uuid_t);

					neighborDownNotification.payload = malloc(sizeof(uuid_t));

					memcpy(neighborDownNotification.payload, toRemove->uuid, sizeof(uuid_t));

					uuid_unparse((const unsigned char *) neighborDownNotification.payload, uuid_str);
					char m[2000];
					memset(m, 0, 2000);
					sprintf(m, "Suspecting node: %s", uuid_str);
					lk_log("FAULTDETECTOR", "SUSPECT" ,m);

					deliverEvent(&neighborDownNotification);

					free(neighborDownNotification.payload);

					(*db) = (*db)->next;
					toRemove->next = NULL;
					free(toRemove);
				} else {
					db = &((*db)->next);
				}
			}
		} else {
			char s[40];
			memset(s, 0, 40);
			sprintf(s,"Unexpected event type: %d received", elem.type);
			lk_log("FAULTDETECTOR", "WARNING", s);
		}
	}

}
