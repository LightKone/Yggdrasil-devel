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


#include "simpleAggregationWithBloomFilter.h"


struct contribution_t_{
	uuid_t id;
	double value;
	struct contribution_t_* next;
};

struct aggValueBloom_{
	double value;
	aggregation_function func;
	short nContributions;
	bloom_t contributions;

};

static double avgValue;

typedef enum AGG_OP_ {
	SUM = 0,
	MAX = 1,
	MIN = 2,
	AVG = 3,
	END = 4
}AGG_OP;

typedef struct agg_timer_ {
	LKTimer timer;
	short reset;
}agg_timer;

static short threshold = 5;

static size_t sizeOfBloom = 526;

static aggValueBloom aggvalues[4]; //4 elems each for each operations (use AGG_OP)

static contribution_t* neighs = NULL; //contribution of direct neighbors

static agg_timer agg_timers[4]; //4 elems each for each operations (use AGG_OP)

static double myValue = -1; //TODO -1 is that there is no value, this should be smarter

static uuid_t myId;

static short protoId;


//static void contribution_destroy(contribution_t** contributions){
//	while(*contributions != NULL){
//		contribution_t* old = *contributions;
//		*contributions = old->next;
//		old->next = NULL;
//		free(old);
//	}
//	*contributions = NULL;
//}

static void addToNeigh(contribution_t* c) {

	if(neighs == NULL){
		c->next = NULL;
		neighs = c;
	}else{
		contribution_t* n = neighs;
		if(uuid_compare(n->id, c->id) == 0){
			c->next = NULL;
			free(c);
		}else{
			while(n->next != NULL){
				if(uuid_compare(n->next->id, c->id) == 0){
					c->next = NULL;
					free(c);
					break;
				}
				n = n->next;
			}
			if(n->next == NULL){
				c->next = NULL;
				n->next = c;
			}
		}
	}
}

static void addContribution(aggValueBloom* av, contribution_t** contribution){

	contribution_t* new_c = *contribution;
	*contribution = new_c->next;




	bloom_add(av->contributions, new_c->id, sizeof(uuid_t));
	av->nContributions ++;
#ifdef DEBUG
	char u[37];
	memset(u, 0, 37);
	char m[2000];
	memset(m,0,2000);
	sprintf(m, "initial value: %f, adding new contribution %s with value %f total contributions now %d", av->value, simpleAggregation_contribution_getId(new_c, u), new_c->value, av->nContributions);
	lk_log("SIMPLE AGGREGATION BLOOM","ALIVE", m);
#endif
	av->value = av->func(av->value, new_c->value);

	addToNeigh(new_c);

}


static void deserializeAggValueBloom(aggValueBloom* av, void* payload, unsigned short datalen){


	memcpy(&av->value, payload, sizeof(double));
	payload += sizeof(double);
	datalen -= sizeof(double);

	memcpy(&av->nContributions, payload, sizeof(short));
	payload += sizeof(short);
	datalen -= sizeof(short);

	bloom_swap(av->contributions, payload, datalen);

}

static int getAggValueBloom(aggValueBloom* av, contribution_t* c, char* data, unsigned short datalen) {
	short op;

	void* payload = (void*)data;

	memcpy(&op, payload, sizeof(short));
	payload += sizeof(short);
	datalen -= sizeof(short);

	c->next = NULL;
	memcpy(&c->value, payload, sizeof(double));
	payload += sizeof(double);
	datalen -= sizeof(double);

	memcpy(c->id, payload, sizeof(uuid_t));
	payload += sizeof(uuid_t);
	datalen -= sizeof(uuid_t);

	deserializeAggValueBloom(av, payload, datalen);

	return op;
}


static unsigned short serializeAggValueBloom(aggValueBloom* av, void* payload) {

	unsigned short len = 0;

	memcpy(payload, &av->value, sizeof(double));
	len += sizeof(double);
	payload += sizeof(double);

	memcpy(payload, &av->nContributions, sizeof(short));
	len += sizeof(short);
	payload += sizeof(short);


	memcpy(payload, bloom_getBits(av->contributions), bloom_getSize(av->contributions));

	len += bloom_getSize(av->contributions);

	return len;
}

