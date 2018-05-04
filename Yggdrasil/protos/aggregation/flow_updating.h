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

#ifndef AGGREGATION_FLOW_UPDATING_H_
#define AGGREGATION_FLOW_UPDATING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"

#include "../discovery/discovery_ft_lb.h" //ev up; ev down

#include "aggregation_gen_interface.h"


void* flow_updating_init(void* args);


#endif /* AGGREGATION_FLOW_UPDATING_H_ */
