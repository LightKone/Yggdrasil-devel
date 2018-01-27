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

#ifndef USER_UTIL_H_
#define USER_UTIL_H_

#include "core/utils/utils.h"
#include "core/proto_data_struct.h"
#include "protos/discovery/disc_ctr/discovery_CTR.h"

/* Cancels a timer */
void cancel_timer(LKTimer* timer);

/* Sets-up a new timer to itself */
void setup_self_timer(LKTimer* timer, const unsigned short protoID, const time_t period);

/* Fill message fields */
void push_msg_info(LKMessage* msg, const unsigned short protoID);

/* Copies my uuid and counter to msg */
void push_msg_uuid_ctr(LKMessage* msg, const uuid_t uuid, const short counter);

#endif
