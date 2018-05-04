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

#include "reliable.h"

#define TIME_TO_LIVE 5
#define TIME_TO_RESEND 2
#define TIMER 2 * 1000 * 1000 //2 seconds

#define THRESHOLD 5 //five times resend

#define PANIC -1
#define DUPLICATE 0
#define OK 1

typedef enum types_{
	MSG,
	ACK,
	NACK
}types;

typedef struct msg_list_{
	unsigned short sqn;
	LKMessage* msg;
	unsigned short resend;
	time_t time;
	struct msg_list_* next;
}msg_list;

typedef struct msgs_{
	time_t first;
	WLANAddr destination;
	msg_list* list;
	struct msgs_* next;
}msgs;

static msgs* outbond = NULL;
static msgs* inbond = NULL;

typedef struct meta_info_{
	unsigned short sqn;
	unsigned short type;
}meta_info;

static unsigned short sqn = 1;

static unsigned short myID;

void* serialize_meta_info(meta_info* info){
	void* buffer = malloc(sizeof(meta_info));

	memcpy(buffer, &info->sqn, sizeof(unsigned short));
	memcpy(buffer + sizeof(unsigned short), &info->type, sizeof(unsigned short));

	return buffer;
}

void deserialize_meta_info(void* buffer, meta_info* info){
	memcpy(&info->sqn, buffer, sizeof(unsigned short));
	memcpy(&info->type, buffer + sizeof(unsigned short), sizeof(unsigned short));
}

void store_outbond(LKMessage* msg){
	//simple store
	msgs* out = NULL;
	if(outbond == NULL){
		outbond = malloc(sizeof(msgs));
		outbond->next = NULL;
		out = outbond;
		memcpy(&out->destination, &msg->destAddr, WLAN_ADDR_LEN);
		out->list = NULL;
		//out->list->next = NULL;
	}else{
		out = outbond;
		while((memcmp(&out->destination, &msg->destAddr, WLAN_ADDR_LEN) != 0) && out->next != NULL){
			out = out->next;
		}
		if((memcmp(&out->destination, &msg->destAddr, WLAN_ADDR_LEN) != 0)){
			out->next = malloc(sizeof(msgs));
			out = out->next;
			memcpy(&out->destination, &msg->destAddr, WLAN_ADDR_LEN);
			out->list = NULL;
			//out->list->next = NULL;

			out->next = NULL;
		}
	}


	//add msg to list
	msg_list* l = malloc(sizeof(msg_list));
	l->msg = malloc(sizeof(LKMessage));
	memcpy(l->msg, msg, sizeof(LKMessage));
	l->sqn = sqn;
	l->resend = 0;
	l->time = time(NULL);

	l->next = out->list;
	out->list = l;
}

short store_inbond(unsigned short recv_sqn, LKMessage* msg){
	//check if it and store it

	msgs* in = NULL;
	if(inbond == NULL){
		inbond = malloc(sizeof(msgs));
		inbond->next = NULL;
		in = inbond;
		memcpy(&in->destination, &msg->destAddr, WLAN_ADDR_LEN);
		in->list = malloc(sizeof(msg_list));

		msg_list* l = in->list;
		l->msg = NULL;
		l->sqn = recv_sqn;
		l->time = time(NULL);

		l->next = NULL;

		in->first = time(NULL);
		in->next = NULL;

	}else{
		in = inbond;
		while((memcmp(&in->destination, &msg->destAddr, WLAN_ADDR_LEN) != 0) && in->next != NULL){
			in = in->next;
		}
		if((memcmp(&in->destination, &msg->destAddr, WLAN_ADDR_LEN) != 0)){
			in->next = malloc(sizeof(msgs));
			in = in->next;
			memcpy(&in->destination, &msg->destAddr, WLAN_ADDR_LEN);
			in->list = malloc(sizeof(msg_list));

			msg_list* l = in->list;
			l->msg = NULL;
			l->sqn = recv_sqn;
			l->time = time(NULL);

			l->next = NULL;

			in->first = time(NULL);
			in->next = NULL;
		}else{
			msg_list* l = in->list;

			if(l == NULL){
				msg_list* l = malloc(sizeof(msg_list));
				l->msg = NULL;
				l->sqn = recv_sqn;
				l->time = time(NULL);

				l->next = NULL;

				in->first = time(NULL);
				in->list = l;

			}else{

				while(l != NULL){
					if(l->sqn == recv_sqn)
						return DUPLICATE;
					if(l->next == NULL){
						break;
					}
					l = l->next;
				}

				l->next = malloc(sizeof(msg_list));
				l->next->msg = NULL;
				l->next->sqn = recv_sqn;
				l->next->time = time(NULL);

				l->next->next = NULL;

			}
		}
	}


	return OK;

}

