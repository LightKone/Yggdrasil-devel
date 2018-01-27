/*
 * simple_aggregation.h
 *
 *  Created on: 16/12/2017
 *      Author: akos
 */

#ifndef AGGREGATION_PROTOS_SIMPLE_AGGREGATION_H_
#define AGGREGATION_PROTOS_SIMPLE_AGGREGATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "aggregation_interface.h"
#include "aggregation_functions.h"
#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"
#include "contribution.h"

void* simple_aggregation_init(void* args);


#endif /* AGGREGATION_PROTOS_SIMPLE_AGGREGATION_H_ */
