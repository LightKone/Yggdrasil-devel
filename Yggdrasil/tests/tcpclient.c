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

#include "lightkone.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"
#include "core/utils/utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SERVER_PORT 8080

int main(int argc, char* argv[]) {

	if(argc != 4) {
		printf("Usage: %s SERVER_IP COMMAND_CODE COMMAND_ARG\n", argv[0]);
		exit(1);
	}

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	bzero(&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	inet_aton(argv[1], &address.sin_addr);
	address.sin_port = htons((unsigned short) SERVER_PORT);
	int success = connect(sock, (const struct sockaddr*) &address, sizeof(address));

	if(success == 0) {
		int size = sizeof(int)*2;
		int command_code = atoi(argv[2]);
		int command_arg = atoi(argv[3]);

		if(command_code == 1){
			void* data = malloc(size);
			memcpy(data, &command_code, sizeof(int));
			memcpy(data+sizeof(int), &command_arg, sizeof(int));

			write(sock, &size, sizeof(int));
			write(sock, data, size);

			free(data);

			printf("Operation sent\n");

			//		if(operation >= 0 && operation <= 3) {
			//			double val;
			//			read(sock, &size, sizeof(int));
			//			if(size == sizeof(double)) {
			//				read(sock, &val, sizeof(double));
			//				printf("Final result: %f\n",val);
			//			} else {
			//				printf("Unexpected response from server.\n");
			//			}
			//
			//		} else if (operation >= 4 && operation <= 7) {
			//TODO to be resolved, not supported yet
			//			read(sock, &operation, sizeof(int));
			//			if(operation > 0) {
			//				void* buffer = malloc(operation);
			//				int missing = operation;
			//				while(missing > 0) {
			//					missing -= read(sock, buffer+(operation-missing), missing);
			//				}
			//				aggValue* av = simpleAggregation_deserializeReply(buffer, operation);
			//				//Next line is optional for debug
			//				//printResult(av);
			//
			//				printf("Final result: %f\n",simpleAggregation_aggValue_getValue(av));
			//				simpleAggregation_aggValue_destroy(&av);
			//				free(buffer);
			//			} else {
			//				printf("Final result: <empty>\n");
			//			}
		} else {
			printf("Operation code invalid. Valid operations are:\n");
			printf("1: CHANGE LINK -> 1 arg (raspi number)\n");
		}

		close(sock);
	} else {
		perror("Unable to connect to node");
	}
}