short rm_outbond(unsigned short ack_sqn, WLANAddr* addr){

	msgs* out = outbond;

	while(out != NULL && (memcmp(&out->destination, addr, WLAN_ADDR_LEN) != 0)){
		//search for destination
		out = out->next;
	}
	if(out == NULL || out->list == NULL){
		return PANIC;
	}

	if(out->list->sqn == ack_sqn){
		msg_list* torm = out->list;
		out->list = torm->next;
		free(torm->msg);
		free(torm);

		return OK;

	}else{
		msg_list* l = out->list;
		while(l->next != NULL){
			if(l->next->sqn == ack_sqn){
				msg_list* torm = l->next;
				l->next = torm->next;
				free(torm->msg);
				free(torm);
				return OK;
			}

			l = l->next;
		}
	}

	return PANIC;
}

short rm_inbond(){
	msgs* in = inbond;
	while(in != NULL){
		if(in->first != 0 &&  in->first + TIME_TO_LIVE <  time(NULL)){
			msg_list* l = in->list;
			while(l != NULL && l->time + TIME_TO_LIVE < time(NULL)){
				msg_list* torm = l;
				l = torm->next;

				in->list = torm->next;
				if(l != NULL)
					in->first = l->time;
				else{
					in->first = 0;
				}

				free(torm);
			}
		}

		in = in->next;
	}

	return OK;
}

//void process_nack(){
//	TODO
//}

void prepareAck(LKMessage* msg, unsigned short recv_sqn, WLANAddr* dest){
	LKMessage_init(msg, dest->data, myID);

	meta_info info;
	info.sqn = recv_sqn;
	info.type = ACK;

	void* buffer = serialize_meta_info(&info);

	pushPayload(msg, (char *) buffer, sizeof(meta_info), myID, dest);

	free(buffer);
}

//void prepareNack(LKMessage* msg, unsigned short recv_sqn, WLANAddr* dest){
//	//TODO
//	LKMessage_init(msg, dest, myID);
//
//	meta_info info;
//	info.sqn = recv_sqn;
//	info.type = ACK;
//
//	void* buffer = serialize_meta_info(&info);
//
//	pushPayload(msg, (char *) buffer, sizeof(meta_info), myID, dest);
//
//	free(buffer);
//}

void notify_failed_delivery(LKMessage* msg){

	void* buffer = malloc(sizeof(meta_info));

	popPayload(msg, (char *) buffer, sizeof(meta_info));

	meta_info info;

	deserialize_meta_info(buffer, &info);

	LKEvent ev;

	ev.proto_origin = myID;
	ev.notification_id = FAILED_DELIVERY;
	ev.payload = malloc(sizeof(LKMessage));

	memcpy(ev.payload, msg, sizeof(LKMessage));
	ev.length = sizeof(LKMessage);

	deliverEvent(&ev);

	free(ev.payload);
}

