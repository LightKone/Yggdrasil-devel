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

#ifndef AGGREGATION_PROTOS_SIMPLE_BLOOM_AGGREGATION_H_
#define AGGREGATION_PROTOS_SIMPLE_BLOOM_AGGREGATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "aggregation_interface.h"
#include "aggregation_functions.h"
#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"
#include "contribution.h"
#include "core/utils/bloomfilter/bloom.h"
#include "core/utils/hashfunctions.h"

void* simple_bloom_aggregation_init(void* args);

#endif /* AGGREGATION_PROTOS_SIMPLE_BLOOM_AGGREGATION_H_ */
