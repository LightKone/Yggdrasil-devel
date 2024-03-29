/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2018
 *********************************************************/

#include "lightkone.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"
#include "protos/control/control_discovery.h"
#include "protos/control/control_command_tree.h"
#include "protos/control/control_protocol_server.h"
#include "core/utils/utils.h"
#include "protos/utils/topologyManager.h"

#include <stdlib.h>


int main(int argc, char* argv[]) {

	char* type = "AdHoc"; //should be an argument
	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);


	queue_t *inBox;

	//Init lk_runtime and protocols
	lk_runtime_init(ntconf, 0, 3, 1);

	signal(SIGPIPE, SIG_IGN);

	//addLKProtocol(PROTO_TOPOLOGY_MANAGER, &topologyManager_init, NULL, 0);

	/*short myId = */registerApp(&inBox, 0);
	addProtocol(300, &control_discovery_init, (void*) 5, 1);
	addProtocol(301, &control_command_tree_init, (void*) 300, 0);
	addProtocol(302, &control_protocol_server_init, (void*) 301, 0);

	registInterestEvents(300, (short[]){0}, 1, 301);

	//Start lk_runtime
	lk_runtime_start();

	while(1) {
		printf("Control Structure Test online...\n");
		sleep(30);
	}

	return 0;
}

