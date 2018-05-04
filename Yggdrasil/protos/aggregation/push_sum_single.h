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

#ifndef AGGREGATION_PUSH_SUM_SINGLE_H_
#define AGGREGATION_PUSH_SUM_SINGLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"

#include "../discovery/discovery.h"
#include "../communication/point-to-point/reliable.h"

#include "aggregation_gen_interface.h"

typedef enum push_sum_opt_code_{
	MAX_ROUNDS = 1 //number of maximum rounds
}push_sum_opt_code;

typedef struct push_sum_attr_{
	short reliable_proto_id;
	short discovery_proto_id;
}push_sum_attr;

void* push_sum_single_init(void* args);



#endif /* AGGREGATION_PUSH_SUM_SINGLE_H_ */
