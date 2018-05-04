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

#ifndef AGGREGATION_PROTOS_AGGREGATION_GEN_INTERFACE_H_
#define AGGREGATION_PROTOS_AGGREGATION_GEN_INTERFACE_H_

#include <stdlib.h>
#include <stdio.h>
#include <uuid/uuid.h>

#include "lightkone.h"
#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"

typedef enum AGG_VAL {
	//POPULATE WITH POSSIBLE VALUE UPON TO DO AGGREGATION
	DEFAULT //use default value to compute aggregation value (test value)
}agg_val;

typedef enum AGG_GEN_OP{
	OP_SUM = 0,
	OP_MAX = 1,
	OP_MIN = 2,
	OP_AVG = 3,
	OP_COUNT = 4,
	OP_NONE = -1
}agg_gen_op;

typedef enum AGG_REQ {
	AGG_INIT, //start a new instance of aggregation, given the aggregation op
	AGG_GET, //gets the current aggregation value given the aggregation op
	AGG_VAL, //gets the result of an aggregation
	AGG_NOT_SUPPORTED, //reply for an invalid request
	AGG_FAILED, //error message
	AGG_TERMINATE //kill
}agg_req;

typedef struct agg_meta_ {
	agg_gen_op op;
	agg_val val;
	unsigned short opt_size;
	void* opt; //some additional options (yet to be defined, and may be protocol specific)
}agg_meta;

short aggregation_get_value(agg_val val, double* value);
short aggregation_get_value_for_op(agg_val valdef, agg_gen_op op, void* value);


agg_meta* aggregation_deserialize_agg_meta(unsigned short length, void* payload);

unsigned short aggregation_serialize_agg_meta(void* payload, agg_meta* metadata); //payload should have the necessary space to accommodate metadata


agg_meta* aggregation_meta_init(agg_gen_op op, agg_val valdef, short nopts);

unsigned short aggregation_meta_add_opt(agg_meta* metadata, short opt_code, short opt_val);

void aggregation_meta_destroy(agg_meta** metadata);

//void aggregation_reportValue(LKRequest* request, short protoId, double value);

//double aggregation_getValue(LKRequest* reply);

#endif /* AGGREGATION_PROTOS_AGGREGATION_INTERFACE_H_ */
