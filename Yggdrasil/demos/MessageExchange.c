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

#include <string.h>
#include <stdlib.h>

#include "core/lk_runtime.h"
#include "core/utils/utils.h"
#include "core/proto_data_struct.h"
#include "core/utils/queue.h"
#include "core/utils/queue_elem.h"

#include "lightkone.h"

int main(int argc, char* argv[]){

	char dest_addr[6];
	if(argc == 2){
		if(str2wlan(dest_addr, argv[1]) == -1){
			printf("Wrong input format, use a valid mac address example:\nUsage: ff:ff:ff:ff:ff:ff\n");
			exit(1);
		}
	}else{
		printf("Program takes 1 input, example:\nUsage: ff:ff:ff:ff:ff:ff\n");
		exit(1);
	}

	//define a network configuration
	//use adhoc mode, default frequency (2442), scan the network 1 time to find and connect to a network called "ledge"
	//setup the LKM_filter in the kernel so that only receive messages tagged with LKP
	NetworkConfig* ntconf = defineNetworkConfig("AdHoc", DEFAULT_FREQ, 1, 0, "ledge", LKM_filter);


	queue_t* inBox; //from where the framework will communicate to the application
	//the application will receive messages, timers, events and requests/replies from the inBox

	//setup the runtime to use 0 lk protocols, 0 user defined protocols and 1 app
	//the runtime will always setup 2 lk protocols, the distcher (to send and receive messages to and from the network)
	//and the timer (to setup timers within the framework)
	lk_runtime_init(ntconf, 0, 0, 1);


	//register the application in the runtime
	//the runtime will initialize the inBox and return the application identifier within the framework so that protocols can communicate with it
	short id = registerApp(&inBox, 0);

	//start the protocols
	//the runtime will lauch a thread per registered protocol (except applications, those are handle by the programmer)
	lk_runtime_start();


	int t = 2000000; //2 seconds
	struct timespec nextoperation;
	gettimeofday((struct timeval *) &nextoperation,NULL);
	nextoperation.tv_sec += t / 1000000;
	nextoperation.tv_nsec += (t % 1000000) * 1000;

	int sequence_number = 0;

	while(1){
		queue_t_elem elem;

		//try to get something from the queue until some time
		//the queue_t is a blocking priority queue
		int obtained = queue_try_timed_pop(inBox, &nextoperation , &elem);

		if(obtained == 1){ //there was something in the queue
			//an element can be of 4 types
			if(elem.type == LK_MESSAGE){
				//LK_MESSAGE (a message from the network)
				//in this example there are only LK_MESSAGES, so we will ignore the other types for now
				//process the message
				//if the message was delivered to here, it was probably sent by the same protocol/app in another device
				LKMessage msg = elem.data.msg; //the message received from the inBox

				//write a log with the contents of the message and from who it was from
				char m[2000];
				memset(m, 0, 2000);
				char addr[33]; //a mac address as 33 characters
				memset(addr, 0, 33);
				wlan2asc(&msg.srcAddr, addr); //auxiliary function that translates the mac address from machine to human readable form
				sprintf(m, "Message from %s content: %s", addr, msg.data);
				lk_log("ONE HOP BCAST", "RECEIVED MESSAGE", m);

				LKMessage_init(&msg, msg.srcAddr.data, id); //init a LKMessage set with destination

				memset(m, 0, 2000);
				sprintf(m, "Message Reply to %s", msg.data); //our message payload
				LKMessage_addPayload(&msg, m, strlen(m)+1); //add the payload to the message (+1 to include string terminator)

				dispatch(&msg); //send the message to the network

			} else if(elem.type == LK_TIMER) {
				//LK_TIMER (a timer that was fired)
			} else if(elem.type == LK_EVENT) {
				//LK_EVENT (an event fired by some other protocol)
			} else if(elem.type == LK_REQUEST){
				//LK_REQUEST (a request or a reply sent by another protocol)
			}

		} else { //there was nothing in the queue to process
			//create a message and send it in one hop broadcast
			LKMessage msg;
			LKMessage_init(&msg, (unsigned char*) dest_addr, id); //init a LKMessage set with destination

			char m[2000];
			memset(m, 0, 2000);
			sprintf(m, "Message with sequence number %d", sequence_number); //our message payload
			LKMessage_addPayload(&msg, m, strlen(m)+1); //add the payload to the message (+1 to include string terminator)

			sequence_number ++;

			dispatch(&msg); //send the message to the network
		}

		gettimeofday((struct timeval *) &nextoperation,NULL);
		nextoperation.tv_sec += t / 1000000;
		nextoperation.tv_nsec += (t % 1000000) * 1000;
	}

	return 0;
}
