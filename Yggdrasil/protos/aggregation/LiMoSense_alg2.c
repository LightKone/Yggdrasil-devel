/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2018
 *********************************************************/

#include "LiMoSense_alg2.h"

#define BEACON 2*1000*1000

//PROTO DATA STRUCTURES
typedef struct est_{
	double val;
	double weight;
}est;


typedef struct neighs_{
	uuid_t id;
	WLANAddr addr;

	//	est val; //neigh estimation
	est sent;
	est recved;

	struct neighs_* next;
}neighs;

//YGGDRASIL STATE VARS:
static short protoId;
static short discovery_id;

static uuid_t my_id;

//STATE VARS:
static const double q = 1/24; //TODO define q ???
//static double bound = 24; //TODO define an arbitrary large bound, for example n, of the weight stored ???

static neighs* neighs_lst = NULL;
static int neis = 0;

static est my_est;
static est unrecved;
static double prevVal;


//AUXILIARY FUNCTIONS:
static void sum_est(est* est1, est* est2, est* res){
#ifdef DEBUG
	char s[500];
	bzero(s, 500);
	sprintf(s, "res->val = ((%f * %f) + (%f * %f)) / (%f + %f)   ", est1->val, est1->weight, est2->val, est2->weight, est1->weight, est2->weight);
	lk_log("LIMOSENSE","SUM EST",s);
#endif
	if(est1->weight + est2->weight != 0){
		res->val = (((est1->val*est1->weight) + (est2->val*est2->weight)) / (est1->weight + est2->weight));
		res->weight = est1->weight + est2->weight;
	} else{
		lk_log("LIMOSENSE", "ERROR", "Attempt to divide by zero");
	}

}

static void subtract_est(est* est1, est* est2, est* res){
	est est3;
	est3.val = est2->val;
	est3.weight = est2->weight*(-1);
	sum_est(est1, &est3, res);

}

static neighs* chooseRandom(){
	if(neis <= 0)
		return NULL;
	int n = rand() % neis;
	neighs* it = neighs_lst;
	while(it != NULL && n > 0){
		it = it->next;
		n--;
	}

	return it;
}

static neighs* find_nei(uuid_t n){
	neighs* it = neighs_lst;
	while(it != NULL){
		if(uuid_compare(it->id, n) == 0){
			return it;
		}
		it = it->next;
	}
	return NULL;
}

static neighs* addneigh(uuid_t n, WLANAddr* addr){
	neighs* newnei = malloc(sizeof(neighs));
	memcpy(newnei->id, n, sizeof(uuid_t));
	memcpy(newnei->addr.data, addr->data, WLAN_ADDR_LEN);
	//	newnei->val.val = 0;
	//	newnei->val.weight = 0;
	newnei->sent.val = 0;
	newnei->sent.weight = 0;
	newnei->recved.val = 0;
	newnei->recved.weight = 0;


	newnei->next = neighs_lst;
	neighs_lst = newnei;

	neis ++;

	return newnei;
}

static void rmneigh(uuid_t n){

	neighs* torm = NULL;
	if(neighs_lst != NULL){
		if(uuid_compare(neighs_lst->id, n) == 0){
			torm = neighs_lst;
			neighs_lst = torm->next;
			torm->next = NULL;


		}else{
			neighs* it = neighs_lst;
			while(it->next != NULL) {
				if(uuid_compare(it->next->id, n) == 0){
					torm = it->next;
					it->next = torm->next;
					torm->next = NULL;
					break;
				}
				it=it->next;
			}
		}
	}

	if(torm != NULL){

		sum_est(&my_est, &torm->sent, &my_est);

		sum_est(&unrecved, &torm->recved, &unrecved);

		neis--;
		free(torm);
	}

}

//EVENT PROCESSORS:
static void sendMsg(){
	neighs* tosend = chooseRandom();
	if(tosend == NULL){
#ifdef DEBUG
		char s[500];
		bzero(s, 500);
		sprintf(s, "no one to send to. my estimation: %f with weight: %f", my_est.val, my_est.weight);
		lk_log("LIMOSENSE","SENT MSG",s);
#endif
		return;
	}
	if(my_est.weight >= 2*q){
		double weight_tmp;
		if(unrecved.weight < my_est.weight - q){
			weight_tmp = unrecved.weight;
		}else{
			weight_tmp = my_est.weight - q;
		}
		est tmp_est;
		tmp_est.val = unrecved.val;
		tmp_est.weight = weight_tmp;
		subtract_est(&my_est, &tmp_est, &my_est);

		unrecved.weight = unrecved.weight - weight_tmp;

	}
	if(my_est.weight >= 2*q){
		est tmp_est;
		tmp_est.val = my_est.val;
		tmp_est.weight = (my_est.weight / 2);
		sum_est(&tosend->sent, &tmp_est, &tosend->sent);

		my_est.weight = tmp_est.weight;
	}

	LKMessage msg;
	LKMessage_init(&msg, tosend->addr.data, protoId);

	LKMessage_addPayload(&msg, (void*)my_id, sizeof(uuid_t));

	LKMessage_addPayload(&msg, (void*)&tosend->sent.val, sizeof(double)); //sent_i(j)
	LKMessage_addPayload(&msg, (void*)&tosend->sent.weight, sizeof(double));

	dispatch(&msg);
#ifdef DEBUG
	char s[500];
	bzero(s, 500);
	sprintf(s, "my estimation: %f with weight: %f", my_est.val, my_est.weight);
	lk_log("LIMOSENSE","SENT MSG",s);
#endif
}

