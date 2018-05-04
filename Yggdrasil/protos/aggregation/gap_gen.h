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

#ifndef AGGREGATION_GAP_GEN_H_
#define AGGREGATION_GAP_GEN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include <limits.h>

#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"

#include "../discovery/discovery.h"
#include "../discovery/discovery_ft_lb.h" //ev up; ev down

#include "aggregation_gen_interface.h"
#include "aggregation_gen_functions.h"

typedef enum policy_type_{
	POLICY_DEFAULT, //cache-like
	POLICY_CONSERVATIVE,
	POLICY_GREEDY
}policy_type;

typedef struct gap_args_ {
	short discoveryId;
	policy_type policy_type;
	short root; //1 if root, 0 if not
}gap_args;

void * gap_init(void * args);

#endif /* AGGREGATION_GAP_GEN_H_ */
