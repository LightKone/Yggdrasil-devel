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

