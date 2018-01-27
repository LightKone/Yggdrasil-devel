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

#ifndef CORE_RUNTIME_H_
#define CORE_RUNTIME_H_

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "core/utils/queue.h"
#include "core/utils/utils.h"
#include "lightkone.h"
#include "core/protos/dispatcher.h"
#include "core/protos/timer.h"
#include "core/proto_data_struct.h"

#define DEFAULT_QUEUE_SIZE 10

#define SIG SIGQUIT

/*********************************************************
 * Lightkone Protocol IDs
 *********************************************************/
typedef enum _PROTOID {
	PROTO_DISPATCH = 0,
	PROTO_TIMER = 1,
	PROTO_DISCOV = 2,
	PROTO_FAULT_DECT = 3,
	PROTO_SIMPLE_AGG = 4,
	PROTO_TCP_AGG_SERVER = 5,
	PROTO_TOPOLOGY_MANAGER = 6
} PROTOID;

#define PROTO_0 100 //begin ID for user defined protocols
#define APP_0 400 //begin ID for apps

/*********************************************************
 * The rest
 *********************************************************/

//Arguments for a protocol to launch
typedef struct _proto_args {
	queue_t* inBox; //the respective protocol queue
	unsigned short protoID; //the respective protocol ID
	uuid_t myuuid; //the uuid of the process
	void* protoAttr; //protocol specific arguments
}proto_args;

/*********************************************************
 * Init functions
 *********************************************************/

/**
 * Initialize the runtime to hold the protocols the applications need to run
 * @param ntconf The configuration of the environment
 * @param n_lk_protos The number of lightkone protocols, not counting the dispatcher and the timer
 * @param n_protos The number of user defined protocols
 * @param n_apps The number of Apps
 * @return SUCCESS if successful; FAILED otherwise
 */
int lk_runtime_init(NetworkConfig* ntconf, int n_lk_protos, int n_protos, int n_apps);

/**
 * Register a lightkone protocol in the runtime
 * @param protoID The lightkone protocol ID
 * @param protoFunc The lightkone protocol main function
 * @param protoAttr The lightkone protocol specific parameters
 * @param max_event_type The number of events the lightkone protocol has
 * @return SUCCESS if all checks out and if there is still space for lightkone protocols, FAILED otherwise
 */
int addLKProtocol(short protoID, void * (*protoFunc)(void*), void* protoAttr, int max_event_type);

/**
 * Register a user defined protocol in the runtime
 * @param protoID The user defined protocol ID
 * @param protoFunc The user defined protocol main function
 * @param protoAttr The user defined protocol specific parameters
 * @param max_event_type The number of events the user defined protocol has
 * @return SUCCESS if all checks out and if there is still space for user define protocols, FAILED otherwise
 */
int addProtocol(short protoID, void * (*protoFunc)(void*), void* protoAttr, int max_event_type);

/**
 * Register an application in the runtime
 * @param app_inBox The application queue
 * @param max_event_type The number of events the application has
 * @return The application ID
 */
short registerApp(queue_t** app_inBox, int max_event_type);

/*********************************************************
 * Config
 *********************************************************/

/**
 * Update the protocol specific parameters
 * @param protoID The protocol ID
 * @param protoAttr The protocol specific parameters to be updated
 * @return SUCCESS if the protocol was registered, FAILED otherwise
 */
int updateProtoAttr(short protoID, void* protoAttr);

/**
 * Change the queue reference of the protocol identified by protoID with the queue reference identified by myID
 * @param protoID The protocol of which the queue should be changed
 * @param myID The protocol to which the queue will be changed
 * @return The queue reference of protoID
 */
queue_t* interceptProtocolQueue(short protoID, short myID);

/**
 * Regist the events to which a protocol is interested
 * @param protoID The protocol ID who has the events
 * @param events A list of interested event IDs
 * @param nEvents The number of interested events
 * @param myID The protocol which is interested in the events
 * @return This operation always succeeds, if the events do not exist it will simply not register the interest
 */
int registInterestEvents(short protoID, short* events, int nEvents, short myID);

/*********************************************************
 * Start
 *********************************************************/

/**
 * Start all of the registered protocols
 * @return This operations always succeeds, if there are errors they will be logged for later analysis
 */
int lk_runtime_start();

/*********************************************************
 * Runtime Functions
 *********************************************************/

/**
 * Send a message to the network
 * @param msg The message to be sent
 * @return This operation always succeeds
 */
int dispatch(LKMessage* msg); //dispatch a message to the network

/**
 * Setup a timer in timer protocol
 * @param timer The timer to be set
 * @return This operation always succeeds
 */
int setupTimer(LKTimer* timer);

/**
 * Cancel an existing timer.
 * @param timer The timer to be cancelled
 * @return This operation always succeeds
 */
int cancelTimer(LKTimer* timer);

/**
 * Deliver a message to the protocol identified in the message
 * @param msg The message to be delivered
 * @return This operation always succeeds
 */
int deliver(LKMessage* msg);

/**
 * Verifies if the message destination is the process by verifying the destination's mac address,
 * If the mac address belongs to the process or it is the broadcast address, the message is delivered
 * to the protocol identified in the message
 * @param msg The message to be delivered
 * @return This operation always succeeds
 */
int filterAndDeliver(LKMessage* msg);

/**
 * Deliver a timer to the protocol identified in the timer
 * @param timer The timer to be delivered
 * @return This operation always succeeds
 */
int deliverTimer(LKTimer* timer);

/**
 * Deliver an event to all interested protocols in the event
 * @param event The event to be delivered
 * @return This operation always succeeds
 */
int deliverEvent(LKEvent* event);

/**
 * Deliver a request to a protocol
 * @param req The request to be delivered
 * @return If the LKRequest is a request returns SUCCESS, if not returns FAILED
 */
int deliverRequest(LKRequest* req);

/**
 * Deliver a reply to a protocol
 * @param res The reply to be delivered
 * @return If the LKRequest is a request returns SUCCESS, if not returns FAILED
 */
int deliverReply(LKRequest* res);

/*********************************************************
 * Other useful read Functions
 *********************************************************/
/**
 * Gets the mac address of the process
 * @param s2 A char array to where the mac address is copied to
 * @return A char array with the mac address
 */
char* getMyAddr(char* s2);

/**
 * Fill the parameter addr with the mac address of the process
 * @param addr A pointer to the WLANAddr that was requested
 */
void setMyAddr(WLANAddr* addr);

double getValue();

const char* getChannelIpAddress();

#endif /* CORE_RUNTIME_H_ */
