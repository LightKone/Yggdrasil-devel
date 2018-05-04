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

#include "proto_data_struct.h"

void LKMessage_initBcast(LKMessage* msg, short protoID) {
	setBcastAddr(&msg->destAddr);
	msg->LKProto = protoID;
	msg->dataLen = 0;
}

void LKMessage_init(LKMessage* msg, unsigned char addr[6], short protoID) {
	memcpy(msg->destAddr.data, addr, WLAN_ADDR_LEN);
	msg->LKProto = protoID;
	msg->dataLen = 0;
}

int LKMessage_addPayload(LKMessage* msg, char* payload, unsigned short payloadLen) {
	if(msg->dataLen + payloadLen > LK_MESSAGE_PAYLOAD)
		return FAILED;

	memcpy(msg->data+msg->dataLen, payload, payloadLen);
	msg->dataLen += payloadLen;
	return SUCCESS;
}

void LKTimer_init(LKTimer* timer, short protoOrigin, short protoDest) {
	genUUID(timer->id);
	timer->proto_origin = protoOrigin;
	timer->proto_dest = protoDest;
	timer->length = 0;
	timer->payload = NULL;
	timer->timer_type = -1;
}

void LKTimer_init_with_uuid(LKTimer* timer, uuid_t uuid, short protoOrigin, short protoDest) {
	memcpy(timer->id, uuid, sizeof(uuid_t));
	timer->proto_origin = protoOrigin;
	timer->proto_dest = protoDest;
	timer->length = 0;
	timer->payload = NULL;
	timer->timer_type = -1;
}

void LKTimer_set(LKTimer* timer, unsigned long firstNotification_us, unsigned long repeat_us) {
	timer->config.first_notification = time(NULL) + ((firstNotification_us / 1000000) < 1 ? 1 : (firstNotification_us / 1000000));
	if(repeat_us > 0)
		timer->config.repeat_interval_ms = ((repeat_us / 1000) < 1 ? 1 : (repeat_us / 1000));
	else
		timer->config.repeat_interval_ms = 0;
}

void LKTimer_setType(LKTimer* timer, short type) {
	timer->timer_type = type;
}

void LKTimer_setPayload(LKTimer* timer, void* payload, unsigned short payloadLen) {
	if(timer->payload != NULL)
		free(timer->payload);

	timer->payload = malloc(payloadLen);

	memcpy(timer->payload, payload, payloadLen);

	timer->length = payloadLen;
}

void LKTimer_addPayload(LKTimer* timer, void* payload, unsigned short payloadLen) {
	if(timer->payload == NULL){
		timer->payload = malloc(payloadLen);
	}else{
		timer->payload = realloc(timer->payload, timer->length + payloadLen);
	}

	memcpy(timer->payload+timer->length, payload, payloadLen);

	timer->length += payloadLen;
}

void LKTimer_freePayload(LKTimer* timer) {
	if(timer->length > 0 && timer->payload != NULL) {
		free(timer->payload);
		timer->payload = NULL;
		timer->length = 0;
	}
}

void LKEvent_init(LKEvent* ev, short protoOrigin, short notification_id) {
	ev->proto_origin = protoOrigin;
	ev->notification_id = notification_id;
	ev->length  = 0;
	ev->payload = NULL;
}

void LKEvent_addPayload(LKEvent* ev, void* payload, unsigned short payloadLen) {
	if(ev->payload == NULL){
			ev->payload = malloc(payloadLen);
		}else{
			ev->payload = realloc(ev->payload, ev->length + payloadLen);
		}

		memcpy(ev->payload+ev->length, payload, payloadLen);

		ev->length += payloadLen;
}

void LKEvent_freePayload(LKEvent* ev) {
	if(ev->length > 0 && ev->payload != NULL) {
		free(ev->payload);
		ev->payload = NULL;
		ev->length = 0;
	}
}

void LKRequest_init(LKRequest* req, short protoOrigin, short protoDest, request_type request, short request_id) {
	req->proto_origin = protoOrigin;
	req->proto_dest = protoDest;
	req->request = request;
	req->request_type = request_id;
}

void LKRequest_addPayload(LKRequest* req, void* payload, unsigned short payloadLen) {
	if(req->payload == NULL){
			req->payload = malloc(payloadLen);
		}else{
			req->payload = realloc(req->payload, req->length + payloadLen);
		}

		memcpy(req->payload+req->length, payload, payloadLen);

		req->length += payloadLen;
}

void LKRequest_freePayload(LKRequest* req) {
	if(req->length > 0 && req->payload != NULL) {
		free(req->payload);
		req->payload = NULL;
		req->length = 0;
	}
}

