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

#ifndef AGGREGATION_MULTI_ROOT_AGGREGATION_H_
#define AGGREGATION_MULTI_ROOT_AGGREGATION_H_

#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include <time.h>

#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"

#include "aggregation_gen_interface.h"
#include "aggregation_gen_functions.h"

#include "../discovery/discovery_ft_lb.h" //EVENT:  DISCOV_FT_LB_NEIGH_DOWN

void* multi_root_init(void* args);


#endif /* AGGREGATION_MULTI_ROOT_AGGREGATION_H_ */
