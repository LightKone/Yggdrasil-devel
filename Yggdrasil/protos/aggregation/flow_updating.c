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


#include "flow_updating.h"

#define BEACON 2*1000*1000 //2 seconds

//PROTO DATA STRUCTURES
typedef struct flow_lst_{
	uuid_t neigh_id;
	double flow_val;
	double estimation;
	struct flow_lst_* next;
}flow_lst;


//YGGDRASIL VARS
static short protoId;
static short discovery_proto;
static uuid_t myuuid;

//PROTO STATE VARS
double input_val;
double estimation;

short n_neighs = 0;
flow_lst* flows = NULL;


//AUX FUNCTIONS:
static void destroy_flow_lst(){

	while(flows != NULL){
		flow_lst* torm = flows;
		flows = torm->next;
		torm->next = NULL;
		free(torm);
		torm = NULL;
	}
}

static void flow_updating_calculate_estimate(){

	//flow updating estimation
	flow_lst* lst = flows;

	double flows_val = 0;
	double estimations = 0;

	while(lst != NULL){
		flows_val += lst->flow_val;
		estimations += lst->estimation;

		lst = lst->next;
	}

	estimation = (((input_val - flows_val) + estimations) / (n_neighs + 1));

	lst = flows;
	while(lst != NULL){
		lst->flow_val = lst->flow_val + (estimation - lst->estimation);
		lst->estimation = estimation;

		lst = lst->next;
	}

}

static void flow_updating_add_flow(flow_lst* new_flow) {

	if(flows == NULL){
		new_flow->next = flows;
		flows = new_flow;

		n_neighs ++;

	}else{
		flow_lst* it = flows;
		while(it->next != NULL){
			if(uuid_compare(it->next->neigh_id, new_flow->neigh_id) == 0){
				free(new_flow);
				return;
			}else{
				it = it->next;
			}
		}

		it->next = new_flow;
		n_neighs ++;

	}

}

static short flow_updating_destroy_flow(uuid_t neigh_id) {

	if(flows != NULL){
		if(uuid_compare(flows->neigh_id, neigh_id) == 0){
			flow_lst* torm = flows;
			flows = torm->next;

			torm->next = NULL;
			free(torm);
			torm = NULL;

			n_neighs --;

			return SUCCESS;
		}else{
			flow_lst* it = flows;
			while(it->next != NULL){
				if(uuid_compare(it->next->neigh_id, neigh_id) == 0){

					flow_lst* torm = it->next;
					it->next = torm->next;
					torm->next = NULL;
					free(torm);
					torm = NULL;

					n_neighs --;

					return SUCCESS;
				}
				it = it->next;
			}
		}
	}
	return FAILED;
}

static flow_lst* find_neigh_flow(uuid_t id){
	flow_lst* it = flows;

	while(it != NULL){
		if(uuid_compare(id, it->neigh_id) == 0)
			break;
		it = it->next;
	}

	return it;
}

//EVENT PROCESSORS:
static void flow_updating_send_msg(){
	flow_updating_calculate_estimate();

	//build message and send it
	LKMessage msg;
	LKMessage_initBcast(&msg, protoId);

	//message payload
	LKMessage_addPayload(&msg, (void*) myuuid, sizeof(uuid_t));
	LKMessage_addPayload(&msg, (void*) &estimation, sizeof(double));

	flow_lst* lst = flows;
	while(lst != NULL){
		LKMessage_addPayload(&msg, (void*) lst->neigh_id, sizeof(uuid_t));
		LKMessage_addPayload(&msg, (void*) &lst->flow_val, sizeof(double));
		lst = lst->next;
	}

	dispatch(&msg);

}
static void flow_updating_process_msg(LKMessage* msg){

	void* data = msg->data;
	unsigned short data_size = msg->dataLen;

	uuid_t sender_id;
	double sender_estimation;
	memcpy(sender_id, data, sizeof(uuid_t));
	data += sizeof(uuid_t);
	memcpy(&sender_estimation, data, sizeof(double));
	data += sizeof(double);

	data_size -= (sizeof(uuid_t) + sizeof(double));

#ifdef DEBUG
	char s[1000];
	bzero(s, 1000);
	char u[37];
	uuid_unparse(sender_id, u);
	sprintf(s, "got message from %s", u);
	lk_log("FLOW UDATING", "DEBUG", s);
#endif

	flow_lst* neigh = find_neigh_flow(sender_id);
	if(neigh == NULL){
		neigh = malloc(sizeof(flow_lst));
		memcpy(neigh->neigh_id, sender_id, sizeof(uuid_t));
		neigh->next = NULL;
		flow_updating_add_flow(neigh);
	}

	neigh->estimation = sender_estimation;

	while(data_size >= sizeof(uuid_t) + sizeof(double)){
		uuid_t id;
		memcpy(id, data, sizeof(uuid_t));

		data += sizeof(uuid_t);
		data_size -= sizeof(uuid_t);
		if(uuid_compare(id, myuuid) == 0){
			memcpy(&neigh->flow_val, data, sizeof(double));
			neigh->flow_val = neigh->flow_val * (-1);

#ifdef DEBUG
			bzero(s, 1000);
			sprintf(s, "updated flow %f for %s", neigh->flow_val, u);
			lk_log("FLOW UDATING", "FLOW", s);
#endif
			break;
		}
		data += sizeof(double);
		data_size -= sizeof(double);
	}

}

