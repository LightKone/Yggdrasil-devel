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

#include <stdio.h>

#include "lk_api.h"
#include "lk_data_struct.h"
#include "lk_errors.h"
#include "lk_constants.h"


int main(int argc, char** argv) {

	Channel ch;
	char* type = "AdHoc"; //should be an argument

	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", (struct sock_filter*)LKM_filter);

	if(setupSimpleChannel(&ch, ntconf) != SUCCESS){
		printf("Failed to setup channel for communication");
		return -1;
	}


	if(setupChannelNetwork(&ch, ntconf) != SUCCESS){
		printf("Failed to setup channel network for communication");
		return -1;
	}

	while(1) {
		LKPhyMessage message;
		int b = chreceive(&ch, &message);
		if(b > 0) {
			char origin[33], dest[33];
			memset(origin,0,33);
			memset(dest,0,33);
			char buffer[MAX_PAYLOAD+1];
			memset(buffer, 0, MAX_PAYLOAD+1);
			wlan2asc(&message.phyHeader.srcAddr, origin);
			wlan2asc(&message.phyHeader.destAddr, dest);
			int r = deserializeLKPhyMessage(&message, b, buffer, MAX_PAYLOAD);

			if(r == 1) {
				printf("Received Message from %s to %s Content: %s, replying to src\n",origin, dest,buffer);

				chsendTo(&ch, &message, (char*) message.phyHeader.srcAddr.data); //reply with the same message (for now)

				wlan2asc(&message.phyHeader.srcAddr, origin);
				wlan2asc(&message.phyHeader.destAddr, dest);
				printf("Sent from %s to %s\n", origin, dest);
			} else {
				printf("This is not a Lightkone message\n");
			}
		} else {
			fprintf(stderr, "Error receiving from channel: %s\n", strerror(b));
		}
	}

	return 0;
}
