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

#include "simple_aggregation.h"

typedef struct simple_agg_value_{
	void* value;
	short size;
	short nCotributions;
	aggregation_function func;
	contribution_t* contributions;
}simple_agg_value;

typedef struct simple_agg_timer_{
	LKTimer timer;
	short reset;
}simple_agg_timer;

typedef struct simple_agg_state {
	short threashold;
	simple_agg_value agg_value;
	simple_agg_timer timer;
}simple_agg_state;

typedef struct aggregation_instance_ {
	simple_agg_state state;
	uuid_t id;
	uuid_t timer_id;
	short sequence_number;
	agg_op op;
	LKRequest* request;
	struct aggregation_instance_* next;
}aggregation_instance;


aggregation_instance* instances = NULL;

static short protoId;
static short sequence_number;
static uuid_t myId;

static void simple_aggregation_addInstance(aggregation_instance* newinstance) {
	if(instances == NULL){
		instances = newinstance;
		instances->next = NULL;
		return;
	}
	aggregation_instance* it = instances;
	while(it->next != NULL){
		it = it->next;
	}
	it->next = newinstance;
	newinstance->next = NULL;
}

static aggregation_instance* simple_aggregation_findInstance(uuid_t id, short sequence_number, short op){

	aggregation_instance* it = instances;
	while(it != NULL){
		if(uuid_compare(id, it->id) == 0 && it->sequence_number == sequence_number && it->op == op){
			return it;
		}
		it = it->next;
	}

	return NULL;
}

static aggregation_instance* simple_aggregation_findInstanceByTimer(uuid_t timer_id){

	aggregation_instance* it = instances;
	while(it != NULL){
		if(uuid_compare(timer_id, it->timer_id) == 0){
			return it;
		}
		it = it->next;
	}

	return NULL;
}

static void simple_aggregation_createInstance(short op, aggregation_function func, LKRequest* req, void* value, short size) {

	aggregation_instance* newinstance = malloc(sizeof(aggregation_instance));
	memcpy(newinstance->id, myId, sizeof(uuid_t));
	newinstance->op = op;
	if(req != NULL){
		newinstance->request = malloc(sizeof(LKRequest));
		memcpy(newinstance->request, req, sizeof(LKRequest));
	}else{
		newinstance->request = NULL;
	}
	newinstance->sequence_number = sequence_number;
	sequence_number ++;

	newinstance->state.threashold = 5;

	newinstance->state.timer.reset = 0;
	LKTimer_init(&newinstance->state.timer.timer, protoId, protoId);
	memcpy(newinstance->timer_id, newinstance->state.timer.timer.id, sizeof(uuid_t));
	LKTimer_set(&newinstance->state.timer.timer, 2000000 + (rand()%2)*1000000, 0);

	newinstance->state.agg_value.func = func;
	newinstance->state.agg_value.nCotributions = 1;
	newinstance->state.agg_value.contributions =  contribution_init(myId, value, size);
	newinstance->state.agg_value.size = size;
	newinstance->state.agg_value.value = value;


	simple_aggregation_addInstance(newinstance);

	setupTimer(&newinstance->state.timer.timer);
}

static void simple_aggregation_registInstance(aggregation_instance* instance) {

	instance->request = NULL;

	instance->state.threashold = 5;

	instance->state.timer.reset = 0;
	LKTimer_init(&instance->state.timer.timer, protoId, protoId);
	memcpy(instance->timer_id, instance->state.timer.timer.id, sizeof(uuid_t));
	LKTimer_set(&instance->state.timer.timer, 2000000 + (rand()%2)*1000000, 0);

	instance->state.agg_value.func = aggregation_function_get(instance->op);

	short size = aggregation_function_get_valueSize(instance->op); //TODO
	void* value = malloc(size);

	if(aggregation_function_get_value(instance->op, value) == NULL){
		lk_log("SIMPLE AGG", "ERROR", "Could not get value");
	}


	contribution_t* mycontribution = contribution_init(myId, value, size);


	void* originalvalue = instance->state.agg_value.value;

	instance->state.agg_value.func(originalvalue, value, instance->state.agg_value.value);


	contribution_addContribution(&instance->state.agg_value.contributions, &mycontribution);
	instance->state.agg_value.nCotributions ++;

	simple_aggregation_addInstance(instance);

	setupTimer(&instance->state.timer.timer);
}

