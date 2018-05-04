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

#ifndef COMMUNICATION_POINT_TO_POINT_RELIABLE_H_
#define COMMUNICATION_POINT_TO_POINT_RELIABLE_H_

#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "core/lk_runtime.h"
#include "lightkone.h"

#include "core/utils/utils.h"

#include "core/proto_data_struct.h"
#include "core/utils/queue.h"

typedef enum reliable_p2p_events_{
	FAILED_DELIVERY,
	MAX_EV
}reliable_p2p_events;

void * reliable_point2point(void * args);

#endif /* COMMUNICATION_POINT_TO_POINT_RELIABLE_H_ */
