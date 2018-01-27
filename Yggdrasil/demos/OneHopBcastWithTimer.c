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

int main(int argc, char* agrv[]){

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

	//In this example we will set a timer to fire every 2 seconds and send a message
	LKTimer timer;
	LKTimer_init(&timer, id, id); //this will generate an uuid (unique identifier) for the timer and set the timer destination to the app
	LKTimer_set(&timer, 2000000, 2000000); //set the timer to go off in 2 seconds and repeat for every 2 seconds

	setupTimer(&timer); //setup the timer in the runtime, when the timer expires it will be delivered to the destination (in this case the app)

	int sequence_number = 0;

	while(1){
		queue_t_elem elem;

		//try to get something from the queue
		//the queue_t is a blocking priority queue
		queue_pop(inBox, &elem);


		//an element can be of 4 types
		if(elem.type == LK_MESSAGE){
			//LK_MESSAGE (a message from the network)
			//if the message was delivered to here, it was probably sent by the same protocol/app in another device
			LKMessage msg = elem.data.msg; //the message received from the inBox

			//write a log with the contents of the message and from who it was from
			char m[200];
			memset(m, 0, 200);
			char addr[33]; //a mac address as 33 characters
			memset(addr, 0, 33);
			wlan2asc(&msg.srcAddr, addr); //auxiliary function that translates the mac address from machine to human readable form
			sprintf(m, "Message from %s content: %s", addr, msg.data);
			lk_log("ONE HOP BCAST", "RECEIVED MESSAGE", m);

		} else if(elem.type == LK_TIMER) {
			//LK_TIMER (a timer that was fired)
			LKTimer recv_timer = elem.data.timer;

			//a protocol can have many timers requested, to distinguish the timers we use the uuid
			if(uuid_compare(timer.id, recv_timer.id) == 1){ //compare the uuid of the timer received with the one stored
				//create a message and send it in one hop broadcast
				LKMessage msg;
				LKMessage_initBcast(&msg, id); //init a LKMessage set with destination to BroadCast

				char m[200];
				memset(m, 0, 200);
				sprintf(m, "Message with sequence number %d", sequence_number); //our message payload
				LKMessage_addPayload(&msg, m, strlen(m)+1); //add the payload to the message (+1 to include string terminator)

				sequence_number ++;

				dispatch(&msg); //send the message to the network

				//for the purpuse of this demostration we will cancel the timer and reset it
				cancelTimer(&timer); //cancel the timer in the runtime (the timer will not fire)

				LKTimer_set(&timer, 3000000, 1000000); //reset the time in the timer (cancel will zero the fields of the LKTimer related to the the time)
				setupTimer(&timer); //set it up again in the runtime

			}
		} else if(elem.type == LK_EVENT) {
			//LK_EVENT (an event fired by some other protocol)
		} else if(elem.type == LK_REQUEST){
			//LK_REQUEST (a request or a reply sent by another protocol)
		}

	}

	return 0;
}


