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

#include "topologyManager.h"

typedef struct WLANAddr_db_ {
	short block;
	WLANAddr addr;
}WLANAddr_db;

static WLANAddr_db db[24]; //The number of nodes you have in the topology control mac address database file

static short protoId;

void * topologyManager_init(void * args) {

	proto_args* pargs = (proto_args*)args;

	protoId = pargs->protoID;
	queue_t* inBox = pargs->inBox;

	FILE* f_db = NULL;
	f_db = fopen("/etc/lightkone/topologyControl/macAddrDB.txt","r");

	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	if(f_db > 0 && f_db != NULL){

		//01 - b8:27:eb:3a:96:0e (line format)
		while ((read = getline(&line, &len, f_db)) != -1) {
			//printf("Retrieved line of length %zu :\n", read);
			//printf("%s", line);

			char index[3];
			bzero(index, 3);
			memcpy(index, line, 2);
			//printf("%d\n", atoi(index));
			char* mac = line + 5;
			//printf("%s", mac);
			if(str2wlan((char*) db[atoi(index) - 1].addr.data, mac) != 0){
				lk_log("TOPOLOGY MANAGER", "ERROR", "Error retrieving wlan address");
			}
			db[atoi(index) - 1].block = 1;
		}

		free(line);
		line = NULL;
		fclose(f_db);
		f_db = NULL;
	}else{
		perror("fopen failed");
		lk_log("TOPOLOGY MANAGER", "ERROR", "Unable to open mac address database file at /etc/lightkone/topologyControl/macAddrDB.txt");
		lk_logflush();
		exit(2);
	}
#ifdef DEBUG
	lk_log("TOPOLOGY MANAGER", "ALIVE", "Populated mac address database");
#endif
	f_db = fopen("/etc/lightkone/topologyControl/neighs.txt","r");

	if(f_db > 0 && f_db != NULL){

		//01 (line format)
		while ((read = getline(&line, &len, f_db)) != -1) {
//			printf("Retrieved line of length %zu :\n", read);
//			printf("%s", line);

			db[atoi(line) - 1].block = 0;

		}

		free(line);
		line = NULL;
		fclose(f_db);
		f_db = NULL;
	}else{
		//perror("fopen failed");
		//lk_log("TOPOLOGY MANAGER", "ERROR", "Unable to open mac address database file at /etc/lightkone/topologyControl/neighs.txt");
		//lk_logflush();
		//exit(2);

		for(int i = 0; i < 24; i++){
			db[i].block = 0;
		}
	}

	LKRequest req;
	req.proto_origin = protoId;
	req.proto_dest = PROTO_DISPATCH;
	req.request = REQUEST;
	req.request_type = DISPATCH_IGNORE_REG;

	int i;
	for(i = 0; i < 24; i++) {
		if(db[i].block == 1){
			//TODO optimize to send only one request
			dispatcher_serializeIgReq(IGNORE, db[i].addr, &req);
			deliverRequest(&req);
			free(req.payload);
		}

	}



	while(1){
		queue_t_elem elem;
		queue_pop(inBox, &elem);
		//TODO support topology changes

		if(elem.type == LK_REQUEST){
			if(elem.data.request.request == REQUEST && elem.data.request.request_type == CHANGE_LINK){
				int tochange;
				memcpy(&tochange, elem.data.request.payload, sizeof(int));
#ifdef DEBUG
				char s[500];
				bzero(s, 500);
				sprintf(s, "request to change %d", tochange);
				lk_log("TOPOLOGY MANAGER", "REQUEST", s);
#endif
				tochange -= 1;
				if(tochange < 24 && tochange >= 0){
					if(db[tochange].block == 1){
						db[tochange].block = 0;

						dispatcher_serializeIgReq(NOT_IGNORE, db[tochange].addr, &req);
						deliverRequest(&req);
					}else{
						db[tochange].block = 1;

						dispatcher_serializeIgReq(IGNORE, db[tochange].addr, &req);
						deliverRequest(&req);
					}

				}
			}

			if(elem.data.request.payload != NULL){
				free(elem.data.request.payload);
				elem.data.request.payload = NULL;
			}
		}
	}

}