static void processMsg(LKMessage* msg){

	uuid_t n;

	est in_est;

	void* payload = msg->data;
	memcpy(n, payload, sizeof(uuid_t));
	payload += sizeof(uuid_t);

	memcpy(&in_est.val, payload, sizeof(double));
	payload += sizeof(double);
	memcpy(&in_est.weight, payload, sizeof(double));

#ifdef DEBUG
	char s[500];
	bzero(s, 500);
	char u[37];
	uuid_unparse(n, u);
	sprintf(s, "received estimation: %f with weight: %f from %s", in_est.val, in_est.weight, u);
	lk_log("LIMOSENSE","RECV MSG",s);
#endif
	neighs* nei = find_nei(n);
	if(nei == NULL){
		nei = addneigh(n, &msg->srcAddr);
	}

	est diff;
	subtract_est(&in_est, &nei->recved, &diff);
	sum_est(&my_est, &diff, &my_est);
	nei->recved.val = in_est.val;
	nei->recved.weight = in_est.weight;

#ifdef DEBUG
	bzero(s, 500);
	sprintf(s, "my estimation: %f with weight: %f", my_est.val, my_est.weight);
	lk_log("LIMOSENSE","RECV MSG",s);
#endif
}

static void changeVal(double newval){
	my_est.val = my_est.val + (1/my_est.weight)*(newval - prevVal);
	prevVal = newval;
}

void* LiMoSense_alg2_init(void* args) {
	proto_args* pargs = (proto_args*) args;

	memcpy(my_id,pargs->myuuid,sizeof(uuid_t));
	protoId = pargs->protoID;
	discovery_id = *((short*)pargs->protoAttr);

	//PROTO ON INIT
	aggregation_get_value(DEFAULT, &my_est.val);
	my_est.weight = 1;
	prevVal = my_est.val;
	unrecved.val = 0;
	unrecved.weight = 0;
	//

	LKTimer timer;
	LKTimer_init(&timer, protoId, protoId);
	LKTimer_set(&timer, BEACON, BEACON);
	setupTimer(&timer);

	queue_t* inBox = pargs->inBox;
	queue_t_elem elem;
	while(1) {
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			//on recv
			processMsg(&elem.data.msg);

		}else if(elem.type == LK_TIMER){
			//send
			sendMsg();
			if(elem.data.timer.payload != NULL){
				free(elem.data.timer.payload);
				elem.data.timer.payload = NULL;
			}
		}else if(elem.type == LK_EVENT){
			if(elem.data.event.proto_origin == discovery_id){
				//topology changes
				if(elem.data.event.notification_id == DISCOV_FT_LB_EV_NEIGH_UP){
					//on addneighbor
					uuid_t n;
					WLANAddr addr;
					//extract n and addr from event payload
					void* data = elem.data.event.payload;
					memcpy(n, data, sizeof(uuid_t));
					memcpy(addr.data, data+sizeof(uuid_t), WLAN_ADDR_LEN);
					if(find_nei(n) == NULL)
						addneigh(n, &addr);

				}else if(elem.data.event.notification_id == DISCOV_FT_LB_EV_NEIGH_DOWN){
					//on remneighbor
					uuid_t n;
					//extract n from event payload
					void* data = elem.data.event.payload;
					memcpy(n, data, sizeof(uuid_t));

					rmneigh(n);

				}else {
					//WARNING
				}

			} else if(elem.data.event.proto_origin == 401){
				double newval;
				memcpy(&newval, elem.data.event.payload, sizeof(double));
				changeVal(newval);
			}

			if(elem.data.event.payload != NULL){
				free(elem.data.event.payload);
				elem.data.event.payload = NULL;
			}
		}else if(elem.type == LK_REQUEST){

			if(elem.data.request.request == REQUEST){
				//got a request to do something

				if(elem.data.request.request_type == AGG_INIT){
					//initiate some aggregation (avg, sum, count)

				}else if(elem.data.request.request_type == AGG_GET){
					//what is the estimated value?
					double myval = prevVal;
					double result = my_est.val;

					elem.data.request.request = REPLY;
					elem.data.request.proto_dest = elem.data.request.proto_origin;
					elem.data.request.proto_origin = protoId;
					elem.data.request.request_type = AGG_VAL;

					if(elem.data.request.payload != NULL){
						free(elem.data.request.payload);
						elem.data.request.payload = NULL;
					}

					elem.data.request.payload = malloc(sizeof(double) + sizeof(double));
					memcpy(elem.data.request.payload, &myval, sizeof(double));
					memcpy(elem.data.request.payload + sizeof(double), &result, sizeof(double));

					elem.data.request.length = sizeof(double) + sizeof(double);

					deliverReply(&elem.data.request);

					free(elem.data.request.payload);
					elem.data.request.payload = NULL;

				}else if(elem.data.request.request_type == AGG_TERMINATE){
					//reset state


				}else{
					lk_log("LIMOSENSE", "WARNING", "Unknown request type");
				}



			}else if(elem.data.request.request == REPLY){
				lk_log("LIMOSENSE", "WARNING", "LK Request, got an unexpected reply");
			}else{
				lk_log("LIMOSENSE", "WARNING", "LK Request, not a request nor a reply");
			}

			if(elem.data.request.payload != NULL){
				free(elem.data.request.payload);
				elem.data.request.payload = NULL;
			}
		}
	}

}
