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

#include "aggregation_interface.h"

void aggregation_reportValue(LKRequest* request, short protoId, double value) {
	request->proto_dest = request->proto_origin;
	request->proto_origin = protoId;

	request->request = REPLY;

	//double value
	if(request->payload != NULL){
		request->payload = realloc(request->payload, request->length + sizeof(int) + sizeof(double));
	}else{
		request->payload = malloc(sizeof(int) + sizeof(double));
	}


	void* payload = request->payload + request->length;

	request->length += sizeof(double);
	memcpy(payload, &value, sizeof(double));

	int r = deliverReply(request);

	if(r == SUCCESS)
		lk_log("AGG", "ALIVE", "delivered reply success");
	else
		lk_log("AGG", "ALIVE", "delivered reply failed");

	free(request->payload);
}


double aggregation_getValue(LKRequest* reply) {

	double value;

	if(reply->length != sizeof(double))
		return -99999;

	memcpy(&value, reply->payload, sizeof(double));

	return value;

}
