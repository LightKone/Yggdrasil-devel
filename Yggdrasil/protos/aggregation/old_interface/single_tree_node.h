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
} node;

node* stree_node_create(const uuid_t id, const WLANAddr src_addr); // should be static, but used by parent on instance state
void stree_node_add(node** lst, const uuid_t id, const WLANAddr src_addr);
void stree_node_remove(node** lst, const uuid_t id);
node* stree_node_find(const node* lst, const uuid_t id);

node* stree_node_deep_clone(const node* src);

#endif /* SINGLETREENODE_H_ */
