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

#include "discovery.h"


int existNeight(short id) {
	if(id < neighs.known){
		return 1;
	}
	return 0;
}

neigh* getNeighbor(short id) {
	if(existNeight(id) == 1){
		neigh* n = malloc(sizeof(neigh));
		memcpy(n, (neighs.neighbors + id), sizeof(neigh));
		return n;
	}
	return NULL;
}

int getKnownNeighbors() {
	return neighs.known;
}

neigh* getNeighbors() {
	if(neighs.known > 0){
		neigh* n = malloc(sizeof(neigh)*neighs.known);
		memcpy(n, (neighs.neighbors), sizeof(neigh)*neighs.known);
		return n;
	}
	return NULL;
}

short destroyNeigh(short id) {
	if(existNeight(id)){

		memcpy(&(neighs.neighbors + id)->addr, &(neighs.neighbors + neighs.known - 1)->addr, WLAN_ADDR_LEN);
		memcpy(&(neighs.neighbors + id)->uuid, &(neighs.neighbors + neighs.known - 1)->uuid, UUID_SIZE);
		free(neighs.neighbors + neighs.known - 1);

		neighs.known = neighs.known - 1;
		short changedId = neighs.known;
		return changedId; //return the id that was changed
	}
	return -1;
}

neigh* getRandomNeighbor() {
	if(neighs.known == 0)
		return NULL;

	int r = rand() % neighs.known;
	return (neighs.neighbors + r);

}

void processLKRequest(LKRequest* req, short protoID){
	int request = 0;
	int reply = 0;
	if(req->request == REQUEST){
		req->proto_dest = req->proto_origin;
		req->proto_origin = protoID;
		req->request = REPLY;

		switch(req->request_type){
		case DISC_REQ_EXIST_NEIGH:
			if(req->length != sizeof(int) || req->payload == NULL){
				lk_log("DISCOV", "ERROR", "Invalid request format");

			}else{
				memcpy(&request, req->payload, req->length);
				reply = existNeight(request);
				memcpy(req->payload, &reply, sizeof(int));
				req->length = sizeof(int);
				deliverReply(req);
			}
			break;
		case DISC_REQ_GET_N_NEIGHS:
			if(req->length != 0 || req->payload != NULL){
				lk_log("DISCOV", "ERROR", "Invalid request format");

			}else{
				reply = getKnownNeighbors();
				memcpy(req->payload, &reply, sizeof(int));
				req->length = sizeof(int);
				deliverReply(req);
			}
			break;
		case DISC_REQ_GET_NEIGH:
			if(req->length != sizeof(int) || req->payload == NULL){
				lk_log("DISCOV", "ERROR", "Invalid request format");

			}else{
				memcpy(&request, req->payload, req->length);
				neigh* n = getNeighbor(request);
				if(n == NULL)
					memcpy(req->payload, &reply, sizeof(int));
				else{
					reply = 1;
					memcpy(req->payload, &reply, sizeof(int));
					memcpy(req->payload+sizeof(int), n, sizeof(neigh));
				}
				req->length = sizeof(int)+(sizeof(neigh)*reply);
				deliverReply(req);
				free(n);
			}
			break;
		case DISC_REQ_GET_ALL_NEIGHS:
			if(req->length != 0 || req->payload != NULL){
				lk_log("DISCOV", "ERROR", "Invalid request format");
			}else{
				neigh* n = getNeighbors();
				reply = getKnownNeighbors();
				memcpy(req->payload, &reply, sizeof(int));
				if(n != NULL)
					memcpy(req->payload+sizeof(int), n, sizeof(neigh)*reply);
				req->length = sizeof(int)+sizeof(neigh)*reply;
				deliverReply(req);
				free(n);
			}
			break;
		case DISC_REQ_GET_RND_NEIGH:
			if(req->length != 0 || req->payload != NULL){
				lk_log("DISCOV", "ERROR", "Invalid request format");
			}else{
				neigh* n = getRandomNeighbor();
				if(n == NULL)
					memcpy(req->payload, &reply, sizeof(int));
				else{
					reply = 1;
					memcpy(req->payload, &reply, sizeof(int));
					memcpy(req->payload+sizeof(int), n, sizeof(neigh));
				}
				deliverReply(req);
			}
			break;
		default:
			lk_log("DISCOV", "WARNING", "Unknown request type");
			break;
		}
	}
}
