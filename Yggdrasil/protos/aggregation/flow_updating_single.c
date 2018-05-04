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


#include "flow_updating_single.h"

#define transmission_time 3*1000*1000 //3 seconds


typedef struct flow_updating_opt_ {
	//short max_rounds; //number of maximum rounds
	short tbd;
}flow_updating_opt;

typedef struct flow_lst_{
	uuid_t neigh_id;
	double flow_val;
	double estimation;
	struct flow_lst_* next;
}flow_lst;

typedef struct state_{
	agg_gen_op op;
	agg_val val_def;
	flow_updating_opt opt;
	double input_val;

	double estimation;

	short n_neighs;
	flow_lst* flows;

	uuid_t id;

	LKTimer timer;

}state;


static short protoId;

static short discovery_proto;

static state* instance_state; //



static void flow_updating_add_opt(flow_updating_opt* opt, flow_updating_opt_code code, short val){
	switch(code){

	default:
		break;
	}
}

static void flow_updating_extract_opt(flow_updating_opt* opt, void* meta_opt, unsigned short meta_opt_size){
	void* tmp = meta_opt;
	short opt_code;
	short opt_val;

	while(meta_opt_size >= sizeof(short) + sizeof(short)){
		memcpy(&opt_code, tmp, sizeof(short));
		tmp += sizeof(short);
		memcpy(&opt_val, tmp, sizeof(short));
		tmp += sizeof(short);

		flow_updating_add_opt(opt, opt_code, opt_val);

		meta_opt_size -= sizeof(short) + sizeof(short);
	}

}

static unsigned short flow_updating_get_info(state* sta, void** info){
	*info = malloc(sizeof(agg_gen_op) + sizeof(agg_val));

	memcpy(*info, &(sta->op), sizeof(agg_gen_op));
	memcpy(*info + sizeof(agg_gen_op), &(sta->val_def), sizeof(agg_val));

	return sizeof(agg_gen_op) + sizeof(agg_val);
}

static void flow_updating_init_agg(state** sta, agg_meta* metainfo, double val){
	short init = 0;
	if(*sta == NULL){
		*sta = malloc(sizeof(state));
		init = 1;
	}

	if(init){

		(*sta)->input_val = val;
		(*sta)->n_neighs = 0;
		(*sta)->op = metainfo->op;
		(*sta)->val_def = metainfo->val;
		//(*sta)->opt.max_rounds

		genUUID((*sta)->id);

		if(metainfo->opt != NULL){
			flow_updating_extract_opt(&((*sta)->opt), metainfo->opt, metainfo->opt_size);

		}

		//		if((*sta)->opt.max_rounds == 0){
		//			(*sta)->timer.opt.max_rounds = -1;
		//		}else
		//			(*sta)->timer.opt.max_rounds = (*sta)->opt.max_rounds;


		LKTimer_init(&((*sta)->timer), protoId, protoId);
		LKTimer_set(&((*sta)->timer), transmission_time, transmission_time);

		void* info;
		unsigned short info_size = flow_updating_get_info(*sta, &info);

		LKTimer_addPayload(&((*sta)->timer), info, info_size);

		setupTimer(&((*sta)->timer));

		free(info);
	}else{
		//		if((*sta)->timer.opt.max_rounds == 0){ //ended and can start again with new parameters
		//
		//			(*sta)->current_val.val = val;
		//			(*sta)->current_val.weight = weight;
		//
		//			if(metainfo->opt != NULL){
		//				push_sum_extract_opt(&((*sta)->opt), metainfo->opt, metainfo->opt_size);
		//
		//			}
		//			if((*sta)->opt.max_rounds == 0){
		//				(*sta)->timer.opt.max_rounds = -1;
		//			}else
		//				(*sta)->timer.opt.max_rounds = (*sta)->opt.max_rounds;
		//		}
	}
}



static void flow_updating_calculate_estimate(state* sta){

	//flow updating estimation
	flow_lst* lst = sta->flows;

	double flows = 0;
	double estimations = 0;

	while(lst != NULL){
		flows += lst->flow_val;
		estimations += lst->estimation;

		lst = lst->next;
	}

	sta->estimation = (((sta->input_val - flows) + estimations) / (sta->n_neighs + 1));

	lst = sta->flows;
	while(lst != NULL){
		lst->flow_val = lst->flow_val + (sta->estimation - lst->estimation);
		lst->estimation = sta->estimation;

		char s[1000];
		bzero(s, 1000);
		char u[37];
		uuid_unparse(lst->neigh_id, u);
		sprintf(s, "calculated flow %f for %s", lst->flow_val, u);
		lk_log("FLOW UDATING", "DEBUG", s);

		lst = lst->next;
	}

}


static void destroy_flow_lst(state* sta){

	while(sta->flows != NULL){
		flow_lst* torm = sta->flows;
		sta->flows = torm->next;
		torm->next = NULL;
		free(torm);
		torm = NULL;
	}
}

