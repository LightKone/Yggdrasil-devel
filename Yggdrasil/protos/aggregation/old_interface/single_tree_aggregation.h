/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * Guilherme Borges (g.borges@campus.fct.unl.pt)
 * (C) 2017
 * This code was produced in the context of the Algorithms
 * and Distributed Systems (2017/18) at DI, FCT, U. NOVA
 * Lectured by João Leitão
 *********************************************************/

#ifndef SINGLETREEAGGREGATION_H_
#define SINGLETREEAGGREGATION_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>

#include "lightkone.h"

#include "core/utils/utils.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"
#include "simple_aggregation.h"
#include "protos/discovery/disc_ctr/neighs.h"
#include "protos/discovery/discovery_ft_lb.h"

#include "aggregation_interface.h"
#include "aggregation_functions.h"
#include "core/proto_data_struct.h"
#include "single_tree_node.h"

typedef enum MESSAGE_TYPE{
	QUERY = 1,
	CHILD = 2,
	VALUE = 3
} msg_type;

typedef enum CHILD_RESPONSE{
	YES = 1,
	NO = 2
} child_response;

void* stree_init(void* args);

#endif /* SINGLETREEAGGREGATION_H_ */
