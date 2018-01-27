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

#include "lightkone.h"
#include "core/utils/queue.h"
#include "core/lk_runtime.h"
#include "protos/discovery/discoveryFullFT.h"
#include "protos/faultdetection/faultdetector.h"
#include "core/utils/utils.h"

#include <stdlib.h>


int main(int argc, char* argv[]) {
	queue_t* myqueue = queue_init(0, 10);

	queue_t_elem writeMSG;
	queue_t_elem readMSG;

	int counter = 1;
	int i;

	printf("TESTE SEM RESIZE\n");

	for(i = 0; i < 10; i++) {
		writeMSG.type = LK_MESSAGE;
		sprintf(writeMSG.data.msg.data, "Message number %d", counter);
		counter++;
		queue_push(myqueue, &writeMSG);
	}

	for(i = 0; i < 10; i++) {
		queue_pop(myqueue, &readMSG);
		printf("MSG: %s\n",readMSG.data.msg.data);
		memset(readMSG.data.msg.data,0,MAX_PAYLOAD);
	}

	for(i = 0; i < 5; i++) {
		writeMSG.type = LK_MESSAGE;
		sprintf(writeMSG.data.msg.data, "Message number %d", counter);
		counter++;
		queue_push(myqueue, &writeMSG);
	}

	for(i = 0; i < 5; i++) {
		queue_pop(myqueue, &readMSG);
		printf("MSG: %s\n",readMSG.data.msg.data);
		memset(readMSG.data.msg.data,0,MAX_PAYLOAD);
	}

	printf("TESTE COM RESIZE NO MEIO\n");

	for(i = 0; i < 15; i++) {
		writeMSG.type = LK_MESSAGE;
		sprintf(writeMSG.data.msg.data, "Message number %d", counter);
		counter++;
		queue_push(myqueue, &writeMSG);
	}

	for(i = 0; i < 15; i++) {
		queue_pop(myqueue, &readMSG);
		printf("MSG: %s\n",readMSG.data.msg.data);
		memset(readMSG.data.msg.data,0,MAX_PAYLOAD);
	}

	printf("TESTE COM RESIZE ALINHADO NO FINAL\n");

	myqueue = queue_init(0, 10);

	for(i = 0; i < 10; i++) {
		writeMSG.type = LK_MESSAGE;
		sprintf(writeMSG.data.msg.data, "Message number %d", counter);
		counter++;
		queue_push(myqueue, &writeMSG);
	}

	for(i = 0; i < 10; i++) {
		queue_pop(myqueue, &readMSG);
		printf("MSG: %s\n",readMSG.data.msg.data);
		memset(readMSG.data.msg.data,0,MAX_PAYLOAD);
	}

	for(i = 0; i < 15; i++) {
		writeMSG.type = LK_MESSAGE;
		sprintf(writeMSG.data.msg.data, "Message number %d", counter);
		counter++;
		queue_push(myqueue, &writeMSG);
	}

	for(i = 0; i < 15; i++) {
		queue_pop(myqueue, &readMSG);
		printf("MSG: %s\n",readMSG.data.msg.data);
		memset(readMSG.data.msg.data,0,MAX_PAYLOAD);
	}

	printf("TESTE COM RESIZE ALINHADO NO INICIO\n");

	myqueue = queue_init(0, 10);

	for(i = 0; i < 20; i++) {
		writeMSG.type = LK_MESSAGE;
		sprintf(writeMSG.data.msg.data, "Message number %d", counter);
		counter++;
		queue_push(myqueue, &writeMSG);
	}

	for(i = 0; i < 20; i++) {
		queue_pop(myqueue, &readMSG);
		printf("MSG: %s\n",readMSG.data.msg.data);
		memset(readMSG.data.msg.data,0,MAX_PAYLOAD);
	}

	printf("TESTE COM DOUBLE RESIZE NO MEIO\n");

	for(i = 0; i < 15; i++) {
		writeMSG.type = LK_MESSAGE;
		sprintf(writeMSG.data.msg.data, "Message number %d", counter);
		counter++;
		queue_push(myqueue, &writeMSG);
	}

	for(i = 0; i < 5; i++) {
		queue_pop(myqueue, &readMSG);
		printf("MSG: %s\n",readMSG.data.msg.data);
		memset(readMSG.data.msg.data,0,MAX_PAYLOAD);
	}

	for(i = 0; i < 30; i++) {
		writeMSG.type = LK_MESSAGE;
		sprintf(writeMSG.data.msg.data, "Message number %d", counter);
		counter++;
		queue_push(myqueue, &writeMSG);
	}
	for(i = 0; i < 40; i++) {
		queue_pop(myqueue, &readMSG);
		printf("MSG: %s\n",readMSG.data.msg.data);
		memset(readMSG.data.msg.data,0,MAX_PAYLOAD);
	}

	printf("TESTE COM QUEUES DE REQUESTS\n");

	myqueue = queue_init(0, 10);

	memset(&writeMSG, 0, sizeof(queue_t_elem));

	for(i = 0; i < 15; i++) {
		writeMSG.type = LK_REQUEST;
		writeMSG.data.request.length = (i+1) * 100;
		writeMSG.data.request.payload = malloc(writeMSG.data.request.length);
		memset(writeMSG.data.request.payload, i, writeMSG.data.request.length);
		queue_push(myqueue, &writeMSG);
		printf("Wrote request with %d length filled with %c\n", writeMSG.data.request.length, ((char*)writeMSG.data.request.payload)[0]+48);
		free(writeMSG.data.request.payload);
	}

	for(i = 0; i < 15; i++) {
		memset(&readMSG, 0, sizeof(queue_t_elem));
		queue_pop(myqueue, &readMSG);
		printf("Read request with %d length filled with %c\n", readMSG.data.request.length, ((char*)readMSG.data.request.payload)[0]+48);
		free(readMSG.data.request.payload);
	}

	return 0;
}

