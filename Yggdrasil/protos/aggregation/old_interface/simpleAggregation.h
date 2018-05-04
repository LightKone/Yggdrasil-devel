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

#ifndef SIMPLEAGGREGATION_H_
#define SIMPLEAGGREGATION_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#include "lightkone.h"
#include "core/lk_runtime.h"
#include "core/utils/utils.h"
#include "core/proto_data_struct.h"

typedef enum SIMPLE_AGG_EVENT_{
	SIMPLE_AGG_EVENT_REQUEST_VALUE,
	SIMPLE_AGG_EVENT_MAX
}SIMPLE_AGG_EVENT;

typedef enum SIMPLE_AGG_REQ_ {
	SIMPLE_AGG_REQ_DO_SUM,
	SIMPLE_AGG_REQ_DO_MAX,
	SIMPLE_AGG_REQ_DO_MIN,
	SIMPLE_AGG_REQ_DO_AVG,
	SIMPLE_AGG_REQ_GET_SUM,
	SIMPLE_AGG_REQ_GET_MAX,
	SIMPLE_AGG_REQ_GET_MIN,
	SIMPLE_AGG_REQ_GET_AVG,
	SIMPLE_AGG_REPLY_FOR_VALUE
}SIMPLE_AGG_REQ;

typedef struct contribution_t_ contribution_t;

typedef struct aggValue_ aggValue;
typedef double (*aggregation_function) (double value1, double value2);

double sumFunc(double value1, double value2);
double minFunc(double value1, double value2);
double maxFunc(double value1, double value2);

void * simpleAggregation_init(void * args);

aggValue* simpleAggregation_deserializeReply(void* data, unsigned short datalen);

char* simpleAggregation_contribution_getId(contribution_t* contribution, char str[37]);
double simpleAggregation_contribution_getValue(contribution_t* contribution);
contribution_t* simpleAggregation_contribution_getNext(contribution_t* contribution);

void simpleAggregation_aggValue_destroy(aggValue** av);

double simpleAggregation_aggValue_getValue(aggValue* av);
short simpleAggregation_aggValue_getNContributions(aggValue* av);
contribution_t* simpleAggregation_aggValue_getContributions(aggValue* av);




#endif /* SIMPLEAGGREGATION_H_ */
