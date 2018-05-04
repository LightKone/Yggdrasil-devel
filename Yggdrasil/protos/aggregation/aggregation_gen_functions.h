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

#ifndef AGGREGATION_PROTOS_AGGREGATION_GEN_FUNCTIONS_H_
#define AGGREGATION_PROTOS_AGGREGATION_GEN_FUNCTIONS_H_

#include <string.h>
#include <stdlib.h>
#include <float.h>

#include "aggregation_gen_interface.h"

typedef void* (*aggregation_function) (void* value1, void* value2, void* newvalue);

typedef enum value_size_t_{
	NOTDEFINEDVALSIZE = -1,
	SUMVALSIZE = sizeof(double),
	MINVALSIZE = sizeof(double),
	MAXVALSIZE = sizeof(double),
	AVGVALSIZE = sizeof(double) + sizeof(int),
	COUNTSIZE = sizeof(int)
}value_size_t;

aggregation_function aggregation_gen_function_get(agg_gen_op op);

void aggregation_gen_function_setNeutral(agg_gen_op op, void* neutral_val);

short aggregation_valueDiff(void* value1, void* value2);

short aggregation_gen_function_getValsize(agg_gen_op op);

/**
 * Sums two doubles and stores the value in newvalue as a double
 * @param value1 The first value as a double to sum
 * @param value2 The second value as a double to sum
 * @param newvalue Store the summed value as a double
 * @return newvalue the summed value as a double
 */
void* sumFunction(void* value1, void* value2, void* newvalue);

/**
 * Calculates the minimum of two doubles and stores it in newvalue as a double
 * @param value1 The first value as a double
 * @param value2 The second value as a double
 * @param newvalue Store the min value as a double
 * @return newvalue the min value as a double
 */
void* minFunction(void* value1, void* value2, void* newvalue);

/**
 * Calculates the maximum of two doubles and stores it in newvalue as a double
 * @param value1 The first value as a double
 * @param value2 The second values as a double
 * @param newvalue Store the max value as a double
 * @return newvalue the max value as a double
 */
void* maxFunction(void* value1, void* value2, void* newvalue);

/**
 * Calculates the new average given two composite average values (double average, int count)
 * where average is the average of the summed values of count
 * as avg = sum / count <=> avg*count = sum
 * as such -> avg = f(avg1,avg2) => (avg1*count1 + avg2*count2 ) / (count1 + count2)
 * @param value1 The first value as a composite (double average, int count)
 * @param value2 The second value as a composite (double average, int count)
 * @param newvalue Store the new average as a composite (double average, int count)
 * @param newvalue the average as a composite (double average, int count)
 */
void* avgFunction(void* value1, void* value2, void* newvalue);


#endif /* AGGREGATION_PROTOS_AGGREGATION_FUNCTIONS_H_ */
