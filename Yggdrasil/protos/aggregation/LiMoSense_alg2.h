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

#ifndef AGGREGATION_LIMOSENSE_H_
#define AGGREGATION_LIMOSENSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "lightkone.h"
#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"

#include "../discovery/discovery_ft_lb.h"

#include "aggregation_gen_interface.h"

void* LiMoSense_alg2_init(void* args);


#endif /* AGGREGATION_LIMOSENSE_H_ */
