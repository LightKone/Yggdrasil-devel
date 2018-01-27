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

#include "push_gossip.h"

typedef struct _uuid_list {
	uuid_t uuid;
	time_t time;
	struct _uuid_list * next;
}uuid_list;

typedef struct _pending_messages {
	uuid_t uuid;
	LKMessage message;
	struct _pending_messages* next;
} pending_msg;


static uuid_list* messages = NULL;
static int off = 60;
static pending_msg* pending = NULL;

static int haveIseenThis(uuid_t uuid) {
	uuid_list* tmp = messages;
	while(tmp != NULL){

		if(uuid_compare(uuid, tmp->uuid) == 0)
			return 1;
		tmp = tmp->next;
	}

	return 0;
}

static void addMessage(uuid_t uuid){
	uuid_list* u = malloc(sizeof(uuid_list));
	memcpy(u->uuid, uuid, sizeof(uuid_t));
	u->time = time(NULL);

	u->next = messages;
	messages = u;
}

static int checkTime(uuid_list* m){
	return (m->time + off) - time(NULL);
}

static void garbageManExecution() {
	while(messages != NULL && checkTime(messages) <  0){
		uuid_list* tmp = messages;
		messages = tmp->next;

		tmp->next = NULL;
		free(tmp);
	}
	if(messages != NULL){
		uuid_list* tmp = messages;
		while(tmp->next != NULL){
			if(checkTime(tmp->next) < 0){
				uuid_list* tmp2 = tmp->next;
				tmp->next = tmp2->next;

				tmp2->next = NULL;
				free(tmp2);
			}else{
				tmp = tmp->next;
			}

		}
	}
}

static void dispatchPendingMessage(uuid_t msg_id) {

	if(pending != NULL && uuid_compare(msg_id, pending->uuid) == 0) {
#ifdef DEBUG
		lk_log("PUSH_GOSSIP", "DEBUG", "retransmit (1st position)");
#endif
		dispatch(&pending->message);
		pending_msg* t = pending;
		pending = t->next;
		t->next = NULL;
		free(t);
		return;
	} else if (pending != NULL) {
		pending_msg* t = pending;
		while(t->next != NULL) {
			if(uuid_compare(msg_id, t->next->uuid) == 0) {
#ifdef DEBUG
				lk_log("PUSH_GOSSIP", "DEBUG", "retransmit (later position)");
#endif
				dispatch(&t->next->message);
				pending_msg* t2 = t->next;
				t->next = t2->next;
				t2->next = NULL;
				free(t2);
				return;
			} else {
				t = t->next;
			}
		}
	}

	lk_log("PUSH_GOSSIP", "ERROR", "No message retransmitted");
}

void * push_gossip_init(void * args) {

	proto_args* pargs = args;
	push_gossip_args* pgargs = (push_gossip_args*) pargs->protoAttr;

	messages = NULL;

	queue_t* inBox = pargs->inBox;

	WLANAddr bcastAddr;
	setBcastAddr(&bcastAddr);

	LKTimer gMan;
	LKTimer_init(&gMan, pargs->protoID, pargs->protoID);
	LKTimer_set(&gMan, 30 * 1000 * 1000, 30 * 1000 * 1000);

	setupTimer(&gMan);

	queue_t_elem elem;

	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE) {

			LKMessage msg;
			memcpy(&msg, &elem.data.msg, sizeof(LKMessage));
			uuid_t uuid;
			popPayload(&msg, (char*) uuid, sizeof(uuid_t));

			if(haveIseenThis(uuid) == 0){
				addMessage(uuid);
				deliver(&msg);

				pushPayload(&msg, (char*) uuid, sizeof(uuid_t), pargs->protoID, &bcastAddr);

				if(pgargs->avoidBCastStorm == 1) {
					pending_msg* t = malloc(sizeof(pending_msg));
					memcpy(&t->message, &msg,sizeof(LKMessage));
					memcpy(&t->uuid, uuid, sizeof(uuid_t));
					t->next = pending;
					pending = t;

					LKTimer mt;
					LKTimer_init_with_uuid(&mt, uuid, pargs->protoID, pargs->protoID);
					LKTimer_set(&mt, rand() % 3000000, 0);
#ifdef DEBUG
					lk_log("PUSH_GOSSIP", "DEBUG", "Setting timer for later retransmission");
#endif
					setupTimer(&mt);
				} else {
					dispatch(&elem.data.msg);
				}
			}

		}else if(elem.type == LK_TIMER) {
			if(uuid_compare(gMan.id, elem.data.timer.id) == 0) {
				garbageManExecution();
			} else {
#ifdef DEBUG
				lk_log("PUSH_GOSSIP", "DEBUG", "Received timer to retransmit");
#endif
				dispatchPendingMessage(elem.data.timer.id);
			}

		}else if(elem.type == LK_REQUEST){
			if(elem.data.request.request == REQUEST && elem.data.request.request_type == PG_REQ) {


				uuid_t mid;
				genUUID(mid);

				LKMessage msg;
				msg.LKProto = elem.data.request.proto_origin;
				setMyAddr(&msg.srcAddr);
				setDestToBroadcast(&msg);
				msg.dataLen = elem.data.request.length;
				memcpy(msg.data, elem.data.request.payload, msg.dataLen);

				//release memory of request payload
				free(elem.data.request.payload);

				addMessage(mid);
				deliver(&msg);

				pushPayload(&msg,(char*) mid, sizeof(uuid_t), pargs->protoID, &bcastAddr);

				dispatch(&msg);

			}else{
				lk_log("PUSH_GOSSIP", "WARNING", "Got unexpected event");
			}
		}else{
			lk_log("PUSH_GOSSIP", "WARNING", "Got something unexpected");
		}
	}
}
