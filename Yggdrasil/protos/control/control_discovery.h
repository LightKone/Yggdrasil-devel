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

#ifndef CONTROL_DISCOVERY_H_
#define CONTROL_DISCOVERY_H_

#include "lightkone.h"
#include "core/utils/utils.h"
#include "core/lk_runtime.h"
#include "core/utils/queue.h"

typedef struct control_neighbor_ {
	WLANAddr mac_addr;
	char ip_addr[16];
	struct control_neighbor_* next;
} control_neighbor;

typedef enum CONTROL_DISC_REQ_ {
	DISABLE_DISCOVERY,
	ENABLE_DISCOVERY
} CONTROL_DISC_REQ;

typedef enum CONTROL_DISC_NOTF_ {
	NEW_NEIGHBOR_IP_NOTIFICATION
} CONTROL_DISC_NOTF;

void* control_discovery_init(void* arg);

#endif /* CONTROL_DISCOVERY_H_ */
