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
#include <unistd.h>

#include "lk_api.h"
#include "lk_data_struct.h"
#include "lk_errors.h"
#include "lk_constants.h"


int main(int argc, char** argv) {

	Channel ch;
	char* type = "AdHoc"; //should be an argument

	NetworkConfig* ntconf = defineNetworkConfig(type, 0, 5, 0, "ledge", LKM_filter);

	if(setupSimpleChannel(&ch, ntconf) != SUCCESS){
		printf("Failed to setup channel for communication");
		return -1;
	}

	if(setupChannelNetwork(&ch, ntconf) != SUCCESS){
			printf("Failed to setup channel network for communication");
			return -1;
	}

	int sequence_number = 0;
		while(1) {
			sequence_number++;
			char buffer[MAX_PAYLOAD];
			memset(buffer, 0, MAX_PAYLOAD);
			sprintf(buffer,"This is a lightkone message with sequence: %d",sequence_number);
			LKPhyMessage message;
			message.phyHeader.type = IP_TYPE;
			char id[] = AF_LK_ARRAY;
			memcpy(message.lkHeader.data, id, LK_HEADER_LEN);
			int len = strlen(buffer);
			message.dataLen = len+1;
			memcpy(message.data, buffer, len+1);
			len = chsend(&ch, &message);

			int checkType = isLightkoneProtocolMessage(&message, sizeof(message));
			if(checkType != 0) { printf("is LK message\n"); } else { printf("is not LK message\n"); }
			char origin[33], dest[33];
			memset(origin,0,33);
			memset(dest,0,33);
			wlan2asc(&message.phyHeader.srcAddr, origin);
			wlan2asc(&message.phyHeader.destAddr, dest);
			printf("Sent %d bytes to the network (index: %d) from %s to %s\n", len, sequence_number, origin, dest);
			//sleep(2);
		}

		return 0;
}
