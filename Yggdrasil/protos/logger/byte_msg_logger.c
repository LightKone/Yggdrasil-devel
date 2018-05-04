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

#include "byte_msg_logger.h"

#define LOG 1*1000*1000

static unsigned long total_bytes_sent = 0;
static unsigned long total_bytes_recved = 0;

static unsigned long bytes_sent = 0;
static unsigned long bytes_recved = 0;

static unsigned int total_msgs_sent = 0;
static unsigned int total_msg_recved = 0;

static unsigned int msgs_sent = 0;
static unsigned int msgs_recved = 0;

static unsigned int round = 1;

static short protoId;


void* byte_logger_init(void* args) {
	proto_args* pargs = (proto_args*) args;
	protoId = pargs->protoID;

	queue_t* dispatcher = (queue_t*) pargs->protoAttr;

	queue_t* inBox = pargs->inBox;


	lk_log("BYTE LOGGER", "SRDS18", "round msgs_sent bytes_sent total_msgs_sent total_bytes_sent msgs_recved bytes_recved total_msgs_recved total_bytes_recved");

	LKTimer log_timer;
	LKTimer_init(&log_timer, protoId, protoId);
	LKTimer_set(&log_timer, LOG, LOG);
	setupTimer(&log_timer);

	queue_t_elem elem;

	while(1) {
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE) {
			if(elem.data.msg.LKProto != protoId){
				//going downstream
				int datalen = elem.data.msg.dataLen;
				pushEmptyPayload(&elem.data.msg, protoId);

				total_bytes_sent += datalen;
				bytes_sent += datalen;

				msgs_sent += 1;
				total_msgs_sent += 1;

				queue_push(dispatcher, &elem);

			}else{
				//going upstream
				popEmptyPayload(&elem.data.msg);
				int datalen = elem.data.msg.dataLen;

				total_bytes_recved += datalen;
				bytes_recved += datalen;

				msgs_recved += 1;
				total_msg_recved += 1;

				filterAndDeliver(&elem.data.msg);
			}

		} else if(elem.type == LK_TIMER){
			if(uuid_compare(elem.data.timer.id, log_timer.id) != 0) {
				queue_push(dispatcher, &elem);
				continue;
			}

			char l[500];
			bzero(l, 500);
			//TODO: define log msg format
			sprintf(l, "%d %d %ld %d %ld %d %ld %d %ld",round, msgs_sent, bytes_sent, total_msgs_sent, total_bytes_sent, msgs_recved, bytes_recved, total_msg_recved, total_bytes_recved);
			lk_log("BYTE LOGGER", "SRDS18", l);
			round ++;

			msgs_sent = 0;
			msgs_recved = 0;
			bytes_recved = 0;
			bytes_sent = 0;
		} else{ //other event
			queue_push(dispatcher, &elem);
		}

		free_elem_payload(&elem);
	}
}
