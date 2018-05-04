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

#include "aggregation_gen_interface.h"

short aggregation_get_value(agg_val val, double* value){

	switch(val){
	case DEFAULT:
		*value = getValue();
		break;
	default:
		return FAILED;
		break;
	}

	return SUCCESS;
}

short aggregation_get_value_for_op(agg_val valdef, agg_gen_op op, void* value){
	double val;
	int one = 1;
	if(op == OP_COUNT){
		memcpy(value, &one, sizeof(int));
		return SUCCESS;
	}
	if(aggregation_get_value(valdef, &val) == SUCCESS){

		switch(op){
			case OP_SUM:
			case OP_MAX:
			case OP_MIN:
				memcpy(value, &val, sizeof(double));
				break;
			case OP_AVG:
				memcpy(value, &val, sizeof(double));
				memcpy(value+sizeof(double), &one, sizeof(int));
				break;
			default:
				return FAILED;
			}
			return SUCCESS;
	}

	return FAILED;
}

agg_meta* aggregation_deserialize_agg_meta(unsigned short length, void* payload){

	agg_meta* metadata = NULL;

	if(length >= sizeof(agg_gen_op) + sizeof(agg_val) + sizeof(unsigned short)){

		metadata = malloc(sizeof(agg_meta));
		void* tmp = payload;

		memcpy(&metadata->op, tmp, sizeof(agg_gen_op));
		tmp += sizeof(agg_gen_op);
		memcpy(&metadata->val, tmp, sizeof(agg_val));
		tmp += sizeof(agg_val);
		memcpy(&metadata->opt_size, tmp, sizeof(unsigned short));
		tmp += sizeof(unsigned short);
		if(metadata->opt_size > 0){
			metadata->opt = malloc(metadata->opt_size);
			memcpy(metadata->opt, tmp, metadata->opt_size);
		}else{
			metadata->opt = NULL;
		}
	}
	return metadata;
}

unsigned short aggregation_serialize_agg_meta(void* payload, agg_meta* metadata){
	unsigned short size = 0;
	void* tmp = payload;

	memcpy(tmp, &metadata->op, sizeof(agg_gen_op));
	tmp += sizeof(agg_gen_op);
	memcpy(tmp, &metadata->val, sizeof(agg_val));
	tmp += sizeof(agg_val);
	memcpy(tmp, &metadata->opt_size, sizeof(unsigned short));
	tmp += sizeof(unsigned short);
	if(metadata->opt_size > 0)
		memcpy(tmp, metadata->opt, metadata->opt_size);

	size = sizeof(agg_gen_op) + sizeof(agg_val) + sizeof(unsigned short) + metadata->opt_size;

	return size;
}

agg_meta* aggregation_meta_init(agg_gen_op op, agg_val valdef, short nopts){

	agg_meta* metadata = malloc(sizeof(agg_meta));

	metadata->op = op;
	metadata->val = valdef;

	metadata->opt_size = 0;
	if(nopts > 0)
		metadata->opt = malloc(sizeof(short)*2*nopts);
	else
		metadata->opt = NULL;

	return metadata;
}

unsigned short aggregation_meta_add_opt(agg_meta* metadata, short opt_code, short opt_val){

	if(metadata->opt != NULL){
		void* tmp = metadata->opt + metadata->opt_size;
		memcpy(tmp, &opt_code, sizeof(short));
		tmp += sizeof(short);
		memcpy(tmp, &opt_val, sizeof(short));

		metadata->opt_size += sizeof(short)*2;
	}

	return metadata->opt_size;

}

void aggregation_meta_destroy(agg_meta** metadata){
	if((*metadata)->opt != NULL){
		free((*metadata)->opt);
		(*metadata)->opt = NULL;
	}
	free(*metadata);
	*metadata = NULL;
}

