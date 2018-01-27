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

#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "lightkone.h"
#include "core/lk_runtime.h"
#include "core/utils/utils.h"
#include "core/proto_data_struct.h"

typedef struct client_t_ client_t;

typedef struct client_request_ client_request;

struct client_request_{
	client_t* client;
};

void client_request_init(client_request* client_req);
void client_request_addClient(client_request* client_req, int client);
void client_request_respondToClient(client_request* client_req, void* data, unsigned short datalen);

typedef enum tcp_server_ops_ {
	TCP_SERVER_EVENT_REQUEST,
	TCP_SERVER_EVENT_MAX
} tcp_server_ops;

typedef enum tcp_server_req_ {
	TCP_SERVER_RESPONSE
}tcp_server_req;

void * simpleTCPServer_init(void * args);

#endif /* TCPSERVER_H_ */