void* flow_updating_init(void* args){
	proto_args* pargs = (proto_args*)args;

	protoId = pargs->protoID;
	discovery_proto = *((short*) pargs->protoAttr);

	flows = NULL;

	aggregation_get_value(DEFAULT, &input_val);
	estimation = 0;


	memcpy(myuuid, pargs->myuuid, sizeof(uuid_t));
	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;

	LKTimer timer;
	LKTimer_init(&timer, protoId, protoId);
	LKTimer_set(&timer, BEACON, BEACON);
	setupTimer(&timer);

	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){

			flow_updating_process_msg(&elem.data.msg);

		}else if(elem.type == LK_TIMER){

			flow_updating_send_msg();

			if(elem.data.timer.payload != NULL){
				free(elem.data.timer.payload);
				elem.data.timer.payload = NULL;
			}


		}else if(elem.type == LK_EVENT){
			//received some event, check who sent it?
			if(elem.data.event.proto_origin == discovery_proto){
				if(elem.data.event.notification_id == DISCOV_FT_LB_EV_NEIGH_UP){
					//process new neight
					flow_lst* new_flow = malloc(sizeof(flow_lst));

					new_flow->flow_val = 0;
					new_flow->estimation = 0;
					new_flow->next = NULL;
					memcpy(new_flow->neigh_id, elem.data.event.payload, sizeof(uuid_t));

					flow_updating_add_flow(new_flow);

				}else if(elem.data.event.notification_id == DISCOV_FT_LB_EV_NEIGH_DOWN){
					//process down neight

					uuid_t torm;
					memcpy(torm, elem.data.event.payload, sizeof(uuid_t));

					flow_updating_destroy_flow(torm);

				}else{
					lk_log("FLOW UPDATING", "WARNING", "Discovery sent unknown event");
				}

			} else if(elem.data.event.proto_origin == 401){
				double newval;
				memcpy(&newval, elem.data.event.payload, sizeof(double));
				input_val = newval;
			}
			else{
				lk_log("FLOW UPDATING", "WARNING", "Unknown event received");
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

						elem.data.request.request = REPLY;
						elem.data.request.proto_dest = elem.data.request.proto_origin;
						elem.data.request.proto_origin = protoId;
						elem.data.request.request_type = AGG_VAL;

						if(elem.data.request.payload != NULL){
							free(elem.data.request.payload);
							elem.data.request.payload = NULL;
						}
						elem.data.request.payload = malloc(sizeof(double) + sizeof(double));
						memcpy(elem.data.request.payload, &input_val, sizeof(double));
						memcpy(elem.data.request.payload + sizeof(double), &estimation, sizeof(double));

						elem.data.request.length = sizeof(double) + sizeof(double);

						deliverReply(&elem.data.request);

						free(elem.data.request.payload);
						elem.data.request.payload = NULL;

				}else if(elem.data.request.request_type == AGG_TERMINATE){
					//reset state
					cancelTimer(&timer);
					destroy_flow_lst();

				}else{
					lk_log("FLOW UPDATING", "WARNING", "Unknown request type");
				}

				if(elem.data.request.payload != NULL){
					free(elem.data.request.payload);
					elem.data.request.payload = NULL;
				}

			}else if(elem.data.request.request == REPLY){
				//random node to sent to from discovery, split val and weight and send it
				if(elem.data.request.payload != NULL){
					free(elem.data.request.payload);
					elem.data.request.payload = NULL;
				}
				lk_log("FLOW UPDATING", "WARNING", "Got an unexcepted reply");
			}else{
				lk_log("FLOW UPDATING", "WARNING", "LK Request, not a request nor a reply");
			}
		}else{
			lk_log("FLOW UPDATING", "WARNING", "Unknown element type popped");
		}
	}
}