static unsigned short simpleAggregationWithBloomFilter_serializeMSG(short op, aggValueBloom* av, char* data) {
	void* payload = (void*)data;

	unsigned short len = 0;

	memcpy(payload, &op, sizeof(short));
	len += sizeof(short);
	payload += sizeof(short);

	memcpy(payload, &myValue, sizeof(double));
	len += sizeof(double);
	payload += sizeof(double);

	memcpy(payload, myId, sizeof(uuid_t));
	len += sizeof(uuid_t);
	payload += sizeof(uuid_t);

	len += serializeAggValueBloom(av, payload);

	return len;

}

static void simpleAggregationWithBloomFilter_serializeReply(LKRequest* reply, aggValueBloom* av){
	reply->proto_dest = reply->proto_origin;
	reply->proto_origin = protoId;

	reply->request = REPLY;
	reply->length = 0;

	if(reply->payload != NULL){
		free(reply->payload);
	}

	//double value + short ncontributions + contributions*(double value + uuid_t id)
	reply->payload = malloc(sizeof(double)+sizeof(short)+bloom_getSize(av->contributions));

	void* payload = reply->payload;

	reply->length = serializeAggValueBloom(av, payload);

}

static void checkValue(){
	if(myValue == -1){
		LKEvent ev;
		ev.proto_origin = protoId;
		ev.notification_id = SIMPLE_AGG_EVENT_REQUEST_VALUE;
		ev.length = 0;
		ev.payload = NULL;
		deliverEvent(&ev);
	}
}

static void init_bloom(aggValueBloom* avb) {
	avb->contributions = bloom_create(sizeOfBloom);
	bloom_add_hash(avb->contributions, &DJBHash);
	bloom_add_hash(avb->contributions, &JSHash);
	bloom_add_hash(avb->contributions, &RSHash);
}

