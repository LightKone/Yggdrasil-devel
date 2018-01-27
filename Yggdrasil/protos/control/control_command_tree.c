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

#include "protos/control/control_command_tree.h"

typedef struct peer_connection_ {
	int socket;
	char ip_addr[16];
	WLANAddr mac_addr;
	struct peer_connection_* next;
} peer_connection;

static int listen_socket;
static peer_connection* peers;

static int local_pipe[2];

static pthread_t queueMonitor;

void* queue_monitor_thread(void* arg) {
	proto_args* pargs = arg;
	queue_t* inBox = pargs->inBox;
	short protoID = pargs->protoID;

	queue_t_elem elem;

	while(1) {
		queue_pop(inBox, &elem);

		write(local_pipe[1], &elem, sizeof(queue_t_elem));
	}
}

void shutdown_protocol() {
	while(peers != NULL) {
			peer_connection* p = peers;
			peers = p->next;
			p->next = NULL;
			close(p->socket);
			free(p);
		}

		close(local_pipe[0]);
		close(local_pipe[1]);
		pthread_cancel(queueMonitor);

		close(listen_socket);

		pthread_exit(NULL);
}

void handle_new_connection() {

	peer_connection* newPeer = malloc(sizeof(peer_connection));
	struct sockaddr_in address;
	unsigned int length = sizeof(address);
	newPeer->socket = accept(listen_socket, (struct sockaddr *) &address, &length);
	if(newPeer->socket != -1) {
		inet_ntop(AF_INET, &address.sin_addr, newPeer->ip_addr, 16);
		int toRead = WLAN_ADDR_LEN + 16;
		char buffer[toRead];
		while(toRead > 0) {
			int r = read(newPeer->socket, buffer + (WLAN_ADDR_LEN + 16) - toRead, toRead);
			if(r >= 0) {
				toRead -=r;
			} else {
#ifdef DEBUG
				perror("CONTROL COMMAND TREE Protocol - Error reading hello message from new socket: ");
#endif
				close(newPeer->socket);
				free(newPeer);
				return;
			}
		}
		memcpy(newPeer->mac_addr.data, buffer, WLAN_ADDR_LEN);
		memcpy(newPeer->ip_addr, buffer+WLAN_ADDR_LEN,16);
	}
#ifdef DEBUG
	else
	{
		perror("CONTROL COMMAND TREE Protocol - Error accepting connection: ");
	}
#endif

}

void handle_queue_request() {
	queue_t_elem elem;
	read(local_pipe[0], &elem, sizeof(queue_t_elem));
	if(elem.type == LK_MESSAGE) {

	} else if(elem.type == LK_TIMER) {

	} else if(elem.type == LK_REQUEST) {

	} else if(elem.type == LK_EVENT) {

	}
#ifdef DEBUG
	else {
		printf("CONTROL COMMAND TREE Protocol - Received unknown element type through queue: %d\n", elem.type);
	}
#endif
}

void	 handle_peer_communication(peer_connection* p) {

}

void handle_listen_exception() {
	printf("CONTROL COMMAND TREE Protocol - Listen socket has failed, stopping everything.\n");
	shutdown_protocol();
}

void handle_queue_exception() {
	printf("CONTROL COMMAND TREE Protocol - Pipe has failed, unable to contact secondary thread, stopping everything.\n");
	shutdown_protocol();
}

void handle_peer_exception(peer_connection* p) {
//#ifdef DEBUG
	printf("CONTROL COMMAND TREE Protocol - Exception in connection with peer %s.\n", p->ip_addr);
//#endif DEBUG
	close(p->socket);
	peer_connection** pp = &peers;
	while(*pp != p) {
		pp = &((*pp)->next);
	}
	*pp = p->next;
	p->next = NULL;
	free(p);
}

void* control_command_tree_init(void* arg) { //argument is the time for the announcement

	peers = NULL;

	proto_args* pargs = arg;
	queue_t* inBox = pargs->inBox;
	short protoID = pargs->protoID;
	short controlDiscoveryProtoID = (unsigned short) ((long) pargs->protoAttr);

	LKRequest req;
	req.length = 0;
	req.payload = NULL;
	req.proto_dest = controlDiscoveryProtoID;
	req.proto_origin = protoID;
	req.request = REQUEST;
	req.request_type = CONTROL_DISC_GET_NEIGHBORS;
	deliverRequest(&req);

	queue_t_elem elem;

	queue_pop(inBox, &elem);

	if( pipe(local_pipe) != 0 ) {
		perror("CONTROL COMMAND TREE Protocol - Unable to setup internal pipe: ");
		return NULL;
	}
/*
	if( pipe2(local_pipe, O_DIRECT) != 0 ) {
		perror("CONTROL COMMAND TREE Protocol - Unable to setup internal pipe: ");
		return NULL;
	}
*/
	pthread_create(&queueMonitor, NULL, queue_monitor_thread, arg);

	listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	bzero(&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons((unsigned short) 6666);

	if(bind(listen_socket, (const struct sockaddr*) &address, sizeof(address)) == 0 ) {

		if(listen(listen_socket, 20) < 0) {
			perror("CONTROL COMMAND TREE Protocol - Unable to setup listen on socket: ");
			pthread_cancel(queueMonitor);
			return NULL;
		}

		fd_set readfds;
		fd_set exceptionfds;

		int largest_fd;

		while(1) {
			FD_ZERO(&readfds);
			FD_ZERO(&exceptionfds);

			FD_SET(listen_socket, &readfds);
			FD_SET(listen_socket, &exceptionfds);
			largest_fd = listen_socket;

			FD_SET(local_pipe[0], &readfds);
			FD_SET(local_pipe[0], &exceptionfds);
			if(largest_fd < local_pipe[0]) largest_fd = local_pipe[0];

			peer_connection* p = peers;

			while(p != NULL) {
				FD_SET(p->socket, &readfds);
				FD_SET(p->socket, &exceptionfds);
				if(largest_fd < p->socket) largest_fd = p->socket;
				p = p->next;
			}

			int s = select(largest_fd+1, &readfds, NULL, &exceptionfds, NULL);

			if(s > 0) {

				//Handle read operations
				if(FD_ISSET(listen_socket, &readfds) != 0) {
					handle_new_connection();
				}
				if(FD_ISSET(local_pipe[0], &readfds) != 0) {
					handle_queue_request();
				}
				p = peers;
				while(p != NULL) {
					if(FD_ISSET(p->socket, &readfds) != 0) {
						handle_peer_communication(p);
					}
					p = p->next;
				}

				//Handle exceptions
				if(FD_ISSET(listen_socket, &exceptionfds) != 0) {
					handle_listen_exception();
				}
				if(FD_ISSET(local_pipe[0], &exceptionfds) != 0) {
					handle_queue_exception();
				}
				p = peers;
				while(p != NULL) {
					if(FD_ISSET(p->socket, &exceptionfds) != 0) {
						handle_peer_exception(p);
					}
					p = p->next;
				}
			}

		}

	} else {
		perror("CONTROL COMMAND TREE Protocol - Unable to setup listen socket: ");
		pthread_cancel(queueMonitor);
		return NULL;
	}

	return NULL;
}
