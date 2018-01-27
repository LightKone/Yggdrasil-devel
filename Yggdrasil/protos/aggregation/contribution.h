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

#ifndef AGGREGATION_PROTOS_CONTRIBUTION_H_
#define AGGREGATION_PROTOS_CONTRIBUTION_H_

#include "core/utils/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

typedef struct contribution_t_ contribution_t;

contribution_t* contribution_init(uuid_t id, void* value, unsigned short sizeofValue);
void contribution_destroy(contribution_t** contributions);

void contribution_addContribution(contribution_t** head, contribution_t** newcontribution);
contribution_t* contribution_getNewContributions(contribution_t* mine, contribution_t** other);

short contribution_serialize(void** payload, unsigned short max, contribution_t* contributions);
contribution_t* contribution_deserialize(void** payload, unsigned short* len, short nContribution);

char* contribution_getCharId(contribution_t* contribution, char str[37]);
void contribution_getId(contribution_t* contribution, uuid_t uuid);
short contribution_getValue(contribution_t* contribution, void* value, unsigned short valuelen);
unsigned short contribution_getValueSize(contribution_t* contribution);
contribution_t* contribution_getNext(contribution_t* contribution);

#endif /* AGGREGATION_PROTOS_CONTRIBUTION_H_ */
