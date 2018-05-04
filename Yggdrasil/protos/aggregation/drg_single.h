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

#ifndef AGGREGATION_DRG_SINGLE_H_
#define AGGREGATION_DRG_SINGLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"

#include "../discovery/discovery.h"
#include "../discovery/discovery_ft_lb.h" //ev up; ev down
#include "../communication/point-to-point/reliable.h"

#include "aggregation_gen_interface.h"

typedef enum drg_opt_code_{
	PROB = 1
}drg_opt_code;

typedef struct drg_attr_{
	short discovery_proto_id;
	short reliable_proto_id;
}drg_attr;

void* drg_single_init(void* args);


#endif /* AGGREGATION_DRG_SINGLE_H_ */
