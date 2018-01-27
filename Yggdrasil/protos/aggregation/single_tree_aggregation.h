#ifndef SINGLETREEAGGREGATION_H_
#define SINGLETREEAGGREGATION_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>

#include "lightkone.h"

#include "core/utils/utils.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"
#include "simple_aggregation.h"
#include "protos/discovery/disc_ctr/neighs.h"

#include "aggregation_interface.h"
#include "aggregation_functions.h"
#include "core/proto_data_struct.h"
#include "single_tree_node.h"

typedef enum MESSAGE_TYPE{
	QUERY = 1,
	CHILD = 2,
	VALUE = 3
}msg_type;

typedef enum CHILD_RESPONSE{
	YES = 1,
	NO = 2
}child_response;

void* stree_init(void* args);

#endif /* SINGLETREEAGGREGATION_H_ */
