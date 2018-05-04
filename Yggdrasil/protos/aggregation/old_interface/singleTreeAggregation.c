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

#include "singleTreeAggregation.h"

typedef enum MESSAGE_TYPE{
	QUERY = 1,
	CHILD = 2,
	VALUE = 3
}msg_type;

typedef enum CHILD_RESPONSE{
	YES = 1,
	NO = 2
}child_response;

typedef struct node_t{
	WLANAddr addr;
	uuid_t uuid;
	struct node_t* next;
}node;

static double value = -1;

static short protoId;
static uuid_t myId;

static node* toBeAccounted;
static node* children;
static node* parent;

static short received = 0;
static short sink = 0;

static aggregation_function f[4];

static short requests[4]; //proto ids to who requested the function

static void node_rem(node* lst, node* toRemove){

	if(uuid_compare(lst->uuid, toRemove->uuid) == 0){
		node* tmp = lst;
		lst = lst->next;
		tmp->next = NULL;
		free(tmp);
	}else{
		node* tmp = lst;
		while(tmp->next != NULL){
			if(uuid_compare(tmp->next->uuid, toRemove->uuid) == 0){
				node* next = tmp->next;
				tmp->next = next->next;
				next->next = NULL;
				free(next);
				break;

			}else{
				tmp = tmp->next;
			}
		}
	}


	free(toRemove);
}

static void node_add(node* lst, node* toAdd) {

	toAdd->next = lst;
	lst = toAdd;
}

static node* node_unload(LKMessage* msg) {
	node* n = malloc(sizeof(node));
	void* payload = msg->data;
	memcpy(n->addr.data, msg->srcAddr.data, WLAN_ADDR_LEN);
	memcpy(n->uuid, payload, sizeof(uuid_t));
	payload += sizeof(uuid_t);
	msg->dataLen -= sizeof(uuid_t);
	char temp[msg->dataLen];
	memcpy(temp, payload, msg->dataLen);
	memcpy(msg->data, temp, msg->dataLen);
	n->next = NULL;

	return n;

}

//static node* node_get_copy(node* original){
//	node* copy = NULL;
//	node* tmp = original;
//	if(tmp != NULL){
//		copy = malloc(sizeof(node));
//		memcpy(copy->addr.data, tmp->addr.data, WLAN_ADDR_LEN);
//		memcpy(copy->uuid, tmp->uuid, sizeof(uuid_t));
//		tmp = tmp->next;
//		copy->next = NULL;
//	}
//	node* prev = NULL;
//	while(tmp != NULL){
//		node* tmp_cpy = malloc(sizeof(node));
//		memcpy(tmp_cpy->addr.data, tmp->addr.data, WLAN_ADDR_LEN);
//		memcpy(tmp_cpy->uuid, tmp->uuid, sizeof(uuid_t));
//		tmp_cpy->next = NULL;
//		if(copy->next == NULL)
//			copy->next = tmp_cpy;
//		else{
//			prev->next = tmp_cpy;
//		}
//		prev = tmp_cpy;
//		tmp = tmp->next;
//	}
//
//	return copy;
//}

static node* node_get_copy_with_exclude(node* original, uuid_t exclude){
	node* copy = NULL;
	node* tmp = original;
	while(tmp != NULL && copy == NULL){
		if(uuid_compare(tmp->uuid, exclude) != 0){
			copy = malloc(sizeof(node));
			memcpy(copy->addr.data, tmp->addr.data, WLAN_ADDR_LEN);
			memcpy(copy->uuid, tmp->uuid, sizeof(uuid_t));
			copy->next = NULL;
		}
		tmp = tmp->next;
	}
	node* prev = NULL;
	while(tmp != NULL){
		if(uuid_compare(tmp->uuid, exclude) != 0){
			node* tmp_cpy = malloc(sizeof(node));
			memcpy(tmp_cpy->addr.data, tmp->addr.data, WLAN_ADDR_LEN);
			memcpy(tmp_cpy->uuid, tmp->uuid, sizeof(uuid_t));
			tmp_cpy->next = NULL;
			if(copy->next == NULL)
				copy->next = tmp_cpy;
			else{
				prev->next = tmp_cpy;
			}
			prev = tmp_cpy;
		}
		tmp = tmp->next;
	}

	return copy;
}


