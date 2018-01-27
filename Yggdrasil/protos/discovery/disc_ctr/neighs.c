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

#include "neighs.h"

/* Initialises a neighbors_ctr* struct */
void init_neighs_ctr(neighbors_ctr* neighs_ctr, const uuid_t myuuid) {
	memcpy(neighs_ctr->myuuid, myuuid, UUID_SIZE);

	neighs_ctr->myctr = 0;
	neighs_ctr->known = 0;
	neighs_ctr->neigh_max = 1;
	neighs_ctr->list = malloc(sizeof(neigh) * 1); //TODO dynamic size??

	pthread_mutex_init((&neighs_ctr->lock), NULL); //fine grain locking ??? for each neigh???
}

/* Inserts a neighbor into the list, grows it if needed */
neigh_ctr* insert_neigh(neighbors_ctr* neighs_ctr, const WLANAddr addr, const uuid_t uuid, const short counter) {
	// check if need to grow
	if (neighs_ctr->known >= neighs_ctr->neigh_max) {
		neighs_ctr->neigh_max *= 2;
		neighs_ctr->list = realloc(neighs_ctr->list, sizeof(neigh_ctr) * (neighs_ctr->neigh_max));
	}

	neigh_ctr* new_neigh = neighs_ctr->list + neighs_ctr->known;

	memcpy(new_neigh->addr.data, addr.data, WLAN_ADDR_LEN);
	memcpy(new_neigh->uuid, uuid, UUID_SIZE);
	new_neigh->counter = counter;
	new_neigh->id = neighs_ctr->known++;

	return new_neigh;
}

/* Removes a neighbor from the list */
void remove_neigh(neighbors_ctr* neighs_ctr, int neigh_pos) {
	const int last_neigh = neighs_ctr->known - 1;

	// if we aren't removing the last neighbour, take the last
	// neighbour and put it on top of what we want to remove
	if (neigh_pos != last_neigh) {
		memcpy(neighs_ctr->list + neigh_pos, neighs_ctr->list + last_neigh, sizeof(neigh_ctr));
	}

	// decrement counter
	neighs_ctr->known--;
}

/* Finds a neighbour in a list, returns it's pos if it exists, if not -1 */
int find_neigh(neighbors_ctr* neighs_ctr, const uuid_t uuid) {
	int i;
	for (i = 0; i < neighs_ctr->known; i++) {
		if (uuid_compare(uuid, (neighs_ctr->list + i)->uuid) == 0) {
			return i;
		}
	}
	return -1;
}
