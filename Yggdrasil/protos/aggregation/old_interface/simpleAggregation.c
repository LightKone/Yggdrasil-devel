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


#include "simpleAggregation.h"

struct contribution_t_{
	uuid_t id;
	double value;
	struct contribution_t_* next;
};

struct aggValue_{
	double value;
	short nContributions;
	aggregation_function* func;
	contribution_t* contributions;
};

double sumFunc(double value1, double value2){
	return value1 + value2;
}

double minFunc(double value1, double value2) {
	return min(value1, value2);
}

double maxFunc(double value1, double value2) {
	return max(value1, value2);
}

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

static aggValue aggvalues[4]; //4 elems each for each operations (use AGG_OP)

static agg_timer agg_timers[4]; //4 elems each for each operations (use AGG_OP)

static double myValue = -1; //TODO -1 is that there is no value, this should be smarter

static uuid_t myId;

static short protoId;


static void contribution_destroy(contribution_t* contributions){
	while(contributions != NULL){
		contribution_t* old = contributions;
		contributions = old->next;
		old->next = NULL;
		free(old);
	}
}

static void addContribution(aggValue* av, contribution_t** contribution){

	contribution_t* new_c = *contribution;
	*contribution = new_c->next;

	new_c->next = av->contributions;
	av->contributions = new_c;
	av->nContributions++;

#ifdef DEBUG
	char u[37];
	memset(u, 0, 37);
	char m[200];
	memset(m,0,200);
	sprintf(m, "added new contribution %s", simpleAggregation_contribution_getId(new_c, u));
	lk_log("SIMPLE AGG","ALIVE", m);
#endif
}

static contribution_t* getNewContributions(contribution_t* mine, contribution_t** other){

	contribution_t* newcontributions = NULL;
	contribution_t* it_new;


	while((*other) != NULL) {
		contribution_t* it_mine = mine;
		int new = 1;
		while(it_mine != NULL){
			if(uuid_compare(it_mine->id, (*other)->id) == 0){
				new = 0;
				break;
			}
			it_mine = it_mine->next;
		}
		if(new == 1) {
			if(newcontributions == NULL){
				newcontributions = *other;
#ifdef DEBUG
				char u[37];
				memset(u, 0, 37);
				char m[200];
				memset(m,0,200);
				sprintf(m, "added new elem %s", simpleAggregation_contribution_getId(newcontributions, u));
				lk_log("SIMPLE AGG","ALIVE", m);
#endif
				(*other) = newcontributions->next;
				newcontributions->next = NULL;
				it_new = newcontributions;
			}else{
				it_new->next = (*other);
				it_new = it_new->next;
				(*other) = it_new->next;
#ifdef DEBUG
				char u[37];
				memset(u, 0, 37);
				char m[200];
				memset(m,0,200);
				sprintf(m,"added new elem %s", simpleAggregation_contribution_getId(it_new, u));
				lk_log("SIMPLE AGG","ALIVE", m);
#endif
				if(it_new != NULL)
					it_new->next = NULL;
			}

		}else{
			contribution_t* old = *other;
			*other = old->next;
			old->next = NULL;
			free(old);
		}
	}

	return newcontributions;
}

static void deserializeAggValue(aggValue* av, void* payload, unsigned short datalen){


	memcpy(&av->value, payload, sizeof(double));
	payload += sizeof(double);
	datalen += sizeof(double);

	memcpy(&av->nContributions, payload, sizeof(short));
	payload += sizeof(short);
	datalen -= sizeof(short);

	contribution_t* contributions = NULL;

	int i;
	for(i = 0; i < av->nContributions && datalen >= (sizeof(uuid_t) + sizeof(double)); i ++) {
		contribution_t* contribution = malloc(sizeof(contribution_t));
		contribution->next = NULL;
		memcpy(&contribution->value, payload, sizeof(double));
		payload += sizeof(double);
		datalen -= sizeof(double);
		memcpy(contribution->id, payload, sizeof(uuid_t));
		payload += sizeof(uuid_t);
		datalen -= sizeof(uuid_t);
		if(av->contributions == NULL){
			av->contributions = contribution;
			contributions = av->contributions;
		}else{
			contributions->next = contribution;
			contributions = contributions->next;
		}
	}
}

