#ifndef SINGLETREENODE_H_
#define SINGLETREENODE_H_

#include <uuid/uuid.h>
#include "lightkone.h"
#include "core/utils/utils.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"

typedef struct node_t{
	WLANAddr addr;
	uuid_t uuid;
	struct node_t* next;
}node;

void node_rem(node** lst, node* toRemove);
void node_add(node** lst, node* toAdd);

node* node_create(const WLANAddr src_addr, const uuid_t* id);

node* node_get_copy(node* original);
node* node_get_copy_with_exclude(node* original, uuid_t exclude);

#endif /* SINGLETREENODE_H_ */
