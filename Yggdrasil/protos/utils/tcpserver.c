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

#include "tcpserver.h"

struct client_t_{
	int id;
	struct client_t_* next;
};

void client_request_init(client_request* client_req){
	client_req->client = NULL;
}

void client_request_addClient(client_request* client_req, int client) {
	client_t* c = malloc(sizeof(client_t));
	c->id =  client;
	c->next = client_req->client;
	client_req->client = c;
}

void client_request_respondToClient(client_request* client_req, void* data, unsigned short datalen){
	LKRequest req;
	req.proto_dest = PROTO_TCP_AGG_SERVER;
	req.request = REPLY;
	req.request_type = TCP_SERVER_RESPONSE;
	req.length = sizeof(int) + datalen;
	void* payload = malloc(sizeof(int)+datalen);
	memcpy(payload+sizeof(int), data, datalen);
	while(client_req->client != NULL){
		memcpy(payload, &client_req->client->id, sizeof(int));
		req.payload = payload;
		deliverReply(&req);

		client_t* c = client_req->client;
		client_req->client = c->next;
		c->next = NULL;
		free(c);
	}
	free(payload);
}

typedef struct socket_list_ {
	int socket;
	struct socket_list_* next;
} socket_list;

static pthread_mutex_t clientsLock;
static socket_list* clients = NULL;
static short protoId;

static void * receiver(void* args) {

	int sock;
	sock = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	bzero(&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons((unsigned short) 8080);
	if(bind(sock, (const struct sockaddr*) &address, sizeof(address)) == 0) {

		if(listen(sock, 10) < 0) {
			perror("Unable to set socket to listen: ");
			exit(2);
		}

		fd_set readfds;

		proto_args* pargs = (proto_args*)args;

		int protoId = pargs->protoID;

		while(1) {

			FD_ZERO(&readfds);
			FD_SET(sock, &readfds);
			int max = sock;
			socket_list* c = clients;
			pthread_mutex_lock(&clientsLock);
			while(c != NULL) {
				FD_SET(c->socket, &readfds);
				if(c->socket > max) max = c->socket;
				c = c->next;
			}
			pthread_mutex_unlock(&clientsLock);

			int s = select(max+1, &readfds, NULL, NULL, NULL);
#ifdef DEBUG
			char ass[2000];
			bzero(ass,2000);
			sprintf(ass, "Got out of select with %d\n",s);
			lk_log("SimpleTCPServer", "ACTIVITY", ass);
#endif
			if(s > 0) {
				if(FD_ISSET(sock, &readfds) != 0) {
#ifdef DEBUG
					lk_log("SimpleTCPServer", "ACTIVITY", "New incoming connection");
#endif
					socket_list* new = malloc(sizeof(socket_list));
					struct sockaddr_in client;
					unsigned int struct_len = sizeof(client);
					new->socket = accept(sock, (struct sockaddr*) &client, &struct_len);
					pthread_mutex_lock(&clientsLock);
					new->next = clients;
					clients = new;
					pthread_mutex_unlock(&clientsLock);
				} else {
#ifdef DEBUG
					lk_log("SimpleTCPServer", "ACTIVITY", "New incoming request");
#endif
					pthread_mutex_lock(&clientsLock);
					c = clients;
					while(c != NULL) {
						if(FD_ISSET(c->socket, &readfds) != 0) {
#ifdef DEBUG
							lk_log("SimpleTCPServer", "ACTIVITY", "Receiving client request");
#endif
							//Process client request
							int request = 0;
							int rb = read(c->socket, &request, sizeof(int));
							if(rb == sizeof(int)) {
								void* data = malloc(request);
								int rb2 = read(c->socket, data, request);

								if(rb2 == request){
									LKEvent lke;
									lke.proto_origin = protoId;
									lke.notification_id = TCP_SERVER_EVENT_REQUEST;
									lke.length = sizeof(int) + rb2;

									lke.payload = malloc(lke.length);

									memcpy(lke.payload, &c->socket, sizeof(int));
									memcpy(lke.payload + sizeof(int), data, rb2);
									deliverEvent(&lke);

									free(lke.payload);

									free(data);
#ifdef DEBUG
									lk_log("SimpleTCPServer", "ACTIVITY", "Delivered event for client");
#endif
								}else{
									lk_log("SimpleTCPServer", "WARNING", "Unparseable request from client");
								}
							} else {
								lk_log("SimpleTCPServer", "WARNING", "Unparseable request from client");
							}
						}
						c = c->next;
					}
					pthread_mutex_unlock(&clientsLock);
				}
			}
		}
	} else {
		lk_log("SimpleTCPServer", "ERROR", "Cannot bind to socket.");
		perror("Error on bind: ");
		exit(2);
	}

	return NULL;
}

void * simpleTCPServer_init(void * args) {

	pthread_mutex_init(&clientsLock, NULL);

	proto_args* pargs = (proto_args*)args;

	protoId = pargs->protoID;

	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;

	pthread_t rec;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_create(&rec, &attr, &receiver, pargs);

	while(1) {
		queue_pop(inBox, &elem);
		if(elem.type == LK_REQUEST) {
#ifdef DEBUG
			char desc[2000];
			bzero(desc, 2000);
			sprintf(desc, "LK_REQUEST Received %s type %d", elem.data.request.request == 1 ? "REPLY" : "REQUEST", elem.data.request.request_type);
			lk_log("SimpleTCPServer", "INFO", desc);
#endif
			if(elem.data.request.request == REPLY && elem.data.request.request_type == TCP_SERVER_RESPONSE) {
				int targetSocket;
				memcpy(&targetSocket, elem.data.request.payload, sizeof(int));
				void* response = elem.data.request.payload + sizeof(int);
				int responseLenght = elem.data.request.length - sizeof(int);

				pthread_mutex_lock(&clientsLock);
				socket_list* c = clients;

				if(c->socket == targetSocket) {
#ifdef DEBUG
					char m[2000];
					memset(m, 0, 2000);
					sprintf(m,"Sending answer (1st of list) length of reply: %d\n", responseLenght);
					lk_log("SimpleTCPServer", "INFO", m);
#endif
					write(c->socket, &responseLenght, sizeof(int));
					if(responseLenght > 0) {
						int missing = responseLenght;
						while(missing > 0)
							missing -= write(c->socket, response+(responseLenght - missing), missing);
					}
					//close(c->socket);
					clients = c->next;
					c->next = NULL;
					free(c);
				} else {

					while(c->next != NULL && c->next->socket != targetSocket);
					if(c->next != NULL) {
#ifdef DEBUG
						char m[2000];
						memset(m, 0, 2000);
						sprintf(m,"Sending answer (middle of list) length of reply: %d\n", responseLenght);
						lk_log("SimpleTCPServer", "INFO", m);
#endif
						write(c->next->socket, &responseLenght, sizeof(int));
						if(responseLenght > 0)
							write(c->next->socket, response, responseLenght);
						//close(c->next->socket);
						socket_list* tmp = c->next;
						c->next = c->next->next;
						tmp->next = NULL;
						free(tmp);
					} else {
						lk_log("SimpleTCPServer", "WARNING", "Failed to find client for sending answer (middle of list)");
					}
				}
				pthread_mutex_unlock(&clientsLock);
			} else {
				lk_log("SimpleTCPServer", "ERROR", "Unexpected type of LK_REQUEST");
			}
			free(elem.data.request.payload);
		} else {
			//ignore
		}
	}
}
