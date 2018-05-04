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

#include "dispatcher_logger.h"

#define LOG 1*1000*1000

static unsigned int mid = 0;

static unsigned long total_bytes_sent = 0;
static unsigned long total_bytes_recved = 0;

static unsigned long bytes_sent = 0;
static unsigned long bytes_recved = 0;

static unsigned int total_msgs_sent = 0;
static unsigned int total_msg_recved = 0;

static unsigned int msgs_sent = 0;
static unsigned int msgs_recved = 0;

static unsigned int round = 1;

static pthread_mutex_t log_lock;

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
	while(tmp != NULL){
		if(memcmp(tmp->src.data, src.data, WLAN_ADDR_LEN) == 0){
			ig = IGNORE;
			break;
		}
		tmp = tmp->next;
	}
	pthread_mutex_unlock(&ig_list_lock);

	return ig;
}


static int serializeLKMessage(LKMessage* msg, char* buffer) {

	int len = 0;

	char* tmp = buffer;

	memcpy(tmp, &mid, sizeof(unsigned int));
	len += sizeof(unsigned int);
	tmp += sizeof(unsigned int);

	memcpy(tmp, &msg->LKProto, sizeof(short));
	len += sizeof(short);
	tmp += sizeof(short);

	memcpy(tmp, msg->data, msg->dataLen);
	len += msg->dataLen;

	return len;
}

static unsigned int deserializeLKMessage(LKMessage* msg, char* buffer, short bufferlen) {

	short total = bufferlen;
	char* tmp = buffer;

	unsigned int recv_mid;

	memcpy(&recv_mid, tmp, sizeof(unsigned int));
	total -= sizeof(unsigned int);
	tmp += sizeof(unsigned int);

	memcpy(&msg->LKProto, tmp, sizeof(short));
	total -= sizeof(short);
	tmp += sizeof(short);

	memcpy(msg->data, tmp, total);
	msg->dataLen = total;

	return recv_mid;

}

static void* dispatcher_receiver(void* args) {
	Channel* ch = (Channel*) args;
	LKPhyMessage phymsg;
	LKMessage msg;
	while(1){
		int recv = 0;
		while((recv = chreceive(ch, &phymsg)) == CHANNEL_RECV_ERROR);

		if(ignore(phymsg.phyHeader.srcAddr) == IGNORE)
			continue;

		pthread_mutex_lock(&log_lock);
		total_bytes_recved += recv;
		bytes_recved += recv;

		total_msg_recved += 1;
		msgs_recved += 1;
		pthread_mutex_unlock(&log_lock);
#ifdef DEBUG
		unsigned int recv_mid = deserializeLKMessage(&msg, phymsg.data, phymsg.dataLen);
#else
		deserializeLKMessage(&msg, phymsg.data, phymsg.dataLen);
#endif
		msg.destAddr = phymsg.phyHeader.destAddr;
		msg.srcAddr = phymsg.phyHeader.srcAddr;
#ifdef DEBUG
		char s[2000];
		char addr[33];
		memset(addr, 0, 33);
		memset(s, 0, 2000);
		sprintf(s, "Delivering msg from %s with seq number %d to proto %d", wlan2asc(&msg.srcAddr, addr), recv_mid, msg.LKProto);
		lk_log("DISPACTHER-RECEIVER", "ALIVE",s);
#endif
		deliver(&msg);
	}
	return NULL;
}


static void dispatcher_deserializeIgReq(dispatch_ignore_req* ig_req, LKRequest* req){

	void * payload = req->payload;
	memcpy(&ig_req->ignore, payload, sizeof(dispatcher_ignore));
	payload += sizeof(dispatcher_ignore);

	memcpy(ig_req->src.data, payload, WLAN_ADDR_LEN);

	free(req->payload);

}

void* dispatcher_logger_init(void* args) {


	proto_args* pargs = args;
	Channel* ch = (Channel*) pargs->protoAttr;

	lk_log("DISPATCHER", "SRDS18", "round msgs_sent bytes_sent total_msgs_sent total_bytes_sent msgs_recved bytes_recved total_msgs_recved total_bytes_recved");

	pthread_mutex_init(&log_lock, NULL);
	pthread_mutex_init(&ig_list_lock, NULL);

	LKTimer log_timer;
	LKTimer_init(&log_timer, pargs->protoID, pargs->protoID);
	LKTimer_set(&log_timer, LOG, LOG);
	setupTimer(&log_timer);

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

			int sent = 0;
			while((sent = chsendTo(ch, &phymsg, (char*) elem.data.msg.destAddr.data)) == CHANNEL_SENT_ERROR);

			total_bytes_sent += sent;
			bytes_sent += sent;

			total_msgs_sent += 1;
			msgs_sent += 1;

#ifdef DEBUG
			char s[200];
			memset(s, 0, 200);
			sprintf(s, "Message sent to network from %d with seq number %d", elem.data.msg.LKProto, mid);
			lk_log("DISPACTHER-SENDER", "ALIVE",s);
#endif
			mid ++;
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
		} else if(elem.type == LK_TIMER) {
			pthread_mutex_lock(&log_lock);
			char l[500];
			bzero(l, 500);

			sprintf(l, "%d %d %ld %d %ld %d %ld %d %ld",round, msgs_sent, bytes_sent, total_msgs_sent, total_bytes_sent, msgs_recved, bytes_recved, total_msg_recved, total_bytes_recved);
			lk_log("DISPATCHER", "SRDS18", l);
			round ++;

			msgs_sent = 0;
			msgs_recved = 0;
			bytes_recved = 0;
			bytes_sent = 0;

			pthread_mutex_unlock(&log_lock);
		}
		else {
			//ignore
		}
	}
}
