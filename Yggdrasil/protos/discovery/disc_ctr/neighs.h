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

#ifndef DISC_CTR_NEIGHS_H_
#define DISC_CTR_NEIGHS_H_

#include <stdlib.h>
#include <string.h>
#include "protos/discovery/discovery.h"

typedef struct _neighbor_ctr {
	short id;
	short counter;
	WLANAddr addr;
	uuid_t uuid;
} neigh_ctr;

typedef struct _neighbors_ctr {
	uuid_t myuuid;
	short myctr;
	neigh_ctr* list;
	int known;
	int neigh_max;
	pthread_mutex_t lock;

} neighbors_ctr;

/* Initialises a neighbors_ctr* struct */
void init_neighs_ctr(neighbors_ctr* neighs_ctr, const uuid_t myuuid);

/* Inserts a neighbor into the list, grows it if needed */
neigh_ctr* insert_neigh(neighbors_ctr* neighs_ctr, const WLANAddr addr, const uuid_t uuid, const short counter);

/* Removes a neighbor from the list */
void remove_neigh(neighbors_ctr* neighs_ctr, int neigh_pos);

/* Finds a neighbour in a list, returns it's pos if it exists, if not -1 */
int find_neigh(neighbors_ctr* neighs_ctr, const uuid_t uuid);

#endif /* DISC_CTR_NEIGHS_H_ */
