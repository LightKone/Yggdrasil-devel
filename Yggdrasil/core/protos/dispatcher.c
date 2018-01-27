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

#include "dispatcher.h"

static pthread_mutex_t ig_list_lock;

typedef struct ignore_list_ {
	WLANAddr src;
	struct ignore_list_* next;
}ignore_list;

static ignore_list* ig_lst;

static int addToIgnoreLst(WLANAddr src) {
	ignore_list* tmp = ig_lst;

	if(tmp == NULL){
		pthread_mutex_lock(&ig_list_lock);
		tmp = malloc(sizeof(ignore_list));
		memcpy(tmp->src.data, src.data, WLAN_ADDR_LEN);
		tmp->next = NULL;
		ig_lst = tmp;
		pthread_mutex_unlock(&ig_list_lock);
		return SUCCESS;
	}

	while(tmp->next != NULL) {
		if(memcmp(tmp->next->src.data, src.data, WLAN_ADDR_LEN) == 0)
			break;
		tmp = tmp->next;
	}
	if(tmp->next == NULL) {
		ignore_list* new = malloc(sizeof(ignore_list));
		pthread_mutex_lock(&ig_list_lock);
		memcpy(new->src.data, src.data, WLAN_ADDR_LEN);
		tmp->next = new;
		new->next = NULL;
		pthread_mutex_unlock(&ig_list_lock);
		return SUCCESS;
	}
	return FAILED;
}

static int remFromIgnoreLst(WLANAddr src) {
	ignore_list* tmp = ig_lst;

	if(tmp == NULL)
		return FAILED;

	if(memcmp(tmp->src.data, src.data, WLAN_ADDR_LEN) == 0) {
		pthread_mutex_lock(&ig_list_lock);
		ignore_list* old = tmp;
		ig_lst = tmp->next;
		old->next = NULL;
		free(old);
		pthread_mutex_unlock(&ig_list_lock);
		return SUCCESS;
	}
	while(tmp->next != NULL) {
		if(memcmp(tmp->next->src.data, src.data, WLAN_ADDR_LEN) == 0) {
			pthread_mutex_lock(&ig_list_lock);
			ignore_list* old = tmp->next;
			tmp->next = old->next;
			old->next = NULL;
			free(old);
			pthread_mutex_unlock(&ig_list_lock);
			return SUCCESS;
		}
		tmp = tmp->next;
	}
	return FAILED;
}

static int ignore(WLANAddr src){
	ignore_list* tmp = ig_lst;
	int ig = NOT_IGNORE;
	pthread_mutex_lock(&ig_list_lock);
	int i = 0;
	while(tmp != NULL){
		if(memcmp(tmp->src.data, src.data, WLAN_ADDR_LEN) == 0){
			ig = IGNORE;
			break;
		}
		tmp = tmp->next;
		i++;
	}
	pthread_mutex_unlock(&ig_list_lock);

	return ig;
}


static int serializeLKMessage(LKMessage* msg, char* buffer) {

	int len = 0;

	char* tmp = buffer;

	memcpy(tmp, &msg->LKProto, sizeof(short));
	len += sizeof(short);
	tmp += sizeof(short);

	memcpy(tmp, msg->data, msg->dataLen);
	len += msg->dataLen;

	return len;
}

static void deserializeLKMessage(LKMessage* msg, char* buffer, short bufferlen) {

	short total = bufferlen;
	char* tmp = buffer;

	memcpy(&msg->LKProto, tmp, sizeof(short));
	total -= sizeof(short);
	tmp += sizeof(short);

	memcpy(msg->data, tmp, total);
	msg->dataLen = total;

}

static void* dispatcher_receiver(void* args) {
	Channel* ch = (Channel*) args;
	LKPhyMessage phymsg;
	LKMessage msg;
	while(1){
		while(chreceive(ch, &phymsg) == CHANNEL_RECV_ERROR);

		if(ignore(phymsg.phyHeader.srcAddr) == IGNORE)
			continue;

		deserializeLKMessage(&msg, phymsg.data, phymsg.dataLen);
		msg.destAddr = phymsg.phyHeader.destAddr;
		msg.srcAddr = phymsg.phyHeader.srcAddr;
#ifdef DEBUG
		char s[2000];
		char addr[33];
		memset(addr, 0, 33);
		memset(s, 0, 2000);
		sprintf(s, "Delivering msg from %s to proto %d", wlan2asc(&msg.srcAddr, addr), msg.LKProto);
		lk_log("DISPACTHER-RECEIVER", "ALIVE",s);
#endif
		deliver(&msg);
	}
	return NULL;
}