void * reliable_point2point(void * args){

	WLANAddr bcastAddr;
	setBcastAddr(&bcastAddr);

	proto_args* pargs = args;

	myID = pargs->protoID;

	queue_t* inBox = pargs->inBox;
	queue_t* dispatcher = pargs->protoAttr;

	LKTimer gMan;
	LKTimer_init(&gMan, pargs->protoID, pargs->protoID);
	LKTimer_set(&gMan, TIMER, TIMER);

	setupTimer(&gMan);

	queue_t_elem elem;

	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE) {

			if(elem.data.msg.LKProto != myID){
				//message from some protocol
				if(memcmp(elem.data.msg.destAddr.data, bcastAddr.data, WLAN_ADDR_LEN) != 0){
					//only keep track of point to point messages

					meta_info info;
					info.sqn = sqn;
					info.type = MSG;

					void* buffer = malloc(sizeof(meta_info));
					memcpy(buffer, &info.sqn, sizeof(unsigned short));
					memcpy(buffer + sizeof(unsigned short), &info.type, sizeof(unsigned short));

					printf("============> SENDING MSG for %u for proto %u, (before pushed) msg len %u\n", info.sqn, elem.data.msg.LKProto, elem.data.msg.dataLen);

					pushPayload(&elem.data.msg,(char *) buffer,sizeof(meta_info),myID,&elem.data.msg.destAddr);

					printf("============> SENDING MSG for %u for proto %u, (after pushed) msg len %u\n", info.sqn, elem.data.msg.LKProto, elem.data.msg.dataLen);

					store_outbond(&elem.data.msg);

					sqn ++;

					free(buffer);
				}

				queue_push(dispatcher, &elem);

			}else{
				//message from the network

				void* buffer = malloc(sizeof(meta_info));

				popPayload(&elem.data.msg, (char *) buffer, sizeof(meta_info));

				meta_info info;

				deserialize_meta_info(buffer, &info);

				free(buffer);

				if(info.type == ACK){
					//check if is ack, and rm from outbond
					printf("============> RECEIVED ACK for %u\n", info.sqn);
					if(rm_outbond(info.sqn, &elem.data.msg.srcAddr) == PANIC){
						lk_log("RELIABLE POINT2POINT", "PANIC", "Tried to remove unexisting outbond message");
					}

				}else if(info.type == NACK){
					//reprocess msg
					//process_nack(info.sqn, &elem.data.msg.srcAddr);

				}else if(info.type == MSG){
					//process msg
					printf("============> RECEIVED MSG for %u\n", info.sqn);
					if(store_inbond(info.sqn, &elem.data.msg) == OK){

						deliver(&elem.data.msg);

					}

					prepareAck(&elem.data.msg, info.sqn, &elem.data.msg.srcAddr);

					printf("============> SENDING ACK for %u for proto %u, msg len %u\n", info.sqn, elem.data.msg.LKProto, elem.data.msg.dataLen);

					queue_push(dispatcher, &elem);
				}else{
					lk_log("RELIABLE POINT2POINT", "WARNING", "Unknown message type");
				}

			}

		}else if(elem.type == LK_TIMER) {

			//resend everything that is pending confirmation
			msgs* out = outbond;
			while(out != NULL){
				msg_list** l = &(out->list);
				while(*l != NULL){
					if((*l)->resend > THRESHOLD){

						msg_list* torm = *l;
						*l = torm->next;

						notify_failed_delivery(torm->msg);
						free(torm->msg);
						free(torm);
					}else if((*l)->time + TIME_TO_RESEND < time(NULL)){

						elem.type = LK_MESSAGE;
						memcpy(&elem.data.msg, (*l)->msg, sizeof(LKMessage));

						printf("============> RESENDING MSG for %u, msg len %u\n", (*l)->sqn, elem.data.msg.dataLen);

						queue_push(dispatcher, &elem);

						(*l)->resend ++;
						(*l)->time = time(NULL);

						l = &((*l)->next);
					}else{
						l = &((*l)->next);
					}
				}

				out = out->next;
			}

			//get some memory
			rm_inbond();

		}else{
			lk_log("RELIABLE POINT2POINT", "WARNING", "Got something unexpected");
		}
	}

}