static void singleTree_sendQuery(WLANAddr* addr, short op){ //1

	LKMessage msg;
	LKMessage_init(&msg, addr->data, protoId);

	char payload[sizeof(uuid_t)+sizeof(short)];
	unsigned short len = sizeof(uuid_t)+sizeof(short);

	memcpy(payload, myId, sizeof(uuid_t));
	memcpy(payload+sizeof(uuid_t), &op, sizeof(short));
	LKMessage_addPayload(&msg, payload, len);

	dispatch(&msg);

}

static void singleTree_sendChild(short response, node* n, short op) { //2

	LKMessage msg;
	LKMessage_init(&msg, n->addr.data, protoId);

	char payload[sizeof(uuid_t)+sizeof(short)];
	unsigned short len = sizeof(uuid_t)+sizeof(short);

	memcpy(payload, myId, sizeof(uuid_t));
	memcpy(payload+sizeof(uuid_t), &response, sizeof(short));
	memcpy(payload+sizeof(uuid_t) + sizeof(short), &op, sizeof(short));

	LKMessage_addPayload(&msg, payload, len);

	dispatch(&msg);

	if(response == NO){
		free(n);
	}
}

static void singleTree_sendValue(double value, short op, node* n){ //3
	LKMessage msg;
	LKMessage_init(&msg, n->addr.data, protoId);

	char payload[sizeof(uuid_t)+sizeof(double) + sizeof(short)];
	unsigned short len = sizeof(uuid_t)+sizeof(double) + sizeof(short);

	memcpy(payload, myId, sizeof(uuid_t));
	memcpy(payload+sizeof(uuid_t), &value, sizeof(double));
	memcpy(payload+sizeof(uuid_t) + sizeof(double), &op, sizeof(short));

	LKMessage_addPayload(&msg, payload, len);

	dispatch(&msg);
}

static void singleTree_sendReport(double value, short op){

	LKRequest reply;
	reply.request = REPLY;
	short type = -1;
	switch(op){
	case 0:
		type = SIMPLE_AGG_REQ_GET_SUM;
		break;
	case 1:
		type = SIMPLE_AGG_REQ_GET_MAX;
		break;
	case 2:
		type = SIMPLE_AGG_REQ_GET_MIN;
		break;
	case 3:
		type = SIMPLE_AGG_REQ_GET_AVG;
		break;
	default:
		break;
	}
	reply.request_type = type;

	reply.proto_dest = requests[op];
	reply.proto_origin = protoId;

	//double value + short op
	reply.payload = malloc(sizeof(double)+sizeof(short));

	void* payload = reply.payload;

	memcpy(payload, &value, sizeof(double));
	memcpy(payload+sizeof(double), &op, sizeof(short));

	reply.length = sizeof(double) + sizeof(short);

	deliverReply(&reply);
}

static void singleTree_report(short op){
	if(toBeAccounted != NULL || children != NULL)
		return;

	if(sink == 0){
		singleTree_sendValue(value, op, parent);
	}else
		singleTree_sendReport(value, op);

}

static void singleTree_processQuery(LKMessage* msg){

	node* n = node_unload(msg);
	short op = 0;
	if(received != 0){
		received = 1;
		parent = n;
		memcpy(&op, msg->data, sizeof(short));
		singleTree_sendChild(YES, n, op);
		node* sendList = node_get_copy_with_exclude(toBeAccounted, parent->uuid);
		while(sendList != NULL){
			singleTree_sendQuery(&sendList->addr, op);
			node* tmp = sendList;
			sendList = sendList->next;
			free(tmp);
			tmp->next = NULL;
		}
	}else{
		singleTree_sendChild(NO, n, op);
	}
}