static void flow_updating_add_flow(state* sta, flow_lst* new_flow) {

	if(sta->flows == NULL){
		new_flow->next = sta->flows;
		sta->flows = new_flow;

		char s[1000];
		bzero(s, 1000);
		char u[37];
		uuid_unparse(new_flow->neigh_id, u);
		sprintf(s, "adding flow %f from %s", new_flow->flow_val, u);
		lk_log("FLOW UDATING", "DEBUG", s);

		sta->n_neighs ++;

	}else{
		flow_lst* it = sta->flows;
		while(it->next != NULL){
			if(uuid_compare(it->next->neigh_id, new_flow->neigh_id) == 0){
				free(new_flow);
				return;
			}else{
				it = it->next;
			}
		}


		char s[1000];
		bzero(s, 1000);
		char u[37];
		uuid_unparse(new_flow->neigh_id, u);
		sprintf(s, "adding flow %f from %s", new_flow->flow_val, u);
		lk_log("FLOW UDATING", "DEBUG", s);

		it->next = new_flow;
		sta->n_neighs ++;

	}

}

static short flow_updating_destroy_flow(state* sta, uuid_t neigh_id) {

	if(sta->flows != NULL){
		if(uuid_compare(sta->flows->neigh_id, neigh_id) == 0){
			flow_lst* torm = sta->flows;
			sta->flows = torm->next;

			torm->next = NULL;
			free(torm);
			torm = NULL;

			sta->n_neighs --;

			return SUCCESS;
		}else{
			flow_lst* it = sta->flows;
			while(it->next != NULL){
				if(uuid_compare(it->next->neigh_id, neigh_id) == 0){

					flow_lst* torm = it->next;
					it->next = torm->next;
					torm->next = NULL;
					free(torm);
					torm = NULL;

					sta->n_neighs --;

					return SUCCESS;
				}
				it = it->next;
			}
		}
	}
	return FAILED;
}

static flow_lst* find_neigh_flow(state* sta, uuid_t id){
	flow_lst* it = sta->flows;

	while(it != NULL){
		if(uuid_compare(id, it->neigh_id) == 0)
			break;
		it = it->next;
	}

	return it;
}


static void flow_updating_process_msg(state* sta, uuid_t myid, void* data, unsigned short data_size){

	uuid_t sender_id;
	double sender_estimation;
	memcpy(sender_id, data, sizeof(uuid_t));
	data += sizeof(uuid_t);
	memcpy(&sender_estimation, data, sizeof(double));
	data += sizeof(double);

	data_size -= (sizeof(uuid_t) + sizeof(double));

	char s[1000];
	bzero(s, 1000);
	char u[37];
	uuid_unparse(sender_id, u);
	sprintf(s, "got message from %s", u);
	lk_log("FLOW UDATING", "DEBUG", s);

	flow_lst* neigh = find_neigh_flow(sta, sender_id);
	if(neigh == NULL){
		neigh = malloc(sizeof(flow_lst));
		memcpy(neigh->neigh_id, sender_id, sizeof(uuid_t));
		neigh->next = NULL;
		flow_updating_add_flow(sta, neigh);
	}

	neigh->estimation = sender_estimation;

	while(data_size >= sizeof(uuid_t) + sizeof(double)){
		uuid_t id;
		memcpy(id, data, sizeof(uuid_t));

		data += sizeof(uuid_t);
		data_size -= sizeof(uuid_t);
		if(uuid_compare(id, myid) == 0){
			memcpy(&neigh->flow_val, data, sizeof(double));
			neigh->flow_val = neigh->flow_val * (-1);

			bzero(s, 1000);
			sprintf(s, "updated flow %f for %s", neigh->flow_val, u);
			lk_log("FLOW UDATING", "FLOW", s);
			break;
		}
		data += sizeof(double);
		data_size -= sizeof(double);
	}

}

