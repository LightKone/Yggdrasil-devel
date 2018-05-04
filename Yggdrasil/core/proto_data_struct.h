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

#ifndef PROTO_DATA_STRUCT_H_
#define PROTO_DATA_STRUCT_H_

#include <uuid/uuid.h>
#include "lightkone.h"

#define LK_MESSAGE_PAYLOAD MAX_PAYLOAD -sizeof(short) -sizeof(unsigned int) //LKProto will be serialized into phy message payload (message id)

// Lightkone message
typedef struct _LKMessage{
	//Mac destination;
	WLANAddr destAddr;
	//Mac source; (will be filled by dispatcher)
	WLANAddr srcAddr;
	//Lightkone Protocol id;
	unsigned short LKProto;
	//PayloadLen
	unsigned short dataLen;
	//Payload
	char data[LK_MESSAGE_PAYLOAD];
} LKMessage;

typedef struct timer_config_ {
	time_t first_notification; //time(NULL) + in how many seconds should the timer go off
	unsigned long repeat_interval_ms; //the repeat interval (if the timer is to be periodic)
}timer_config;

typedef struct lk_timer_ {
	uuid_t id; //the timer unique identifier
	unsigned short proto_origin; //the protocol that requested the timer
	unsigned short proto_dest; //the protocol to which the timer is to be delivered
	unsigned short timer_type; //type of timer (can be used to multiplex timers by protocols)
	timer_config config; //the timer configuration
	unsigned short length; //The length (in bytes) of the payload associated with this timer.
	void* payload; //The payload associated with a timer, a protocol that receives a timer must do free of this.
	//The payload should be set to null if no payload is associated and the length should be set to zero in this case.
}LKTimer;

typedef struct lk_event_ {
	unsigned short proto_origin; //the protocol who created the event
	unsigned short notification_id; //the event ID
	unsigned short length; //the length of the payload
	void* payload; //The payload associated with a timer, a protocol that receives a timer must do free of this.
	//The payload should be set to null if no payload is associated and the length should be set to zero in this case.
}LKEvent;

typedef enum request_type_ {
	REQUEST = 0,
	REPLY = 1
} request_type;

typedef struct lk_request_ {
	unsigned short proto_origin; //the protocol who created the event
	unsigned short proto_dest; //the protocol who will receive the event
	request_type request; //0 = REQUEST; 1 = REPLY;
	unsigned short request_type; //the request type (according to the handler of the request)
	unsigned short length;
	void* payload;
}LKRequest;

#include "core/utils/utils.h"

/**
 * Initialize a LKMessage with the broadcast address
 * @param msg the message
 * @param protoID the protocol ID that requested the initializaion
 */
void LKMessage_initBcast(LKMessage* msg, short protoID);

/**
 * Initialize a LKMessage with the destination given in addr
 * @param msg the message
 * @param addr the address to be sent to
 * @param protoID the protocol ID that requested the initialization
 */
void LKMessage_init(LKMessage* msg, unsigned char addr[6], short protoID);

/**
 * Adds the contents in payload to the message payload
 * @param msg the message
 * @param payload the payload to be added
 * @param payloadLen the payload length
 * @return SUCESS if there was still space in the message payload to add the payload, FAILED otherwise
 */
int LKMessage_addPayload(LKMessage* msg, char* payload, unsigned short payloadLen);

/**
 * Initialize a timer with the proto origin and proto destination
 * @param timer the timer
 * @param protoOrigin the protocol who requested the timer
 * @param protoDest the protocol to whom it will be delivered
 */
void LKTimer_init(LKTimer* timer, short protoOrigin, short protoDest);

/**
 * Initialize a timer with the proto origin and proto destination
 * @param timer the timer
 * @param uuid the timers uuid
 * @param protoOrigin the protocol who requested the timer
 * @param protoDest the protocol to whom it will be delivered
 */
void LKTimer_init_with_uuid(LKTimer* timer, uuid_t uuid, short protoOrigin, short protoDest);