static int getAggValue(aggValue* av, char* data, unsigned short datalen) {
	short op;

	void* payload = (void*)data;

	memcpy(&op, payload, sizeof(short));
	payload += sizeof(short);
	datalen -= sizeof(short);

	deserializeAggValue(av, payload, datalen);

	return op;
}


static unsigned short serializeAggValue(aggValue* av, void* payload) {

	unsigned short len = 0;

	memcpy(payload, &av->value, sizeof(double));
	len += sizeof(double);
	payload += sizeof(double);

	memcpy(payload, &av->nContributions, sizeof(short));
	len += sizeof(short);
	payload += sizeof(short);

	int i;
	contribution_t* contributions = av->contributions;
	for(i = 0; i < av->nContributions && contributions != NULL; i++){
		memcpy(payload, &contributions->value, sizeof(double));
		len += sizeof(double);
		payload += sizeof(double);
		memcpy(payload, contributions->id, sizeof(uuid_t));
		len+= sizeof(uuid_t);
		payload += sizeof(uuid_t);
		contributions = contributions->next;
	}

	return len;
}

static unsigned short simpleAggregation_serializeMSG(short op, aggValue* av, char* data) {
	void* payload = (void*)data;

	unsigned short len = 0;

	memcpy(payload, &op, sizeof(short));
	len += sizeof(short);
	payload += sizeof(short);

	len += serializeAggValue(av, payload);

	return len;

}

