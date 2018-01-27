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

#ifndef DISCOVERYFAULTDETECTORLINKBLOCKER_H_
#define DISCOVERYFAULTDETECTORLINKBLOCKER_H_

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


typedef enum discov_ft_lb_events_ {
	DISCOV_FT_LB_EV_NEIGH_UP,
	DISCOV_FT_LB_EV_NEIGH_DOWN,
	DISCOV_FT_LB_EV_MAX
} discov_ft_lb_events;

typedef struct _discov_ft_lb_attr {
	queue_t* intercept;
	unsigned long timeout_failure_ms;
} discov_ft_lb_attr;


void* discovery_ft_lb_init(void* arg);

#endif /* DISCOVERYFAULTDETECTORLINKBLOCKER_H_ */
