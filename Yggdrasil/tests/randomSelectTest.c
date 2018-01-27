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

#include "protos/membership/gls/global_membership.h"

int main(int argc, char* argv[]) {

	members* new_members = malloc(sizeof(members));
	init_members(new_members);

	int counter = 0;
	int attr = 0;

	printf("---------INSERT---------\n");

	int i = 0;
	for(; i < 10; i++){

		uuid_t uuid;

		genUUID(uuid);

		insert_member(new_members, uuid, counter, attr);

		char u[37];
		uuid_unparse(uuid, u);

		printf("uuid %s\n", u);

	}

	printf("---------REMOVE---------\n");

	for(i = 0; i < 10; i++){
		member m;
		randomSelect(new_members, &m);

		char u[37];
		uuid_unparse(m.uuid, u);

		printf("uuid %s\n", u);
	}



	destroy_membership(new_members);

}
