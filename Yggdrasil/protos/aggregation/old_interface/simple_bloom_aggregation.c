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


#include "simple_bloom_aggregation.h"

typedef struct simple_agg_value_{
	void* value;
	short size;
	short nCotributions;
	aggregation_function func;
	contribution_t* neighs;
	bloom_t contributions;
}simple_agg_value;

typedef struct simple_agg_timer_{
	LKTimer timer;
	short reset;
}simple_agg_timer;

typedef struct simple_bloom_agg_state_ {
	short threashold;
	size_t sizeOfBloom;
	contribution_t* mycontribution;
	simple_agg_value agg_value;
	simple_agg_timer timer;
}simple_bloom_agg_state;

typedef struct aggregation_instance_ {
	simple_bloom_agg_state state;
	uuid_t id;
	uuid_t timer_id;
	short sequence_number;
	agg_op op;
	LKRequest* request;
	struct aggregation_instance_* next;
}aggregation_instance;


static aggregation_instance* instances = NULL;

static short protoId;
static short sequence_number;
static uuid_t myId;

static void simple_bloom_aggregation_addInstance(aggregation_instance* newinstance) {
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

static aggregation_instance* simple_bloom_aggregation_findInstance(uuid_t id, short sequence_number, short op){

	aggregation_instance* it = instances;
	while(it != NULL){
		if(uuid_compare(id, it->id) == 0 && it->sequence_number == sequence_number && it->op == op){
			return it;
		}
		it = it->next;
	}

	return NULL;
}

static aggregation_instance* simple_bloom_aggregation_findInstanceByTimer(uuid_t timer_id){

	aggregation_instance* it = instances;
	while(it != NULL){
		if(uuid_compare(timer_id, it->timer_id) == 0){
			return it;
		}
		it = it->next;
	}

	return NULL;
}

static void init_bloom(simple_bloom_agg_state* state) {
	state->agg_value.contributions = bloom_create(state->sizeOfBloom);
	bloom_add_hash(state->agg_value.contributions, &DJBHash);
	bloom_add_hash(state->agg_value.contributions, &JSHash);
	bloom_add_hash(state->agg_value.contributions, &RSHash);
}

static void simple_bloom_aggregation_createInstance(short op, aggregation_function func, LKRequest* req, void* value, short size) {

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
	newinstance->state.sizeOfBloom = 526;

	newinstance->state.timer.reset = 0;
	LKTimer_init(&newinstance->state.timer.timer, protoId, protoId);
	memcpy(newinstance->timer_id, newinstance->state.timer.timer.id, sizeof(uuid_t));
	LKTimer_set(&newinstance->state.timer.timer, 2000000 + (rand()%2)*1000000, 0);

	init_bloom(&newinstance->state);

	newinstance->state.agg_value.func = func;
	newinstance->state.agg_value.nCotributions = 1;
	newinstance->state.mycontribution = contribution_init(myId, value, size);
	newinstance->state.agg_value.neighs = newinstance->state.mycontribution;
	newinstance->state.agg_value.size = size;
	newinstance->state.agg_value.value = value;
	uuid_t id;
	contribution_getId(newinstance->state.mycontribution, id);
	bloom_add(newinstance->state.agg_value.contributions, id, sizeof(uuid_t));

	simple_bloom_aggregation_addInstance(newinstance);

	setupTimer(&newinstance->state.timer.timer);
}

static void simple_bloom_aggregation_registInstance(aggregation_instance* instance) {

	instance->request = NULL;

	instance->state.threashold = 5;
	//init_bloom(&instance->state);

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

	instance->state.agg_value.neighs = mycontribution;

	contribution_addContribution(&instance->state.agg_value.neighs, &instance->state.mycontribution);

	instance->state.mycontribution = mycontribution;

	void* originalvalue = instance->state.agg_value.value;

	instance->state.agg_value.func(originalvalue, value, instance->state.agg_value.value);


	instance->state.agg_value.nCotributions ++;
	uuid_t id;
	contribution_getId(instance->state.mycontribution, id);
	bloom_add(instance->state.agg_value.contributions, id, sizeof(uuid_t));

	simple_bloom_aggregation_addInstance(instance);

	setupTimer(&instance->state.timer.timer);
}

static short simple_bloom_aggregation_destroyInstance(aggregation_instance* instance){
	if(instance->request != NULL)
		free(instance->request);
	instance->request = NULL;
	free(instance->state.agg_value.value);
	instance->state.agg_value.value = NULL;

	bloom_free(instance->state.agg_value.contributions);

	if(instance->state.agg_value.neighs != NULL)
		contribution_destroy(&instance->state.agg_value.neighs);
	if(instance->state.mycontribution != NULL)
		contribution_destroy(&instance->state.mycontribution);

	instance->next = NULL;
	free(instance);
	return 0;
}

static aggregation_instance* simple_bloom_aggregation_deserializeMsg(char* data, unsigned short dataLen) {

	aggregation_instance* instance = malloc(sizeof(aggregation_instance));

	instance->request = NULL;

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


	instance->state.mycontribution = contribution_deserialize(&payload, &len, 1);
	instance->state.agg_value.neighs = NULL;


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

	instance->state.sizeOfBloom = len;

	init_bloom(&instance->state);

	bloom_swap(instance->state.agg_value.contributions, payload, len);

	return instance;

}

static short simple_bloom_aggregation_serializeMSG(uuid_t id, short sequence_number, agg_op op, simple_bloom_agg_state* state, char* data) {
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

	short  contributionLen = contribution_serialize(&payload, LK_MESSAGE_PAYLOAD - len, state->mycontribution);

	if(contributionLen < 0)
		return -1;

	len += contributionLen;

	memcpy(payload, &state->agg_value.size, sizeof(short));
	len += sizeof(short);
	payload += sizeof(short);

	memcpy(payload, state->agg_value.value, state->agg_value.size);
	len += state->agg_value.size;
	payload += state->agg_value.size;

	memcpy(payload, &state->agg_value.nCotributions, sizeof(short));
	len += sizeof(short);
	payload += sizeof(short);

	memcpy(payload, bloom_getBits(state->agg_value.contributions), bloom_getSize(state->agg_value.contributions));

	len += bloom_getSize(state->agg_value.contributions);


	return len;
}

void simple_bloom_aggregation_addContribution(aggregation_instance* instance, contribution_t** contribution) {
	uuid_t id;
	contribution_getId(*contribution, id);
	bloom_add(instance->state.agg_value.contributions, id, sizeof(uuid_t));

	short size = contribution_getValueSize(*contribution);
	void* value = malloc(size);

	contribution_getValue(*contribution, value, size);

	instance->state.agg_value.func(instance->state.agg_value.value, value, instance->state.agg_value.value);
	instance->state.agg_value.nCotributions ++;

	contribution_addContribution(&instance->state.agg_value.neighs, contribution);

	free(value);
}

void* simple_bloom_aggregation_init(void* args){
	proto_args* pargs = (proto_args*)args;

	memcpy(myId, pargs->myuuid, sizeof(uuid_t));

	protoId = pargs->protoID;

	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;


	while(1) {
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			LKMessage msg = elem.data.msg;

			aggregation_instance* instance = simple_bloom_aggregation_deserializeMsg(msg.data, msg.dataLen);

			aggregation_instance* myInstance = simple_bloom_aggregation_findInstance(instance->id, instance->sequence_number, instance->op);
			if(myInstance == NULL){

				simple_bloom_aggregation_registInstance(instance);

			} else {
				//execute normal proto

				short initsize = myInstance->state.agg_value.nCotributions;

				if(bloom_test_equal(myInstance->state.agg_value.contributions, bloom_getBits(instance->state.agg_value.contributions), bloom_getSize(instance->state.agg_value.contributions)) == 1){
					//test if the contribution received is in either set
					uuid_t id;
					contribution_getId(instance->state.mycontribution, id);
					if(bloom_test(myInstance->state.agg_value.contributions, id, sizeof(uuid_t)) == 0) {
						//contribution received is not in my bloom filter
						simple_bloom_aggregation_addContribution(myInstance, &instance->state.mycontribution);

					}else if(instance->state.agg_value.nCotributions != myInstance->state.agg_value.nCotributions){
						//this should not happen
						lk_log("SIMPLE AGGREGATION BLOOM", "WARNING", "Received bloom filter is equal but has different number of contributions");
					}

				}else if(bloom_test_disjoin(myInstance->state.agg_value.contributions, bloom_getBits(instance->state.agg_value.contributions), bloom_getSize(instance->state.agg_value.contributions)) == 1){
					//we are in luck boys!
					bloom_merge(myInstance->state.agg_value.contributions, bloom_getBits(instance->state.agg_value.contributions), bloom_getSize(instance->state.agg_value.contributions));

					myInstance->state.agg_value.func(myInstance->state.agg_value.value, instance->state.agg_value.value, myInstance->state.agg_value.value);
					myInstance->state.agg_value.nCotributions += instance->state.agg_value.nCotributions;

				}else {
					//test if the contribution received is not in my bloom
					uuid_t id;
					contribution_getId(instance->state.mycontribution, id);
					if(bloom_test(myInstance->state.agg_value.contributions, id, sizeof(uuid_t)) == 0){

						simple_bloom_aggregation_addContribution(myInstance, &instance->state.mycontribution);
					}
					//test the bloom I receive against all my stored contributions
					contribution_t* c = myInstance->state.agg_value.neighs;
					while(c != NULL) {
						contribution_getId(c, id);
						if(bloom_test(instance->state.agg_value.contributions, id, sizeof(uuid_t)) == 0) {

							bloom_add(instance->state.agg_value.contributions, id, sizeof(uuid_t));

							short size = contribution_getValueSize(c);
							void* value = malloc(size);

							contribution_getValue(c, value, size);

							myInstance->state.agg_value.func(instance->state.agg_value.value, value, instance->state.agg_value.value);
							instance->state.agg_value.nCotributions ++;

							free(value);
						}

						c = contribution_getNext(c);
					}
					//keep the biggest set
					if(instance->state.agg_value.nCotributions > myInstance->state.agg_value.nCotributions){

						myInstance->state.agg_value.nCotributions = instance->state.agg_value.nCotributions;
						memcpy(myInstance->state.agg_value.value, instance->state.agg_value.value, instance->state.agg_value.size);
						bloom_swap(myInstance->state.agg_value.contributions, bloom_getBits(instance->state.agg_value.contributions), bloom_getSize(instance->state.agg_value.contributions));

					}else if(instance->state.agg_value.nCotributions == myInstance->state.agg_value.nCotributions){
						//if equal, keep the one closest to the value
						if(instance->op == MIN){
							//The one with smallest value
							if(aggregation_valueDiff(instance->state.agg_value.value, myInstance->state.agg_value.value) < 0){
								memcpy(myInstance->state.agg_value.value, instance->state.agg_value.value, instance->state.agg_value.size);
								bloom_swap(myInstance->state.agg_value.contributions, bloom_getBits(instance->state.agg_value.contributions), bloom_getSize(instance->state.agg_value.contributions));
							}
						}else if(instance->op == MAX){
							//The one with biggest value
							if(aggregation_valueDiff(instance->state.agg_value.value, myInstance->state.agg_value.value) > 0){
								memcpy(myInstance->state.agg_value.value, instance->state.agg_value.value, instance->state.agg_value.size);
								bloom_swap(myInstance->state.agg_value.contributions, bloom_getBits(instance->state.agg_value.contributions), bloom_getSize(instance->state.agg_value.contributions));
							}
						}else{
							//Else chose one at random to keep (they may be equal for all we know...)
							if(rand()%2 == 1){

								memcpy(myInstance->state.agg_value.value, instance->state.agg_value.value, instance->state.agg_value.size);
								bloom_swap(myInstance->state.agg_value.contributions, bloom_getBits(instance->state.agg_value.contributions), bloom_getSize(instance->state.agg_value.contributions));
							}
						}
					}

				}

				if(initsize < myInstance->state.agg_value.nCotributions) {
					if(myInstance->state.timer.reset >= myInstance->state.threashold){
						LKTimer_set(&myInstance->state.timer.timer, 2000000 + (rand()%2)*1000000, 0);
						setupTimer(&myInstance->state.timer.timer);
					}
					myInstance->state.timer.reset = 0;
				}

				simple_bloom_aggregation_destroyInstance(instance);
			}

		} else if(elem.type == LK_TIMER){
			LKTimer timer = elem.data.timer;

			aggregation_instance* instance = simple_bloom_aggregation_findInstanceByTimer(timer.id);

			LKMessage msg;
			LKMessage_initBcast(&msg, protoId);

			msg.dataLen = simple_bloom_aggregation_serializeMSG(instance->id, instance->sequence_number, instance->op, &instance->state, msg.data);
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

				char desc[2000];
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

				switch(req.request_type){
				case AGG_REQ_DO_SUM:
					size = aggregation_function_get_valueSize(SUM);
					value = malloc(size);
					if(aggregation_function_get_value(SUM, value) == NULL){
						lk_log("SIMPLE AGG", "ERROR", "Could not get value");
					}
					simple_bloom_aggregation_createInstance(SUM, sumFunction, &req, value, size);
					break;
				case AGG_REQ_DO_MAX:
					size = aggregation_function_get_valueSize(MAX);
					value = malloc(size);
					if(aggregation_function_get_value(MAX, value) == NULL){
						lk_log("SIMPLE AGG", "ERROR", "Could not get value");
					}
					simple_bloom_aggregation_createInstance(MAX, maxFunction, &req, value, size);
					break;
				case AGG_REQ_DO_MIN:
					size = aggregation_function_get_valueSize(MIN);
					value = malloc(size);
					if(aggregation_function_get_value(MIN, value) == NULL){
						lk_log("SIMPLE AGG", "ERROR", "Could not get value");
					}
					simple_bloom_aggregation_createInstance(MIN, minFunction, &req, value, size);
					break;
				case AGG_REQ_DO_AVG:
					size = aggregation_function_get_valueSize(AVG);
					value = malloc(size);
					if(aggregation_function_get_value(AVG, value) == NULL){
						lk_log("SIMPLE AGG", "ERROR", "Could not get value");
					}
					simple_bloom_aggregation_createInstance(AVG, avgFunction, &req, value, size);
					break;
				default:
					break;
				}
			}
		}
	}
}