/**
 * Set a timer to fire in firstNotification microseconds, and the repeat interval in microseconds
 * @param timer the timer
 * @param firstNotification_ms the time in microseconds until the timer fires for the first time
 * @param repeat_ms the repeat interval in microseconds or 0 if no repeat is neaded
 */
void LKTimer_set(LKTimer* timer, unsigned long firstNotification_us, unsigned long repeat_us);


/**
 * Sets the type of timer on the timer structure, to distinguish timers in a procotol
 * @param timer timer structure to be set
 * @param type codification of the type of timer
 */
void LKTimer_setType(LKTimer* timer, short type);

/**
 * Sets the payload of a timer to be exactly the given one.
 * This fucntion first frees the previous payload if existed, and mallocs the new one.
 * @param timer the timer structure to be set
 * @param payload the payload to be set
 * @param payloadLen the payload's lenght
 */
void LKTimer_setPayload(LKTimer* timer, void* payload, unsigned short payloadLen);

/**
 * Adds the payload to the existing payload of the timer
 * This functions reallocs the paylaod of the timer in case more space is needed.
 * @param timer the timer structure
 * @param payload the paylaod to be added
 * @param payloadLen the payload's lenght
 */
void LKTimer_addPayload(LKTimer* timer, void* payload, unsigned short payloadLen);

/**
 * Frees the payload of the timer
 * @param timer timer structure that will have its paylaod freed
 */
void LKTimer_freePayload(LKTimer* timer);

/**
 * Initializes an event structure with the paramenters given
 * The payload is set to NULL
 * @param ev the event structure to be initialized
 * @param protoOrigin the protocol generating the event
 * @param notification_id the notification code that identifies the event
 */
void LKEvent_init(LKEvent* ev, short protoOrigin, short notification_id);

/**
 * Adds payload to the existing payload of the event structure
 * This functions reallocs the payload if more space is needed
 * @param ev the event structure
 * @param payload the payload to be added
 * @param payloadLen the payload's length
 */
void LKEvent_addPayload(LKEvent* ev, void* payload, unsigned short payloadLen);

/**
 * Frees the payload of the event structure
 * @param ev the event structure that will have its payload freed
 */
void LKEvent_freePayload(LKEvent* ev);

/**
 * Initializes the request structure with the parameters given
 * The payload is set to NULL
 * @param req the request structure
 * @param protoOrigin the protocol id creating the request structure
 * @param protoDest the protocol id to which the request is to be delivered
 * @param request the request type, if it is a REQUEST or a REPLY
 * @param request_id the id code of the REQUEST/REPLY interaction
 */
void LKRequest_init(LKRequest* req, short protoOrigin, short protoDest, request_type request, short request_id);

/**
 * Adds payload to the existing payload of the request structure
 * This function reallocs the payload if more space is needed
 * @param req the request structure
 * @param payload the paylaod to be added
 * @param payloadLen the payload's length
 */
void LKRequest_addPayload(LKRequest* req, void* payload, unsigned short payloadLen);

/**
 * Frees the payload of the request structure
 * @param req the request structure that will have its paylaod freed
 */
void LKRequest_freePayload(LKRequest* req);

/**
 * Serializes the message content and headers of a message to the payload with the new payload and updates the headers
 * @param msg The original message
 * @param buffer The new content
 * @param len The lenght of the new content
 * @param protoID The protocol Id who requested the operation
 * @param newDest The new destination of the message
 * @return SUCCESS if the serialization concluded, FALSE if the new payload size if bigger than the constant MAX_PAYLOAD
 */
int pushPayload(LKMessage* msg, char* buffer, unsigned short len, short protoID, WLANAddr* newDest);

/**
 * Deserializes part of the message content that was pushed, and restores the original headers and original payload
 * @param msg The message
 * @param buffer The buffer to where the pushed content will be copied
 * @param readlen The number of bytes to read
 * @return The number of bytes read, or -1 if the bytes read is higher than the number of bytes asked to read
 */
int popPayload(LKMessage* msg, char* buffer, unsigned short readlen);

int pushEmptyPayload(LKMessage* msg, short protoID);
int popEmptyPayload(LKMessage* msg);

#endif /* PROTO_DATA_STRUCT_H_ */