static void singleTree_processChild(LKMessage* msg){

	node* n = node_unload(msg);

	short r;
	memcpy(&r, msg->data, sizeof(short));
	short op;
	memcpy(&op, msg->data+sizeof(short), sizeof(short));

	if(r == YES){
		node_add(children, n);
	}
	node_rem(toBeAccounted, n);
	singleTree_report(op);
}

static void singleTree_processValue(LKMessage* msg){

	node* n = node_unload(msg);

	double v;
	short op;

	memcpy(&v, msg->data, sizeof(double));
	memcpy(&op, msg->data + sizeof(double), sizeof(short));

	if(op < 0 || op > 4){
		lk_log("SINGLE TREE AGG", "ERROR", "No support for this type of aggregation");
		return;
	}

	aggregation_function func = f[op];

	value = func(value, v);

	node_rem(children, n);

	singleTree_report(op);

}

static short singleTree_checkMsg(LKMessage* msg){
	void* payload = msg->data;
	short type;
	memcpy(&type, payload, sizeof(short));
	payload += sizeof(short);
	msg->dataLen -= sizeof(short);

	char temp[msg->dataLen];
	memcpy(temp, payload, msg->dataLen);
	memcpy(msg->data, temp, msg->dataLen);

	return type;
}


void* singleTreeAggregation_init(void* args) {
	proto_args* pargs = (proto_args*) args;

	queue_t* inBox = pargs->inBox;
	protoId = pargs->protoID;
	memcpy(myId, pargs->myuuid, sizeof(uuid_t));

	value = *((double*) pargs->protoAttr);

	f[0] = sumFunc;
	f[1] = maxFunc;
	f[2] = minFunc;
	//f[3] = avgFunc; TODO: Put here the right function

	while(1){
		queue_t_elem elem;
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			LKMessage msg = elem.data.msg;

			short type = singleTree_checkMsg(&msg);

			switch(type){
			case QUERY:
				singleTree_processQuery(&msg);
				break;
			case CHILD:
				singleTree_processChild(&msg);
				break;
			case VALUE:
				singleTree_processValue(&msg);
				break;
			default:
				lk_log("SINGLE TREE AGG", "WARNING", "Unknown message type received");
				break;
			}

		//}else if(elem.type == LK_TIMER){
			//LKTimer timer = elem.data.timer;
		//}else if(elem.type == LK_EVENT){
			//LKEvent event = elem.data.event;
		}else if(elem.type == LK_REQUEST){
			LKRequest req = elem.data.request;
			if(req.request == REQUEST){
				switch(req.request_type){
				case SIMPLE_AGG_REQ_DO_SUM:
					requests[0] = req.proto_origin;
					//TODO: Replace NULL(s) by something correct... eventually do a cycle
					singleTree_sendQuery(NULL, 0);
					break;
				case SIMPLE_AGG_REQ_DO_MAX:
					requests[1] = req.proto_origin;
					singleTree_sendQuery(NULL, 1);
					break;
				case SIMPLE_AGG_REQ_DO_MIN:
					requests[2] = req.proto_origin;
					singleTree_sendQuery(NULL, 2);
					break;
				case SIMPLE_AGG_REQ_DO_AVG:
					requests[3] = req.proto_origin;
					singleTree_sendQuery(NULL, 3);
					break;
				default:
					break;
				}
			}else if(req.request == REPLY){
				if(req.request_type == SIMPLE_AGG_REPLY_FOR_VALUE){
					memcpy(&value, req.payload, sizeof(double));

				}else{
					lk_log("SINGLE TREE AGG", "WARNING", "Unexpected reply received");
				}

				free(req.payload);

			}else{
				lk_log("SINGLE TREE AGG", "WARNING", "Unknown Request type");
			}
		}else{
			lk_log("SINGLE TREE AGG", "WARNING", "Unknown type received from queue");
		}
	}


}