static short simple_aggregation_destroyInstance(aggregation_instance* instance){
	if(instance->request != NULL)
		free(instance->request);
	instance->request = NULL;
	free(instance->state.agg_value.value);
	instance->state.agg_value.value = NULL;
	contribution_destroy(&instance->state.agg_value.contributions);
	instance->next = NULL;
	free(instance);
	return 0;
}

static aggregation_instance* simple_aggregation_deserializeMsg(char* data, unsigned short dataLen) {

	aggregation_instance* instance = malloc(sizeof(aggregation_instance));


	void* payload = (void*)data;

	unsigned short len = dataLen;

	memcpy(instance->id, payload, sizeof(uuid_t));
	len -= sizeof(uuid_t);
	payload += sizeof(uuid_t);

	memcpy(&instance->sequence_number, payload, sizeof(short));
	len -= sizeof(short);
	payload += sizeof(short);

	memcpy(&instance->op, payload, sizeof(agg_op));
	len -= sizeof(agg_op);
	payload += sizeof(agg_op);

	memcpy(&instance->state.agg_value.size, payload, sizeof(short));
	len -= sizeof(short);
	payload += sizeof(short);

	instance->state.agg_value.value = malloc(instance->state.agg_value.size);

	memcpy(instance->state.agg_value.value, payload, instance->state.agg_value.size);
	len -= instance->state.agg_value.size;
	payload += instance->state.agg_value.size;

	memcpy(&instance->state.agg_value.nCotributions, payload, sizeof(short));
	len -= sizeof(short);
	payload += sizeof(short);

	instance->state.agg_value.contributions = contribution_deserialize(&payload, &len, instance->state.agg_value.nCotributions);


	return instance;

}

static short simple_aggregation_serializeMSG(uuid_t id, short sequence_number, agg_op op, simple_agg_state* state, char* data) {
	void* payload = (void*)data;

	unsigned short len = 0;

	memcpy(payload, id, sizeof(uuid_t));
	len += sizeof(uuid_t);
	payload += sizeof(uuid_t);

	memcpy(payload, &sequence_number, sizeof(short));
	len += sizeof(short);
	payload += sizeof(short);

	memcpy(payload, &op, sizeof(agg_op));
	len += sizeof(agg_op);
	payload += sizeof(agg_op);

	memcpy(payload, &state->agg_value.size, sizeof(short));
	len += sizeof(short);
	payload += sizeof(short);

	memcpy(payload, state->agg_value.value, state->agg_value.size);
	len += state->agg_value.size;
	payload += state->agg_value.size;

	memcpy(payload, &state->agg_value.nCotributions, sizeof(short));
	len += sizeof(short);
	payload += sizeof(short);

	short  contributionLen = contribution_serialize(&payload, LK_MESSAGE_PAYLOAD - len, state->agg_value.contributions);

	if(contributionLen < 0)
		return -1;

	len += contributionLen;

	return len;
}

