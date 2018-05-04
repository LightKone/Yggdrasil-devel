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

#ifndef AGGREGATION_GAP_AVG_BCAST_H_
#define AGGREGATION_GAP_AVG_BCAST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include <limits.h>

#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"

#include "protos/discovery/discovery.h"
#include "protos/discovery/discovery_ft_lb.h" //ev up; ev down

#include "protos/aggregation/aggregation_gen_interface.h"
#include "protos/aggregation/aggregation_gen_functions.h"

#include "protos/aggregation/gap_gen.h"

void * gap_avg_bcast_init(void * args);

#endif /* AGGREGATION_GAP_AVG_BCAST_H_ */
