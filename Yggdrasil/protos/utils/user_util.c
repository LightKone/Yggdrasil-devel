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

#include "user_util.h"

/* Cancels a timer */
void cancel_timer(LKTimer* timer) {
	LKTimer_set(timer, 0, 0);
	setupTimer(timer);
}

/* Sets-up a new timer to itself */
void setup_self_timer(LKTimer* timer, const unsigned short protoID, const time_t period) {
	LKTimer_init(timer, protoID, protoID);
	LKTimer_set(timer, period*1000*1000, 0);
	setupTimer(timer);
}

/* Fill message fields */
void push_msg_info(LKMessage* msg, const unsigned short protoID) {
	setDestToBroadcast(msg);

	msg->LKProto = protoID;
}

/* Copies my uuid and counter to msg */
void push_msg_uuid_ctr(LKMessage* msg, const uuid_t uuid, const short counter) {
	msg->dataLen = UUID_SIZE + CTR_SIZE;
	memcpy(msg->data, uuid, UUID_SIZE);
	memcpy(msg->data + UUID_SIZE, &counter, CTR_SIZE);
}