void* simple_aggregation_init(void* args){
	proto_args* pargs = (proto_args*)args;

	memcpy(myId, pargs->myuuid, sizeof(uuid_t));

	protoId = pargs->protoID;

	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;


	while(1) {
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			LKMessage msg = elem.data.msg;

			aggregation_instance* instance = simple_aggregation_deserializeMsg(msg.data, msg.dataLen);

			aggregation_instance* myInstance = simple_aggregation_findInstance(instance->id, instance->sequence_number, instance->op);

			if(myInstance == NULL){
				simple_aggregation_registInstance(instance);

			} else {
				//execute normal proto
				contribution_t* newcontributions = contribution_getNewContributions(myInstance->state.agg_value.contributions, &instance->state.agg_value.contributions);

				short initsize = myInstance->state.agg_value.nCotributions;

				while(newcontributions != NULL){
					short valsize = contribution_getValueSize(newcontributions);
					void * val = malloc(valsize);
					contribution_getValue(newcontributions, val, valsize);
					myInstance->state.agg_value.func(myInstance->state.agg_value.value, val, myInstance->state.agg_value.value);
					contribution_addContribution(&myInstance->state.agg_value.contributions, &newcontributions);
					myInstance->state.agg_value.nCotributions ++;

					free(val);
				}
				if(initsize < myInstance->state.agg_value.nCotributions) {
					if(myInstance->state.timer.reset >= myInstance->state.threashold){
						LKTimer_set(&myInstance->state.timer.timer, 2000000 + (rand()%2)*1000000, 0);
						setupTimer(&myInstance->state.timer.timer);
					}
					myInstance->state.timer.reset = 0;
				}

				simple_aggregation_destroyInstance(instance);
			}

		} else if(elem.type == LK_TIMER){
			LKTimer timer = elem.data.timer;

			lk_log("SIMPLE AGG", "ALIVE", "got a timer");

			aggregation_instance* instance = simple_aggregation_findInstanceByTimer(timer.id);

			char desc[2000];
			bzero(desc, 2000);
			sprintf(desc, "instance %d", instance->sequence_number);

			lk_log("SIMPLE AGG", "ALIVE", desc);

			LKMessage msg;
			LKMessage_initBcast(&msg, protoId);

			msg.dataLen = simple_aggregation_serializeMSG(instance->id, instance->sequence_number, instance->op, &instance->state, msg.data);
			if(msg.dataLen > 0){
				dispatch(&msg);
			}else
				lk_log("SIMPLE AGGREGATION", "ERROR", "Unable to serialize message");

			instance->state.timer.reset ++;
			if(instance->state.timer.reset < instance->state.threashold){
				LKTimer_set(&instance->state.timer.timer, 2000000 + (rand()%2)*1000000, 0);
				setupTimer(&instance->state.timer.timer);
			} else {
				//TODO report value
				double value;
				memcpy(&value, instance->state.agg_value.value, sizeof(double));

				bzero(desc, 2000);
				sprintf(desc, "reporting value %f", value);
				lk_log("SIMPLE AGG", "ALIVE", desc);
				if(instance->request != NULL)
					aggregation_reportValue(instance->request, protoId, value);
			}


		} else if(elem.type == LK_REQUEST) {

			LKRequest req = elem.data.request;
			if(req.request == REPLY && req.request_type == AGG_REPLY_FOR_VALUE){

			}
			else if(req.request == REQUEST) {
				void* value; //TODO: SET THIS
				short size; //TODO

				lk_log("SIMPLE AGG", "ALIVE", "got a request");

				switch(req.request_type){
				case AGG_REQ_DO_SUM:
					size = aggregation_function_get_valueSize(SUM);
					value = malloc(size);
					if(aggregation_function_get_value(SUM, value) == NULL){
						lk_log("SIMPLE AGG", "ERROR", "Could not get value");
					}
					simple_aggregation_createInstance(SUM, sumFunction, &req, value, size);
					break;
				case AGG_REQ_DO_MAX:
					size = aggregation_function_get_valueSize(MAX);
					value = malloc(size);
					if(aggregation_function_get_value(MAX, value) == NULL){
						lk_log("SIMPLE AGG", "ERROR", "Could not get value");
					}
					simple_aggregation_createInstance(MAX, maxFunction, &req, value, size);
					break;
				case AGG_REQ_DO_MIN:
					size = aggregation_function_get_valueSize(MIN);
					value = malloc(size);
					if(aggregation_function_get_value(MIN, value) == NULL){
						lk_log("SIMPLE AGG", "ERROR", "Could not get value");
					}
					simple_aggregation_createInstance(MIN, minFunction, &req, value, size);
					break;
				case AGG_REQ_DO_AVG:
					size = aggregation_function_get_valueSize(AVG);
					value = malloc(size);
					if(aggregation_function_get_value(AVG, value) == NULL){
						lk_log("SIMPLE AGG", "ERROR", "Could not get value");
					}
					simple_aggregation_createInstance(AVG, avgFunction, &req, value, size);
					break;
				default:
					break;
				}
			}
		}
	}
}
