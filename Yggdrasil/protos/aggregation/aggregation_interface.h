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

#ifndef AGGREGATION_PROTOS_AGGREGATION_INTERFACE_H_
#define AGGREGATION_PROTOS_AGGREGATION_INTERFACE_H_

#include <stdlib.h>
#include <stdio.h>
#include <uuid/uuid.h>

#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"

typedef enum AGG_OP_ {
	SUM = 0,
	MAX = 1,
	MIN = 2,
	AVG = 3,
	NONE = -1
}agg_op;

typedef enum AGG_REQ_ {
	AGG_REQ_DO_SUM, //start a new instance doing Sum, when terminated returns the aggregated value
	AGG_REQ_DO_MAX, //start a new instance doing Max, when terminated returns the aggregated value
	AGG_REQ_DO_MIN, //start a new instance doing Min, when terminated returns the aggregated value
	AGG_REQ_DO_AVG, //start a new instance doing Avg, when terminated returns the aggregated value
	AGG_REQ_GET_SUM, //return last value if exists else start a new instance for Sum
	AGG_REQ_GET_MAX, //return last value if exists else start a new instance for Max
	AGG_REQ_GET_MIN, //return last value if exists else start a new instance for Min
	AGG_REQ_GET_AVG, //return last value if exists else start a new instance for Avg
	AGG_REPLY_FOR_VALUE
}AGG_REQ;


void aggregation_reportValue(LKRequest* request, short protoId, double value);

double aggregation_getValue(LKRequest* reply);

#endif /* AGGREGATION_PROTOS_AGGREGATION_INTERFACE_H_ */
