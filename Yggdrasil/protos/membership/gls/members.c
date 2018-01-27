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

#include "members.h"

/* Initialises a members struct */
void init_members(members* m_list) {
	m_list->size = 0;
	m_list->max = 8;
	m_list->list = malloc(sizeof(member) * m_list->max);
}

/* Inserts a new member into the list, grows it if needed */
member* insert_member(members* m_list, const uuid_t uuid, const short counter, const member_attr attr) {
	// check if need to grow
	if (m_list->size >= m_list->max) {
		m_list->max *= 2;
		m_list->list = realloc(m_list->list, sizeof(member) * (m_list->max));
	}

	member* new_member = m_list->list + m_list->size;
	memcpy(new_member->uuid, uuid, sizeof(uuid_t));
	new_member->counter = counter;
	new_member->attr = attr;
	m_list->size++;

	return new_member;
}

/* Removes a member from the list */
void remove_member(members* m_list, int memb_pos) {
	const int last_memb = m_list->size - 1;

	// if we aren't removing the last neighbour, take the last
	// neighbour and put it on top of what we want to remove
	if (memb_pos != last_memb) {
		memcpy(m_list->list + memb_pos, m_list->list + last_memb, sizeof(member));
	}

	// decrement counter
	m_list->size--;
}

/* Finds a member in a list, returns it's pos if it exists, if not -1 */
int find_member(members* m_list, const uuid_t uuid) {
	int i;
	for(i = 0; i < m_list->size; i ++){
		if(!uuid_compare(uuid, m_list->list[i].uuid))
			return i;
	}

	return -1;
}

void randomSelect(members* m_list, member* m) {
	int r = rand() % m_list->size;

	memcpy(m,m_list->list + r, sizeof(member));
	if(r+1 < m_list->size){
		int t = m_list->size - (r + 1);
		member temp[t];
		memcpy(temp, m_list->list + (r + 1), sizeof(member)*t);
		memcpy(m_list->list + r, temp, sizeof(member)*t);
	}

	m_list->size --;

}

void destroy_membership(members* m_list){
	free(m_list->list);
	free(m_list);
}