static void simpleAggregation_serializeReply(LKRequest* reply, aggValue* av){
	reply->proto_dest = reply->proto_origin;
	reply->proto_origin = protoId;

	reply->request = REPLY;
	reply->length = 0;

	if(reply->payload != NULL){
		free(reply->payload);
	}

	//double value + short ncontributions + contributions*(double value + uuid_t id)
	reply->payload = malloc(sizeof(double)+sizeof(short)+(sizeof(double)+sizeof(uuid_t))*av->nContributions);

	void* payload = reply->payload;

	reply->length = serializeAggValue(av, payload);

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

void * simpleAggregation_init(void * args) {

	proto_args* pargs = (proto_args*)args;

	memcpy(myId, pargs->myuuid, sizeof(uuid_t));

	protoId = pargs->protoID;

	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;
	int i;
	for(i = 0; i < END; i++){
		agg_timers[i].reset = threshold;
		LKTimer_init(&agg_timers[i].timer, protoId, protoId);

		if(i == MIN)
			aggvalues[i].value = DBL_MAX;
		else if(i == MAX)
			aggvalues[i].value = DBL_MIN;
		else
			aggvalues[i].value = 0;
		aggvalues[i].contributions = NULL;
		aggvalues[i].nContributions  = 0;

	}

	while(1) {
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			aggValue* av = malloc(sizeof(aggValue));
			av->contributions = NULL;
			int op = getAggValue(av, elem.data.msg.data, elem.data.msg.dataLen);
#ifdef DEBUG
			char m[200];
			memset(m,0,200);
			sprintf(m, "Got a message with op: %d", op);
			lk_log("SIMPLE AGG", "ALIVE", m);
#endif
			if(op >= SUM && op <= AVG){
				checkValue();
			}

			if(op == SUM){
				contribution_t* newcontributions = getNewContributions(aggvalues[op].contributions, &av->contributions);
				short initsize = aggvalues[op].nContributions;

				while(newcontributions != NULL){
					aggvalues[op].value += newcontributions->value;
					addContribution(&aggvalues[op], &newcontributions);
				}
				if(initsize < aggvalues[op].nContributions) {
					if(agg_timers[op].reset >= threshold){
						LKTimer_set(&agg_timers[op].timer, 2000000 + rand()%2000000, 0);
						setupTimer(&agg_timers[op].timer);
					}
					agg_timers[op].reset = 0;
				}

			}else if(op == MAX) {
				contribution_t* newcontributions = getNewContributions(aggvalues[op].contributions, &av->contributions);
				short initsize = aggvalues[op].nContributions;
				while(newcontributions != NULL){
					aggvalues[op].value = max(newcontributions->value, aggvalues[op].value);
					addContribution(&aggvalues[op], &newcontributions);
				}
				if(initsize < aggvalues[op].nContributions) {
					if(agg_timers[op].reset >= threshold){
						LKTimer_set(&agg_timers[op].timer, 2000000 + rand()%2000000, 0);
						setupTimer(&agg_timers[op].timer);
					}
					agg_timers[op].reset = 0;
				}

			}else if(op == MIN) {
				contribution_t* newcontributions = getNewContributions(aggvalues[op].contributions, &av->contributions);
				short initsize = aggvalues[op].nContributions;
				while(newcontributions != NULL){
					aggvalues[op].value = min(newcontributions->value, aggvalues[op].value);
					addContribution(&aggvalues[op], &newcontributions);
				}
				if(initsize < aggvalues[op].nContributions) {
					if(agg_timers[op].reset >= threshold){
						LKTimer_set(&agg_timers[op].timer, 2000000 + rand()%2000000, 0);
						setupTimer(&agg_timers[op].timer);
					}
					agg_timers[op].reset = 0;
				}

			}else if(op == AVG) {
				contribution_t* newcontributions = getNewContributions(aggvalues[op].contributions, &av->contributions);
				short initsize = aggvalues[op].nContributions;
				while(newcontributions != NULL){
					aggvalues[op].value += newcontributions->value;
					addContribution(&aggvalues[op], &newcontributions);
				}
				if(aggvalues[op].nContributions > 0 && initsize < aggvalues[op].nContributions) {
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

			simpleAggregation_aggValue_destroy(&av);

		} else if(elem.type == LK_TIMER){

			for(i = 0; i < END; i++){
				if(uuid_compare(agg_timers[i].timer.id, elem.data.timer.id) == 0){
					if(aggvalues[i].nContributions > 0){
						LKMessage msg;
						msg.LKProto = protoId;
						msg.dataLen = simpleAggregation_serializeMSG(i, &aggvalues[i], msg.data);
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
					if(i == MIN)
						aggvalues[i].value = min(myValue, aggvalues[i].value);
					else if(i == MAX)
						aggvalues[i].value = max(myValue, aggvalues[i].value);
					else
						aggvalues[i].value += myValue;

					contribution_t* mycontribution = malloc(sizeof(contribution_t));
					memcpy(mycontribution->id,myId, sizeof(uuid_t));
					mycontribution->value = myValue;
					addContribution(&aggvalues[i], &mycontribution);
				}
			}else if(req.request == REQUEST) {
#ifdef DEBUG
				char ass[200];
				memset(ass, 0, 200);
				sprintf(ass, "Got a request for %d", req.request_type);
				lk_log("SIMPLE AGG", "ALIVE", ass);
#endif
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
					simpleAggregation_serializeReply(&req, &aggvalues[SUM]);
					deliverReply(&req);
					break;
				case SIMPLE_AGG_REQ_GET_MAX:
					simpleAggregation_serializeReply(&req, &aggvalues[MAX]);
					deliverReply(&req);
					break;
				case SIMPLE_AGG_REQ_GET_MIN:
					simpleAggregation_serializeReply(&req, &aggvalues[MIN]);
					deliverReply(&req);
					break;
				case SIMPLE_AGG_REQ_GET_AVG:
					aggValueAVG = aggvalues[AVG].value;
					aggvalues[AVG].value = avgValue;
					simpleAggregation_serializeReply(&req, &aggvalues[AVG]);
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

aggValue* simpleAggregation_deserializeReply(void* data, unsigned short datalen) {

	aggValue* reply = malloc(sizeof(aggValue));

	void * payload = data;

	deserializeAggValue(reply, payload, datalen);

	return reply;

}

double simpleAggregation_aggValue_getValue(aggValue* av) {
	return av->value;
}
short simpleAggregation_aggValue_getNContributions(aggValue* av) {
	return av->nContributions;
}
contribution_t* simpleAggregation_aggValue_getContributions(aggValue* av) {
	return av->contributions;
}

void simpleAggregation_aggValue_destroy(aggValue** av){

	if(*av != NULL){
		contribution_destroy((*av)->contributions);
		free(*av);
		*av = NULL;
	}

}

char* simpleAggregation_contribution_getId(contribution_t* contribution, char str[37]) {
	memset(str, 0, 37);
	uuid_unparse(contribution->id, str);
	return str;
}
double simpleAggregation_contribution_getValue(contribution_t* contribution) {
	return contribution->value;
}

contribution_t* simpleAggregation_contribution_getNext(contribution_t* contribution) {
	return contribution->next;
}
