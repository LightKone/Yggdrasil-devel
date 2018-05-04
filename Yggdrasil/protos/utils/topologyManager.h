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

#ifndef TOPOLOGYMANAGER_H_
#define TOPOLOGYMANAGER_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core/lk_runtime.h"
#include "core/proto_data_struct.h"
#include "core/utils/queue.h"
#include "core/utils/utils.h"
#include "lightkone.h"

typedef enum topologyMan_REQ {
	CHANGE_LINK
}topologyMan_req;

void * topologyManager_init(void * args);

#endif /* TOPOLOGYMANAGER_H_ */
