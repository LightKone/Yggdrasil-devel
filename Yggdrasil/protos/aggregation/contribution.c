/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2017
 *********************************************************/

#include "contribution.h"

struct contribution_t_{
	uuid_t id;
	unsigned short size;
	void* value;
	struct contribution_t_* next;
};

contribution_t* contribution_init(uuid_t id, void* value, unsigned short sizeofValue){
	contribution_t* contribution = malloc(sizeof(contribution_t));
	memcpy(contribution->id, id, sizeof(uuid_t));
	contribution->size = sizeofValue;
	contribution->value = malloc(sizeofValue);
	memcpy(contribution->value, value, sizeofValue);
	contribution->next = NULL;
	return contribution;
}

void contribution_destroy(contribution_t** contributions){
	while(*contributions != NULL){
		contribution_t* old = *contributions;
		*contributions = old->next;
		old->next = NULL;
		free(old->value);
		free(old);
	}
}

void contribution_addContribution(contribution_t** head, contribution_t** newcontribution){

	if(*newcontribution == NULL) return;

	contribution_t* next = (*newcontribution)->next;

	(*newcontribution)->next = *head;
	*head = *newcontribution;

	*newcontribution = next;
}

contribution_t* contribution_getNewContributions(contribution_t* mine, contribution_t** other){

	contribution_t* newcontributions = NULL;
	contribution_t* it_new;


	while((*other) != NULL) {
		contribution_t* it_mine = mine;
		int new = 1;
		while(it_mine != NULL){
			if(uuid_compare(it_mine->id, (*other)->id) == 0){
				new = 0;
				break;
			}
			it_mine = it_mine->next;
		}
		if(new == 1) {
			if(newcontributions == NULL){
				newcontributions = *other;

				(*other) = newcontributions->next;
				newcontributions->next = NULL;
				it_new = newcontributions;

#ifdef DEBUG
				char u[37];
				memset(u, 0, 37);
				char m[200];
				memset(m,0,200);
				contribution_getCharId(it_new, u);
				sprintf(m, "added new elem %s", u);
				lk_log("SIMPLE AGG","ALIVE", m);
#endif

			}else{
				it_new->next = (*other);
				it_new = it_new->next;
				(*other) = it_new->next;
#ifdef DEBUG
				char u[37];
				memset(u, 0, 37);
				char m[200];
				memset(m,0,200);
				contribution_getCharId(it_new, u);
				sprintf(m,"added new elem %s", u);
				lk_log("SIMPLE AGG","ALIVE", m);
#endif
				if(it_new != NULL)
					it_new->next = NULL;
			}

		}else{
			contribution_t* old = *other;
			*other = old->next;
			old->next = NULL;
			free(old->value);
			free(old);
		}
	}

	return newcontributions;
}

short contribution_serialize(void** payload, unsigned short max, contribution_t* contributions){
	unsigned short len = 0;
	while(contributions != NULL){
		unsigned short contribution_t_size = sizeof(unsigned short) + contributions->size + sizeof(uuid_t);
		if(len + contribution_t_size >  max)
			return -1;

		memcpy(*payload, &contributions->size, sizeof(unsigned short));
		*payload += sizeof(unsigned short);
		memcpy(*payload, contributions->value, contributions->size);
		*payload += contributions->size;
		memcpy(*payload, contributions->id, sizeof(uuid_t));
		*payload += sizeof(uuid_t);

		len+= contribution_t_size;
		contributions = contributions->next;
	}
	return len;
}

contribution_t* contribution_deserialize(void** payload, unsigned short* len, short nContributions) {
	contribution_t* contributions = NULL;

	short i = 0;
	while(i < nContributions && *len > sizeof(unsigned short)) {
		contribution_t* contribution = malloc(sizeof(contribution_t));

		memcpy(&contribution->size, *payload, sizeof(unsigned short));
		unsigned short contribution_t_size = sizeof(unsigned short) + contribution->size + sizeof(uuid_t);

		if(*len >= contribution_t_size){
			*payload += sizeof(unsigned short);

			contribution->value = malloc(contribution->size);

			memcpy(contribution->value, *payload, contribution->size);
			*payload += contribution->size;

			memcpy(contribution->id, *payload, sizeof(uuid_t));
			*payload += sizeof(uuid_t);

			*len -= contribution_t_size;
			contribution->next = contributions;
			contributions = contribution;
			i++;
		}else{
			free(contribution);
			break;
		}

	}

	return contributions;
}

char* contribution_getCharId(contribution_t* contribution, char str[37]) {
	memset(str, 0, 37);
	uuid_unparse(contribution->id, str);
	return str;
}

void contribution_getId(contribution_t* contribution, uuid_t uuid) {
	memcpy(uuid, contribution->id, sizeof(uuid_t));
}

short contribution_getValue(contribution_t* contribution, void* value, unsigned short valuelen) {
	if(valuelen < contribution->size)
		return -1;
	memcpy(value, contribution->value, contribution->size);
	return contribution->size;
}

unsigned short contribution_getValueSize(contribution_t* contribution) {
	return contribution->size;
}

contribution_t* contribution_getNext(contribution_t* contribution) {
	return contribution->next;
}
