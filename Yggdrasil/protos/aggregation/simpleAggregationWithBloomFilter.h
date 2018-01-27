/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2017
 *********************************************************/

#ifndef SIMPLEAGGREGATIONWITHBLOOMFILTER_H_
#define SIMPLEAGGREGATIONWITHBLOOMFILTER_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#include "lightkone.h"
#include "core/lk_runtime.h"
#include "core/utils/utils.h"
#include "core/proto_data_struct.h"
#include "core/utils/bloomfilter/bloom.h"
#include "core/utils/hashfunctions.h"
#include "simpleAggregation.h"

typedef struct aggValueBloom_ aggValueBloom;

void * simpleAggregationWithBloomFilter_init(void * args);

aggValueBloom* simpleAggregationWithBloomFilter_deserializeReply(void* data, unsigned short datalen);


void simpleAggregationWithBloomFilter_aggValueBloom_destroy(aggValueBloom** av);

double simpleAggregationWithBloomFilter_aggValueBloom_getValue(aggValueBloom* av);
short simpleAggregationWithBloomFilter_aggValueBloom_getNContributions(aggValueBloom* av);
bloom_t simpleAggregationWithBloomFilter_aggValueBloom_getContributions(aggValueBloom* av);


#endif /* SIMPLEAGGREGATIONWITHBLOOMFILTER_H_ */