void * simpleAggregationWithBloomFilter_init(void * args) {

	proto_args* pargs = (proto_args*)args;

	memcpy(myId, pargs->myuuid, sizeof(uuid_t));

	protoId = pargs->protoID;

	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;
	int i;
	for(i = 0; i < END; i++){
		agg_timers[i].reset = threshold;
		LKTimer_init(&agg_timers[i].timer, protoId, protoId);

		if(i == MIN){
			aggvalues[i].value = DBL_MAX;
			aggvalues[i].func = &minFunc;
		}
		else if(i == MAX){
			aggvalues[i].value = DBL_MIN;
			aggvalues[i].func = &maxFunc;
		}
		else{
			aggvalues[i].value = 0;
			aggvalues[i].func = &sumFunc;
		}
		init_bloom(&aggvalues[i]);
		aggvalues[i].nContributions  = 0;

	}

	while(1) {
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			aggValueBloom* av = malloc(sizeof(aggValueBloom));
			init_bloom(av);
			contribution_t* contribution = malloc(sizeof(contribution_t));
			contribution->next = NULL;
			int op = getAggValueBloom(av, contribution, elem.data.msg.data, elem.data.msg.dataLen);

#ifdef DEBUG
			char m[200];
			memset(m,0,200);
			sprintf(m, "Got a message with op: %d", op);
			lk_log("SIMPLE AGG", "ALIVE", m);
#endif
			if(op >= SUM && op <= AVG){
				checkValue();
				short initsize = aggvalues[op].nContributions;
				if(bloom_test_equal(aggvalues[op].contributions, bloom_getBits(av->contributions), bloom_getSize(av->contributions)) == 1){
					//test if the contribution received is in either set
					if(bloom_test(aggvalues[op].contributions, contribution->id, sizeof(uuid_t)) == 0) {
						//contribution received is not in my bloom filter
						addContribution(&aggvalues[op], &contribution);

					}else if(av->nContributions != aggvalues[op].nContributions){
						//this should not happen
						lk_log("SIMPLE AGGREGATION BLOOM", "WARNING", "Received bloom filter is equal but has different number of contributions");
					}

				}else if(bloom_test_disjoin(aggvalues[op].contributions, bloom_getBits(av->contributions), bloom_getSize(av->contributions)) == 1){
					//we are in luck boys!
					bloom_merge(aggvalues[op].contributions, bloom_getBits(av->contributions), bloom_getSize(av->contributions));
#ifdef DEBUG
					memset(m,0,200);
					sprintf(m, "Merging values: %f and %f contributions: %d and %d", aggvalues[op].value, av->value,aggvalues[op].nContributions, av->nContributions);
					lk_log("SIMPLE AGGREAGATION BLOOM", "ALIVE", m);
#endif
					aggvalues[op].value = aggvalues[op].func(aggvalues[op].value, av->value);
					aggvalues[op].nContributions += av->nContributions;

				}else {
					//test if the contribution received is not in my bloom
					if(bloom_test(aggvalues[op].contributions, contribution->id, sizeof(uuid_t)) == 0){
						addContribution(&aggvalues[op], &contribution);
					}
					//test the bloom I receive against all my stored contributions
					contribution_t* c = neighs;
					while(c != NULL) {

						if(bloom_test(av->contributions, c->id, sizeof(uuid_t)) == 0) {
							bloom_add(av->contributions, c->id, sizeof(uuid_t));
#ifdef DEBUG
							memset(m,0,200);
							char u[37];
							memset(u, 0, 37);
							av->nContributions++;
							sprintf(m, "Initial value %f adding value: %f of contribution: %s total contributions now %d",av->value, c->value, simpleAggregation_contribution_getId(c, u) , av->nContributions);
							lk_log("SIMPLE AGGREAGATION BLOOM", "ALIVE", m);
#endif
							av->value = aggvalues[op].func(av->value, c->value);
						}

						c = c->next;
					}
					//keep the biggest set
					if(av->nContributions > aggvalues[op].nContributions){
						aggvalues[op].nContributions = av->nContributions;
						aggvalues[op].value = av->value;
						bloom_swap(aggvalues[op].contributions, bloom_getBits(av->contributions), bloom_getSize(av->contributions));
					}else if(av->nContributions == aggvalues[op].nContributions){
						//if equal, keep the one closest to the value
						if(op == MIN){
							//The one with smallest value
							if(av->value < aggvalues[op].value){
								aggvalues[op].value = av->value;
								bloom_swap(aggvalues[op].contributions, bloom_getBits(av->contributions), bloom_getSize(av->contributions));
							}
						}else if(op == MAX){
							//The one with biggest value
							if(av->value > aggvalues[op].value){
								aggvalues[op].value = av->value;
								bloom_swap(aggvalues[op].contributions, bloom_getBits(av->contributions), bloom_getSize(av->contributions));
							}
						}else{
							//Else chose one at random to keep (they may be equal for all we know..)
							if(rand()%2 == 1){
								aggvalues[op].value = av->value;
								bloom_swap(aggvalues[op].contributions, bloom_getBits(av->contributions), bloom_getSize(av->contributions));
							}
						}
					}

				}

				if(initsize < aggvalues[op].nContributions) {
					if(op == AVG)
						avgValue = aggvalues[op].value / aggvalues[op].nContributions;
					if(agg_timers[op].reset >= threshold){
						LKTimer_set(&agg_timers[op].timer, 2000000 + rand()%2000000, 0);
						setupTimer(&agg_timers[op].timer);
					}
					agg_timers[op].reset = 0;
				}


			}else {
				char msg[200];
				memset(msg, 0, 200);
				sprintf(msg, "Invalid operation received %d", op);
				lk_log("SIMPLE AGG", "WARNING", msg);
			}

			if(contribution != NULL){
				free(contribution);
				contribution = NULL;
			}
			simpleAggregationWithBloomFilter_aggValueBloom_destroy(&av);

		} else if(elem.type == LK_TIMER){

			for(i = 0; i < END; i++){
				if(uuid_compare(agg_timers[i].timer.id, elem.data.timer.id) == 0){
					if(aggvalues[i].nContributions > 0 && myValue != -1){
						LKMessage msg;
						msg.LKProto = protoId;
						msg.dataLen = simpleAggregationWithBloomFilter_serializeMSG(i, &aggvalues[i], msg.data);
						setDestToBroadcast(&msg);
						dispatch(&msg);
						agg_timers[i].reset ++;
						if(agg_timers[i].reset < threshold){
							LKTimer_set(&agg_timers[i].timer, 2000000 + rand()%2000000, 0);
							setupTimer(&agg_timers[i].timer);
						}
					}else{
						LKTimer_set(&agg_timers[i].timer, 4000000 + rand()%2000000, 0);
						setupTimer(&agg_timers[i].timer);
					}
				}
			}
		} else if(elem.type == LK_REQUEST) {

			LKRequest req = elem.data.request;
			if(req.request == REPLY && req.request_type == SIMPLE_AGG_REPLY_FOR_VALUE){
				memcpy(&myValue, req.payload, sizeof(double));
				free(req.payload);
				for(i = 0; i < END; i ++){
					contribution_t* mycontribution = malloc(sizeof(contribution_t));
					mycontribution->next = NULL;
					memcpy(mycontribution->id,myId, sizeof(uuid_t));
					mycontribution->value = myValue;
#ifdef DEBUG
					char u[37];
					memset(u, 0, 37);
					char m[2000];
					memset(m,0,2000);
					sprintf(m, "adding my contribution %s with value %f",simpleAggregation_contribution_getId(mycontribution, u), mycontribution->value);
					lk_log("SIMPLE AGGREGATION BLOOM","ALIVE", m);
#endif
					if(bloom_test(aggvalues[i].contributions, mycontribution->id, sizeof(uuid_t)) == 0)
						addContribution(&aggvalues[i], &mycontribution);
				}
			}else if(req.request == REQUEST) {
				char ass[200];
				memset(ass, 0, 200);
				sprintf(ass, "Got a request for %d", req.request_type);
				lk_log("SIMPLE AGG", "ALIVE", ass);
				double aggValueAVG;

				switch(req.request_type){
				case SIMPLE_AGG_REQ_DO_SUM:
					checkValue();
					agg_timers[SUM].reset = 0;
					LKTimer_set(&agg_timers[SUM].timer, 2000000 + rand()%2000000, 0);
					setupTimer(&agg_timers[SUM].timer);
					break;
				case SIMPLE_AGG_REQ_DO_MAX:
					checkValue();
					agg_timers[MAX].reset = 0;
					LKTimer_set(&agg_timers[MAX].timer, 2000000 + rand()%2000000, 0);
					setupTimer(&agg_timers[MAX].timer);
					break;
				case SIMPLE_AGG_REQ_DO_MIN:
					checkValue();
					agg_timers[MIN].reset = 0;
					LKTimer_set(&agg_timers[MIN].timer, 2000000 + rand()%2000000, 0);
					setupTimer(&agg_timers[MIN].timer);
					break;
				case SIMPLE_AGG_REQ_DO_AVG:
					checkValue();
					agg_timers[AVG].reset = 0;
					LKTimer_set(&agg_timers[AVG].timer, 2000000 + rand()%2000000, 0);
					setupTimer(&agg_timers[AVG].timer);
					break;
				case SIMPLE_AGG_REQ_GET_SUM:
					simpleAggregationWithBloomFilter_serializeReply(&req, &aggvalues[SUM]);
					deliverReply(&req);
					break;
				case SIMPLE_AGG_REQ_GET_MAX:
					simpleAggregationWithBloomFilter_serializeReply(&req, &aggvalues[MAX]);
					deliverReply(&req);
					break;
				case SIMPLE_AGG_REQ_GET_MIN:
					simpleAggregationWithBloomFilter_serializeReply(&req, &aggvalues[MIN]);
					deliverReply(&req);
					break;
				case SIMPLE_AGG_REQ_GET_AVG:
					aggValueAVG = aggvalues[AVG].value;
					aggvalues[AVG].value = avgValue;
					simpleAggregationWithBloomFilter_serializeReply(&req, &aggvalues[AVG]);
					deliverReply(&req);
					aggvalues[AVG].value = aggValueAVG;
					break;
				default:
					break;
				}
			}
		}
	}

}

aggValueBloom* simpleAggregationWithBloomFilter_deserializeReply(void* data, unsigned short datalen) {

	aggValueBloom* reply = malloc(sizeof(aggValueBloom));
	init_bloom(reply);

	void * payload = data;

	deserializeAggValueBloom(reply, payload, datalen);

	return reply;

}

double simpleAggregationWithBloomFilter_aggValueBloom_getValue(aggValueBloom* av) {
	return av->value;
}
short simpleAggregationWithBloomFilter_aggValueBloom_getNContributions(aggValueBloom* av) {
	return av->nContributions;
}
bloom_t simpleAggregationWithBloomFilter_aggValueBloom_getContributions(aggValueBloom* av) {
	return av->contributions;
}

void simpleAggregationWithBloomFilter_aggValueBloom_destroy(aggValueBloom** av){

	if(*av != NULL){
		bloom_free((*av)->contributions);
		(*av)->contributions = NULL;
		free(*av);
		*av = NULL;
	}

}