int pushPayload(LKMessage* msg, char* buffer, unsigned short len, short protoID, WLANAddr* newDest) {
	unsigned short newPayloadSize = len + (sizeof(unsigned short) * 3) + WLAN_ADDR_LEN + msg->dataLen;

	if(newPayloadSize > LK_MESSAGE_PAYLOAD)
		return FAILED;

	char dataCopy[msg->dataLen];
	memcpy(dataCopy, msg->data, msg->dataLen);

	void* tmp = msg->data;
	memcpy(tmp, &len, sizeof(unsigned short));
	tmp += sizeof(unsigned short);
	memcpy(tmp, buffer, len);
	tmp += len;
	memcpy(tmp, &msg->LKProto, sizeof(unsigned short));
	tmp += sizeof(unsigned short);
	memcpy(tmp, msg->destAddr.data, WLAN_ADDR_LEN);
	tmp += WLAN_ADDR_LEN;
	memcpy(tmp, &msg->dataLen, sizeof(unsigned short));
	tmp += sizeof(unsigned short);

	memcpy(tmp, dataCopy, msg->dataLen);

	msg->dataLen = newPayloadSize;
	msg->LKProto = protoID;
	memcpy(msg->destAddr.data, newDest->data, WLAN_ADDR_LEN);

	return SUCCESS;
}

int popPayload(LKMessage* msg, char* buffer, unsigned short readlen) {
	unsigned short readBytes = 0;
	void* tmp = msg->data;

	memcpy(&readBytes, tmp, sizeof(unsigned short));
	tmp += sizeof(unsigned short);

	if(readBytes > readlen)
		return -1;

	memcpy(buffer, tmp, readBytes);
	tmp += readBytes;

	msg->dataLen = msg->dataLen - sizeof(unsigned short) - readBytes;
	if(msg->dataLen > 0) {
		memcpy(&msg->LKProto, tmp, sizeof(unsigned short));
		tmp += sizeof(unsigned short);
		memcpy(msg->destAddr.data, tmp, WLAN_ADDR_LEN);
		tmp += WLAN_ADDR_LEN;
		unsigned short payloadLen = 0;
		memcpy(&payloadLen, tmp, sizeof(unsigned short));
		tmp += sizeof(unsigned short);
		msg->dataLen = msg->dataLen - ((sizeof(unsigned short) * 2) + WLAN_ADDR_LEN);
#ifdef DEBUG
		if(payloadLen != msg->dataLen) {
			char s[2000];
			memset(s, 0, 2000);
			sprintf(s, "Warning, enclosing message has %u bytes but was expected to have %u bytes instead\n",msg->dataLen, payloadLen);
			lk_log("LKMESSAGE", "WARNING", s);
		}
#endif
		char remainingPayload[payloadLen];
		memcpy(remainingPayload, tmp, payloadLen);

		memset(msg->data, 0, LK_MESSAGE_PAYLOAD);
		memcpy(msg->data, remainingPayload, payloadLen);
		msg->dataLen = payloadLen;
	}

	return readBytes;
}

int pushEmptyPayload(LKMessage* msg, short protoID) {

	unsigned short newPayloadSize = msg->dataLen + 2*sizeof(short);
	if(newPayloadSize > LK_MESSAGE_PAYLOAD)
		return FAILED;

	char dataCopy[msg->dataLen];
	memcpy(dataCopy, msg->data, msg->dataLen);

	void* tmp = msg->data;
	memcpy(tmp, &msg->LKProto, sizeof(unsigned short));
	tmp += sizeof(unsigned short);

	memcpy(tmp, &msg->dataLen, sizeof(unsigned short));
	tmp += sizeof(unsigned short);

	memcpy(tmp, dataCopy, msg->dataLen);

	msg->dataLen = newPayloadSize;
	msg->LKProto = protoID;

	return SUCCESS;
}

int popEmptyPayload(LKMessage* msg) {

	unsigned short readBytes = 0;
	void* tmp = msg->data;

	if(msg->dataLen > 0) {
		memcpy(&msg->LKProto, tmp, sizeof(unsigned short));
		tmp += sizeof(unsigned short);

		unsigned short payloadLen = 0;
		memcpy(&payloadLen, tmp, sizeof(unsigned short));
		tmp += sizeof(unsigned short);
		msg->dataLen = msg->dataLen - (sizeof(unsigned short)*2);

#ifdef DEBUG
		if(payloadLen != msg->dataLen) {
			char s[2000];
			memset(s, 0, 2000);
			sprintf(s, "Warning, enclosing message has %u bytes but was expected to have %u bytes instead\n",msg->dataLen, payloadLen);
			lk_log("LKMESSAGE", "WARNING", s);
		}
#endif
		char remainingPayload[payloadLen];
		memcpy(remainingPayload, tmp, payloadLen);

		memset(msg->data, 0, LK_MESSAGE_PAYLOAD);
		memcpy(msg->data, remainingPayload, payloadLen);
		msg->dataLen = payloadLen;
	}

	return readBytes;
}

