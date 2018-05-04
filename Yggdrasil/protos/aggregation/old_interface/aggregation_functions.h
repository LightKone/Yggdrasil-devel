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

#ifndef AGGREGATION_PROTOS_AGGREGATION_FUNCTIONS_H_
#define AGGREGATION_PROTOS_AGGREGATION_FUNCTIONS_H_

#include "aggregation_interface.h"
#include "protos/aggregation/aggregation_gen_functions.h"

aggregation_function aggregation_function_get(agg_op op);
void* aggregation_function_get_value(agg_op op, void* value);
value_size_t aggregation_function_get_valueSize(agg_op op);



#endif /* AGGREGATION_PROTOS_AGGREGATION_FUNCTIONS_H_ */
