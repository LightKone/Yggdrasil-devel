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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "protos/control/control_protocol_utils.h"

#define BUFSIZE 50

char* read_line(void)
{
	int bufsize = BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		// Read a character
		c = getchar();

		// If we hit EOF, replace it with a null character and return.
		if (c == EOF || c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		// If we have exceeded the buffer, reallocate.
		if (position >= bufsize) {
			bufsize += BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

int executeCommandCheck(int sock) {
	int command = COMMAND_CHECK_CONTROL_STRUCTURE;
	return writefully(sock, &command, sizeof(int));
}

int executeCommandQuit(int sock) {
	int command = COMMAND_QUIT;
	return writefully(sock, &command, sizeof(int));
}

char* getResponse(int sock) {
	char* answer = NULL;
	int size;
	int r = readfully(sock, &size, sizeof(int));
	if(! (r < 0)) {
		answer = malloc(size);
		r = readfully(sock, answer, size);
		if(r < 0) {
			free(answer);
			answer = NULL;
		}
	}
	return answer;
}

int prompt(int sock) {
	printf("Available operations:\n");
	printf("0: check\n");
	printf("1: initialize control topology\n");
	printf("2: execute command\n");
	printf("3: gather file\n");
	printf("4: exit\n");

	char* command = read_line();

	int code = atoi(command);
	free(command);

	switch(code) {
	case 0:
		if(executeCommandCheck(sock) >= 0) {
			char* reply = getResponse(sock);
			if(reply != NULL) {
				printf("Response: %s\n", reply);
				return 0;
			} else {
				printf("An error has occurred\n");
				return 1;
			}
		}
		break;
	case 4:
		if(executeCommandQuit(sock) >= 0) {
			char* reply = getResponse(sock);
			if(reply != NULL)
				printf("Response: %s\n", reply);
		}
		return 1;
		break;
	}

	return 0;
}

int main(int argc, char* argv[]) {

	if(argc != 3) {
		printf("Usage: %s IP PORT\n", argv[0]);
		return 1;
	}

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	bzero(&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	inet_aton(argv[1], &address.sin_addr);
	address.sin_port = htons( atoi(argv[2]) );
	int success = connect(sock, (const struct sockaddr*) &address, sizeof(address));

	if(success == 0) {

		while(prompt(sock) == 0);

		close(sock);
	} else {
		printf("Unable to connect to server.\n");
		return 1;
	}

	return 0;
}