void* flow_updating_single_init(void* args){
	proto_args* pargs = (proto_args*)args;

	protoId = pargs->protoID;
	discovery_proto = ((flow_updating_attr*) pargs->protoAttr)->discovery_proto_id;

	queue_t* inBox = pargs->inBox;

	instance_state = NULL;

	queue_t_elem elem;

	char s[1000];
	bzero(s, 1000);
	char u[37];

	int round = 1;

	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			void* payload = elem.data.msg.data;
			uuid_t id;
			agg_gen_op op;
			agg_val valdef;
			memcpy(id, payload, sizeof(uuid_t));
			payload += sizeof(uuid_t);

			memcpy(&op, payload, sizeof(agg_gen_op));
			payload += sizeof(agg_gen_op);
			memcpy(&valdef, payload, sizeof(agg_val));
			payload += sizeof(agg_val);
			//state* sta = find_instance(id)

			state* sta = instance_state;
			if(sta == NULL){
				double val;
				agg_meta* meta = malloc(sizeof(agg_meta));
				meta->op = op;
				meta->val = valdef;
				meta->opt_size = 0;
				meta->opt = NULL;
				if(aggregation_get_value(valdef, &val) == SUCCESS)
					flow_updating_init_agg(&instance_state, meta, val);
				else{
					lk_log("FLOW UPDATING", "ERROR", "Failed to retrive value");
					exit(-1);
				}
				free(meta);
			}

			unsigned short data_size = elem.data.msg.dataLen - (sizeof(uuid_t) + sizeof(agg_gen_op) + sizeof(agg_val));
			flow_updating_process_msg(sta, pargs->myuuid, payload, data_size);

		}else if(elem.type == LK_TIMER){
			//received timer, ask from discovery a random node
			if(instance_state != NULL){
				flow_updating_calculate_estimate(instance_state);

				bzero(s, 1000);
				sprintf(s, "new estimation for round %d: %f", round, instance_state->estimation);
				lk_log("FLOW UDATING", "ESTIMATION", s);

				//build message and send it
				LKMessage msg;
				LKMessage_initBcast(&msg, protoId);
				//METADATA -> instance id, instance operation, instance value type
				LKMessage_addPayload(&msg, (void*) instance_state->id, sizeof(uuid_t));
				LKMessage_addPayload(&msg, (void*) &instance_state->op, sizeof(agg_gen_op));
				LKMessage_addPayload(&msg, (void*) &instance_state->val_def, sizeof(agg_val));

				//message payload
				LKMessage_addPayload(&msg, (void*) pargs->myuuid, sizeof(uuid_t));
				LKMessage_addPayload(&msg, (void*) &instance_state->estimation, sizeof(double));

				flow_lst* lst = instance_state->flows;
				while(lst != NULL){
					LKMessage_addPayload(&msg, (void*) lst->neigh_id, sizeof(uuid_t));
					LKMessage_addPayload(&msg, (void*) &lst->flow_val, sizeof(double));
					lst = lst->next;
				}

				dispatch(&msg);
				round ++;
			}else
				lk_log("FLOW UPDATING", "ERROR", "Could not find state to continue algorithm");

			free(elem.data.timer.payload);

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

					flow_updating_add_flow(instance_state, new_flow);

				}else if(elem.data.event.notification_id == DISCOV_FT_LB_EV_NEIGH_DOWN){
					//process down neight

					uuid_t torm;
					memcpy(torm, elem.data.event.payload, sizeof(uuid_t));

					bzero(s, 1000);
					uuid_unparse(torm, u);
					sprintf(s, "node %s went down", u);
					lk_log("FLOW UDATING", "DEBUG", s);

					flow_updating_destroy_flow(instance_state, torm);

					bzero(s, 1000);
					sprintf(s, "node %s removed", u);
					lk_log("FLOW UDATING", "DEBUG", s);

				}else{
					lk_log("FLOW UPDATING", "WARNING", "Discovery sent unknown event");
				}

				if(elem.data.event.payload != NULL){
					free(elem.data.event.payload);
					elem.data.event.payload = NULL;
				}

			}else{
				lk_log("FLOW UPDATING", "WARNING", "Unknown event received");
			}
		}else if(elem.type == LK_REQUEST){
			if(elem.data.request.request == REQUEST){
				//got a request to do something

				if(elem.data.request.request_type == AGG_INIT){
					//initiate some aggregation (avg, sum, count)
					agg_meta* metainfo = aggregation_deserialize_agg_meta(elem.data.request.length, elem.data.request.payload);
					double val;
					if(metainfo->op != OP_COUNT && aggregation_get_value(metainfo->val, &(val)) == FAILED){
						elem.data.request.request = REPLY;
						elem.data.request.request_type = AGG_FAILED;
						deliverReply(&elem.data.request);

					}else{
						switch(metainfo->op){
						case OP_AVG:
							if(instance_state == NULL)
								flow_updating_init_agg(&instance_state, metainfo, val);
							break;

						default:
							//return with op not supported
							elem.data.request.request = REPLY;
							elem.data.request.request_type = AGG_NOT_SUPPORTED;
							deliverReply(&elem.data.request);
							break;
						}
					}

					aggregation_meta_destroy(&metainfo);
					if(elem.data.request.payload != NULL){
						free(elem.data.request.payload);
						elem.data.request.payload = NULL;
					}

				}else if(elem.data.request.request_type == AGG_GET){
					//what is the estimated value?
					if(instance_state != NULL){
						double result = instance_state->estimation;
						elem.data.request.request = REPLY;
						elem.data.request.proto_dest = elem.data.request.proto_origin;
						elem.data.request.proto_origin = protoId;
						elem.data.request.request_type = AGG_VAL;
						if(elem.data.request.payload != NULL){
							free(elem.data.request.payload);
							elem.data.request.payload = NULL;
						}
						elem.data.request.payload = malloc(sizeof(double));
						memcpy(elem.data.request.payload, &result, sizeof(double));

						elem.data.request.length = sizeof(double);

						deliverReply(&elem.data.request);

						free(elem.data.request.payload);
						elem.data.request.payload = NULL;
					}
				}else if(elem.data.request.request_type == AGG_TERMINATE){
					//reset state
					cancelTimer(&instance_state->timer);
					destroy_flow_lst(instance_state);
					free(instance_state);
					instance_state = NULL;


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

