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

#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_

#include <stdio.h>
#include <uuid/uuid.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>


#include "lightkone.h"
#include "core/lk_runtime.h"
#include "core/protos/timer.h"
#include "core/protos/dispatcher.h"
#include "core/utils/queue.h"
#include "core/utils/utils.h"

#include "discovery_ft_lb.h"

typedef struct connection_attr_{
	queue_t* intercept;
	unsigned long discovery_period;
	unsigned short msg_lost_per_fault; //0 fault detector off; number of messages to send within period;
	unsigned short black_list_links; //0 off; number of consecutive faults before black list;
}connection_attr;

void* connection_manager_init(void* arg);

#endif /* CONNECTION_MANAGER_H_ */
