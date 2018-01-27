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

#ifndef GLS_MEMBERS_H_
#define GLS_MEMBERS_H_

#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

typedef enum _member_attr {
	DIRECT = 0,
	INDIRECT = 1,
	DEAD = 69
} member_attr;

typedef struct _member {
	uuid_t uuid;
	short counter;
	member_attr attr;
} member;

typedef struct _members {
	int size;
	int max;
	member* list;
} members;

/* Initialises a members struct */
void init_members(members* m_list);

/* Inserts a member into the list, grows it if needed */
member* insert_member(members* m_list, const uuid_t uuid, const short counter, const member_attr attr);

/* Removes a member from the list */
void remove_member(members* m_list, int memb_pos);

/* Finds a member in a list, returns it's pos if it exists, if not -1 */
int find_member(members* m_list, const uuid_t uuid);

/* select a random member from the list and remove it */
void randomSelect(members* m_list, member* m);

/* free a membership */
void destroy_membership(members* m_list);

#endif /* GLS_MEMBERS_H_ */
