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

#include <sys/time.h>
#include <sys/types.h>

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

	fd_set readfds;


	int sequence_number = 0;
	while(1) {

		FD_ZERO(&readfds);
		FD_SET(ch.sockid, &readfds);

		struct timeval tv;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

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


		int r = select(ch.sockid + 1, &readfds, NULL, NULL, &tv);
		printf("Select returned %d\n", r);
		if(r < 0){
			perror("FODEU: ");
		}
		while(r > 0){
			printf("Waiting for reply\n");
			int b = chreceive(&ch, &message);
			if(b > 0) {
				memset(origin,0,33);
				memset(dest,0,33);
				char buffer2[MAX_PAYLOAD+1];
				memset(buffer2, 0, MAX_PAYLOAD+1);
				int s = deserializeLKPhyMessage(&message, b, buffer2, MAX_PAYLOAD);

				if(s == 1) {
					wlan2asc(&message.phyHeader.srcAddr, origin);
					wlan2asc(&message.phyHeader.destAddr, dest);
					printf("Received Message Content: %s, from  %s to %s\n",buffer, origin, dest);

				} else {
					printf("This is not a Lightkone message\n");
				}
			} else {
				fprintf(stderr, "Error receiving from channel: %s\n", strerror(b));
			}

			FD_ZERO(&readfds);
			FD_SET(ch.sockid, &readfds);

			r = select(1, &readfds, NULL, NULL, &tv);
		}

	}

	return 0;
}
