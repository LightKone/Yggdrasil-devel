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

#include "single_tree_node.h"

node* stree_node_create(const uuid_t id, const WLANAddr src_addr) {
	node* n = malloc(sizeof(node));

	memcpy(n->addr.data, src_addr.data, WLAN_ADDR_LEN);
	memcpy(n->uuid, id, sizeof(uuid_t));

	n->next = NULL;
	return n;
}

void stree_node_add(node** lst, const uuid_t id, const WLANAddr src_addr) {
	// create the node and add it to the list
	node* n = stree_node_create(id, src_addr);
	n->next = *lst;
	*lst = n;
}

void stree_node_remove(node** lst, const uuid_t id) {
	char uuid_str2[37];
	memset(uuid_str2,0,37);
	uuid_unparse(id, uuid_str2);
	//printf("try removal %s\n",uuid_str2);

	node* curr = *lst;
	node* prev = NULL;

	while (curr) {
			char uuid_str21[37];
			memset(uuid_str21,0,37);
			uuid_unparse(curr->uuid, uuid_str21);
			//printf("found %s\n",uuid_str21);

			curr = curr->next;
		}
	curr = *lst;

	while (curr) {
		if (!uuid_compare(curr->uuid, id)) {
			//printf("got it\n", id);

			if(!prev) {
				// remove first element
				*lst = (*lst)->next;
			} else {
				// remove element in the middle or end
				prev->next = curr->next;
			}

			free(curr);
			return;
		}

		prev = curr;
		curr = curr->next;
	}
	//printf("not found\n");
}

node* stree_node_find(const node* lst, const uuid_t id) {
	node* tmp = (node*)lst;
	while (tmp != NULL) {
		if (!uuid_compare(tmp->uuid, id))
			return tmp;
		else
			tmp = tmp->next;
	}

	return NULL;
}

// returns a deep clone copy of a given list
node* stree_node_deep_clone(const node* src) {
	node* dst = NULL;
	node* tmp = (node*)src;

	while(tmp) {
		stree_node_add(&dst, tmp->uuid, tmp->addr);
		tmp = tmp->next;
	}

	return dst;
}
