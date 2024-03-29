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


#ifndef LK_NETWORK_H_
#define LK_NETWORK_H_

#include <linux/nl80211.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "lk_errors.h"
#include "lk_data_struct.h"
#include "lk_hw.h"


int leaveMesh(Interface* itf);
int connectToMesh(Interface* itf, Network* net);

int leaveAdHoc(Interface* itf);
int connectToAdHoc(Interface* itf, Network* net);

struct trigger_results {
    int done;
    int aborted;
};


Network* scanNetworks(Channel* ch);

#endif /* LK_NETWORK_H_ */
