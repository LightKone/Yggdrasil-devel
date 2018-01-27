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

#include "protos/control/control_protocol_server.h"

static queue_t* inBox;
static short protoID;
static short commandTreeProtoID;

void executeClientSession(int clientSocket, struct sockaddr_in* addr) {

	char clientIP[16];
	inet_ntop(AF_INET, &addr->sin_addr, clientIP, 16);
	printf("CONTROL PROTOCOL SERVER - Starting session for a client located with address %s:%d\n", clientIP, addr->sin_port);

	int command_code;
	int lenght;
	int stop = 0;
	char* reply = malloc(50);

	while(stop == 0) {
		bzero(reply, 50);
		if(readfully(clientSocket, &command_code, sizeof(int)) < 0) { stop = 1; continue; }

		switch(command_code) {
		case COMMAND_QUIT:
			sprintf(reply,"Bye bye");
			lenght = strlen(reply) + 1;
			writefully(clientSocket, &lenght, sizeof(int));
			writefully(clientSocket, reply, lenght);
			stop = 1;
			break;
		default:
			sprintf(reply,"Unknown Command");
			lenght = strlen(reply) + 1;
			writefully(clientSocket, &lenght, sizeof(int));
			writefully(clientSocket, reply, lenght);
		}

	}

	printf("CONTROL PROTOCOL SERVER - Terminating current session with client\n");
	close(clientSocket);
}

void * control_protocol_server_init(void * args) {

	proto_args* pargs = args;
	inBox = pargs->inBox;
	protoID = pargs->protoID;
	commandTreeProtoID = (unsigned short) ((long) pargs->protoAttr);

	int listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	bzero(&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons((unsigned short) 5000);

	if(bind(listen_socket, (const struct sockaddr*) &address, sizeof(address)) == 0 ) {

		if(listen(listen_socket, 20) < 0) {
			perror("CONTROL PROTOCOL SERVER - Unable to setup listen on socket: ");
			return NULL;
		}

		while(1) {
			//Main control cycle... we handle a single client each time...
			bzero(&address, sizeof(struct sockaddr_in));
			unsigned int length = sizeof(struct sockaddr_in);
			int client_socket = accept(listen_socket, (struct sockaddr*) &address, &length);

			executeClientSession(client_socket, &address);
		}

	} else {
		perror("CONTROL PROTOCOL SERVER - Unable to bind on listen socket: ");
		return NULL;
	}

	return NULL;
}
