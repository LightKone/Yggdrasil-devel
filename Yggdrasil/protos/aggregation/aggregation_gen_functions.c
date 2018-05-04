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

#include "aggregation_gen_functions.h"

aggregation_function aggregation_gen_function_get(agg_gen_op op) {
	switch(op){
	case OP_COUNT:
	case OP_SUM:
		return sumFunction;
		break;
	case OP_MAX:
		return maxFunction;
		break;
	case OP_MIN:
		return minFunction;
		break;
	case OP_AVG:
		return avgFunction;
		break;
	default:
		break;
	}
	return NULL;
}

void aggregation_gen_function_setNeutral(agg_gen_op op, void* neutral_val){

	int zero_int = 0;
	double zero_double = 0;
	double max_double = DBL_MAX;
	double min_double = DBL_MIN;

	switch(op){
		case OP_COUNT:
			memcpy(neutral_val, &zero_int, sizeof(int));
			break;
		case OP_SUM:
			memcpy(neutral_val, &zero_double, sizeof(double));
			break;
		case OP_MAX:
			memcpy(neutral_val, &max_double, sizeof(double));
			break;
		case OP_MIN:
			memcpy(neutral_val, &min_double, sizeof(double));
			break;
		case OP_AVG:
			memcpy(neutral_val, &zero_double, sizeof(double));
			memcpy(neutral_val + sizeof(double), &zero_int, sizeof(int));
			break;
		default:
			break;
		}
}

short aggregation_gen_function_getValsize(agg_gen_op op){
	switch(op){
		case OP_SUM:
			return SUMVALSIZE;
			break;
		case OP_MAX:
			return MAXVALSIZE;
			break;
		case OP_MIN:
			return MINVALSIZE;
			break;
		case OP_AVG:
			return AVGVALSIZE;
			break;
		case OP_COUNT:
			return COUNTSIZE;
			break;
		default:
			break;
		}
		return NOTDEFINEDVALSIZE;
}

short aggregation_valueDiff(void* value1, void* value2){
	double val1, val2;

	memcpy(&val1, value1, sizeof(double));
	memcpy(&val2, value2, sizeof(double));

	return val1 - val2;
}

void* sumFunction(void* value1, void* value2, void* newvalue){
	double v1 = *(double*) value1;
	double v2 = *(double*) value2;
	double v = v1 + v2;
	memcpy(newvalue, &v, sizeof(double));
	return newvalue;
}

void* minFunction(void* value1, void* value2, void* newvalue) {
	double v1 = *(double*) value1;
	double v2 = *(double*) value2;
	double v = min(v1, v2);
	memcpy(newvalue, &v, sizeof(double));
	return newvalue;
}

void* maxFunction(void* value1, void* value2, void* newvalue) {
	double v1 = *(double*) value1;
	double v2 = *(double*) value2;
	double v = max(v1, v2);
	memcpy(newvalue, &v, sizeof(double));
	return newvalue;
}

void* avgFunction(void* value1, void* value2, void* newvalue){
	double avg1, avg2, avg;
	int count1, count2, count;

	memcpy(&avg1, value1, sizeof(double));
	memcpy(&avg2, value2, sizeof(double));

	memcpy(&count1, value1 + sizeof(double), sizeof(int));
	memcpy(&count2, value2 + sizeof(double), sizeof(int));

	double sum1, sum2, sum;

	sum1 = avg1*count1;
	sum2 = avg2*count2;

	sum = sum1 + sum2;
	count = count1 + count2;
	avg = sum / count;


	memcpy(newvalue, &avg, sizeof(double));
	memcpy(newvalue + sizeof(double), &count, sizeof(int));

	return newvalue;
}
