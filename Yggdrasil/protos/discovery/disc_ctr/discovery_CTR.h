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

#ifndef DISCOVERY_CTR_H_
#define DISCOVERY_CTR_H_


#include <string.h>
#include <stdio.h>
#include <uuid/uuid.h>

#include "core/utils/utils.h"
#include "protos/membership/gls/global_membership.h"
#include "protos/discovery/discovery.h"
#include "protos/faultdetection/faultdetectorLinkBlocker.h"
#include "protos/utils/user_util.h"
#include "neighs.h"

#define CTR_SIZE sizeof(short)
#define PROTO_DISCOV_CTR 100

typedef enum discovery_ctr_events_ {
	DISC_CTR_NEIGHBOR_UP = 0,
	DISC_CTR_EV_MAX = 1
} discovery_ctr_events;

void* discovery_Full_FT_CTR_init(void* arg);

#endif /* DISCOVERY_CTR_H_ */
