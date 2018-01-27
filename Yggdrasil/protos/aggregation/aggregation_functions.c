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

#include "aggregation_functions.h"

aggregation_function aggregation_function_get(agg_op op) {
	switch(op){
	case SUM:
		return sumFunction;
		break;
	case MAX:
		return maxFunction;
		break;
	case MIN:
		return minFunction;
		break;
	case AVG:
		return avgFunction;
		break;
	default:
		break;
	}
	return NULL;
}

void* aggregation_function_get_value(agg_op op, void* value) {

	double v = getValue(); //TODO askvalue from runtime?
	int one = 1;

	switch(op){
		case SUM:
			memcpy(value, &v, sizeof(double));
			return value;
			break;
		case MAX:
			memcpy(value, &v, sizeof(double));
			return value;
			break;
		case MIN:
			memcpy(value, &v, sizeof(double));
			return value;
			break;
		case AVG:
			memcpy(value, &v, sizeof(double));
			memcpy(value+sizeof(double), &one, sizeof(int));
			return value;
			break;
		default:
			break;
		}
		return NULL;
}

value_size_t aggregation_function_get_valueSize(agg_op op) {
	switch(op){
	case SUM:
		return SUMVALSIZE;
		break;
	case MAX:
		return MAXVALSIZE;
		break;
	case MIN:
		return MINVALSIZE;
		break;
	case AVG:
		return AVGVALSIZE;
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