void dispatcher_serializeIgReq(dispatcher_ignore ignore, WLANAddr src, LKRequest* req){
	req->payload = malloc(sizeof(dispatcher_ignore) + WLAN_ADDR_LEN);
	void * tmp = req->payload;
	int len = 0;
	memcpy(tmp, &ignore, sizeof(dispatcher_ignore));
	len += sizeof(dispatcher_ignore);
	tmp += sizeof(dispatcher_ignore);
	memcpy(tmp, src.data, WLAN_ADDR_LEN);
	len += WLAN_ADDR_LEN;

	req->length = len;
}

static void dispatcher_deserializeIgReq(dispatch_ignore_req* ig_req, LKRequest* req){

	void * payload = req->payload;
	memcpy(&ig_req->ignore, payload, sizeof(dispatcher_ignore));
	payload += sizeof(dispatcher_ignore);

	memcpy(ig_req->src.data, payload, WLAN_ADDR_LEN);

	free(req->payload);

}

void* dispatcher_init(void* args) {


	proto_args* pargs = args;
	Channel* ch = (Channel*) pargs->protoAttr;

	pthread_mutex_init(&ig_list_lock, NULL);

	queue_t* inBox = pargs->inBox;

	pthread_t receiver;
	pthread_attr_t patribute;
	pthread_attr_init(&patribute);

	pthread_create(&receiver, &patribute, &dispatcher_receiver, ch);

	queue_t_elem elem;
	while(1){

		queue_pop(inBox, &elem);
		if(elem.type == LK_MESSAGE){

			LKPhyMessage phymsg;
			initLKPhyMessage(&phymsg);
			int len = serializeLKMessage(&elem.data.msg, phymsg.data);
			phymsg.dataLen = len;

			while(chsendTo(ch, &phymsg, (char*) elem.data.msg.destAddr.data) == CHANNEL_SENT_ERROR);
#ifdef DEBUG
			char s[200];
			memset(s, 0, 200);
			sprintf(s, "Message sent to network from %d", elem.data.msg.LKProto);
			lk_log("DISPACTHER-SENDER", "ALIVE",s);
#endif
		} else if(elem.type == LK_REQUEST){
			LKRequest req = elem.data.request;
			if(req.proto_dest == pargs->protoID && req.request == REQUEST && req.request_type == DISPATCH_IGNORE_REG){
				dispatch_ignore_req ig_req;
				dispatcher_deserializeIgReq(&ig_req, &req);

				if(ig_req.ignore == IGNORE){
#ifdef DEBUG
					char s[33];
					char msg[2000];
					memset(msg, 0, 2000);
					int r = addToIgnoreLst(ig_req.src);
					if(r == SUCCESS){
						sprintf(msg, "successfully added %s to ignore list", wlan2asc(&ig_req.src, s));
						lk_log("DISPATCHER", "DEBUG", msg);
					} else if(r == FAILED){
						lk_log("DISPATCHER", "DEBUG", "failed to add to ignore list, is it already there?");
					}
#else
					addToIgnoreLst(ig_req.src);
#endif
				}
				else if(ig_req.ignore == NOT_IGNORE){
#ifdef DEBUG
					int r = remFromIgnoreLst(ig_req.src);
					if(r == SUCCESS){
						lk_log("DISPATCHER", "DEBUG", "successfully removed from ignore list");
					} else if(r == FAILED){
						lk_log("DISPATCHER", "DEBUG", "failed to remove from ignore list, was it there?");
					}
#else
					remFromIgnoreLst(ig_req.src);
#endif
				}
				else{
					lk_log("DISPATCHER", "PANIC", "something went terribly wrong when deserializing request");
				}
			}
		}
		else {
			//ignore
		}
	}
}
