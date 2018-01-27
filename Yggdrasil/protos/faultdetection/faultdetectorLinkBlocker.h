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

#ifndef FAULTDETECTORLINKBLOCKER_H_
#define FAULTDETECTORLINKBLOCKER_H_

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

#define PROTO_FT_LB 102

typedef enum ft_lb_events_ {
	FT_LB_SUSPECT = 0,
	FT_LB_EV_MAX = 1
} ft_lb_events;

typedef struct _ft_lb_attr {
	queue_t* intercept;
	unsigned long timeout_failure_ms;
} ft_lb_attr;


void* faultdetectorLinkBlocker_init(void* arg);

#endif /* FAULTDETECTORLINKBLOCKER_H_ */
