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


#include "push_sum_single.h"

#define transmission_time 3*1000*1000 //3 seconds


typedef struct push_sum_opt_ {
	short max_rounds; //number of maximum rounds
}push_sum_opt;

typedef struct push_sum_val{
	double val;
	double weight;
}val;

typedef struct push_sum_timers_{
	LKTimer timer;
	push_sum_opt opt;

}push_sum_timer;

typedef struct state_{
	agg_gen_op op;
	agg_val val_def;
	push_sum_opt opt;
	val current_val;

	uuid_t id;

	push_sum_timer timer;

}state;

typedef struct send_list_{
	val v;
	uuid_t id;
	agg_gen_op op;
	agg_val valdef;
	struct send_list_* next;
}send_lst;

static short protoId;

static short reliable_proto;
static short discovery_proto;

static state* instance_state; //three states operating at once (AVG, SUM and COUNT)

static send_lst* toSend = NULL;


static void push_sum_add_opt(push_sum_opt* opt, push_sum_opt_code code, short val){
	switch(code){
	case MAX_ROUNDS:
		opt->max_rounds = val;
		break;
	default:
		break;
	}
}

static void push_sum_extract_opt(push_sum_opt* opt, void* meta_opt, unsigned short meta_opt_size){
	void* tmp = meta_opt;
	short opt_code;
	short opt_val;

	while(meta_opt_size >= sizeof(short) + sizeof(short)){
		memcpy(&opt_code, tmp, sizeof(short));
		tmp += sizeof(short);
		memcpy(&opt_val, tmp, sizeof(short));
		tmp += sizeof(short);

		push_sum_add_opt(opt, opt_code, opt_val);

		meta_opt_size -= sizeof(short) + sizeof(short);
	}

}

static unsigned short push_sum_get_info(state* sta, void** info){
	*info = malloc(sizeof(agg_gen_op) + sizeof(agg_val));

	memcpy(*info, &(sta->op), sizeof(agg_gen_op));
	memcpy(*info + sizeof(agg_gen_op), &(sta->val_def), sizeof(agg_val));

	return sizeof(agg_gen_op) + sizeof(agg_val);
}

static void push_sum_init_agg(state** sta, agg_meta* metainfo, double weight, double val){
	short init = 0;
	if(*sta == NULL){
		*sta = malloc(sizeof(state));
		init = 1;
	}

	if(init){

		(*sta)->current_val.val = val;
		(*sta)->current_val.weight = weight;
		(*sta)->op = metainfo->op;
		(*sta)->opt.max_rounds = 0;

		genUUID((*sta)->id);

		if(metainfo->opt != NULL){
			push_sum_extract_opt(&((*sta)->opt), metainfo->opt, metainfo->opt_size);

		}
		if((*sta)->opt.max_rounds == 0){
			(*sta)->timer.opt.max_rounds = -1;
		}else
			(*sta)->timer.opt.max_rounds = (*sta)->opt.max_rounds;

		LKTimer_init(&((*sta)->timer.timer), protoId, protoId);
		LKTimer_set(&((*sta)->timer.timer), transmission_time, transmission_time);

		void* info;
		unsigned short info_size = push_sum_get_info(*sta, &info);

		LKTimer_addPayload(&((*sta)->timer.timer), info, info_size);

		setupTimer(&((*sta)->timer.timer));

		free(info);
	}else{
		if((*sta)->timer.opt.max_rounds == 0){ //ended and can start again with new parameters

			(*sta)->current_val.val = val;
			(*sta)->current_val.weight = weight;

			if(metainfo->opt != NULL){
				push_sum_extract_opt(&((*sta)->opt), metainfo->opt, metainfo->opt_size);

			}
			if((*sta)->opt.max_rounds == 0){
				(*sta)->timer.opt.max_rounds = -1;
			}else
				(*sta)->timer.opt.max_rounds = (*sta)->opt.max_rounds;
		}
	}
}


void add_to_send_lst(val v, uuid_t id, agg_gen_op op, agg_val valdef){
	if(toSend == NULL){
		toSend = malloc(sizeof(send_lst));
		toSend->next = NULL;
		toSend->v = v;
		memcpy(toSend->id, id, sizeof(uuid_t));
		toSend->op = op;
		toSend->valdef = valdef;
	}else{
		send_lst* tmp = toSend;
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = malloc(sizeof(send_lst));
		tmp->next->v = v;
		memcpy(tmp->next->id, id, sizeof(uuid_t));
		tmp->next->op = op;
		tmp->next->valdef = valdef;
		tmp->next->next = NULL;
	}
}

void push_sum_split(state* sta){
	val v;
	v.val = sta->current_val.val / 2;
	v.weight = sta->current_val.weight / 2;

	add_to_send_lst(v, sta->id, sta->op, sta->val_def);

	sta->current_val.val -= v.val;
	sta->current_val.weight -= v.weight;

}

void push_sum_add_val(state* sta, double value, double weight){
	sta->current_val.val += value;
	sta->current_val.weight += weight;

}

short get_val_to_send(val* v, void** info) {
	if(toSend != NULL){
		v->val = toSend->v.val;
		v->weight = toSend->v.weight;

		*info = malloc(sizeof(uuid_t) + sizeof(agg_gen_op) + sizeof(agg_val));
		void* tmp = *info;
		memcpy(tmp, toSend->id, sizeof(uuid_t));
		tmp += sizeof(uuid_t);
		memcpy(tmp, &toSend->op, sizeof(agg_gen_op));
		tmp += sizeof(agg_gen_op);
		memcpy(tmp, &toSend->valdef, sizeof(agg_val));

		send_lst* torm = toSend;
		toSend = torm->next;
		free(torm);
		return sizeof(uuid_t) + sizeof(agg_gen_op) + sizeof(agg_val);
	}

	return -1;
}

void destroy_send_lst() {
	while(toSend != NULL){
		send_lst* torm = toSend;
		toSend = torm->next;

		torm->next = NULL;
		free(torm);

	}
}

void request_destination(){
	LKRequest req;
	req.proto_origin = protoId;
	req.proto_dest = discovery_proto;
	req.request = REQUEST;
	req.request_type = DISC_REQ_GET_RND_NEIGH;
	req.length = 0;
	req.payload = NULL;

	deliverRequest(&req);
}

void push_sum_process_msg(LKMessage* msg, short init){
	//received message, add val and weight to local
	uuid_t id;

	void* payload = msg->data;

	memcpy(id, payload, sizeof(uuid_t));
	payload += sizeof(uuid_t);
	unsigned short tmp_size = sizeof(agg_gen_op) + sizeof(agg_val);

	if(instance_state == NULL && init){

		agg_gen_op op;
		agg_val valdef;

		memcpy(&op, payload, sizeof(agg_gen_op));
		payload += sizeof(agg_gen_op);
		memcpy(&valdef, payload, sizeof(agg_val));
		payload += sizeof(agg_val);

		double value;
		double weight;

		switch(op){
		case OP_AVG:
			aggregation_get_value(valdef, &value);
			weight = 1;
			break;
		case OP_SUM:
			aggregation_get_value(valdef, &value);
			weight = 0;
			break;
		case OP_COUNT:
			value = 1;
			weight = 0;
			break;
		default:
			break;
		}

		agg_meta meta;
		meta.op = op;
		meta.val = valdef;
		meta.opt = NULL;
		meta.opt_size = 0;
		push_sum_init_agg(&instance_state, &meta, weight, value);

		memcpy(instance_state->id, id, sizeof(uuid_t));
	}else{
		payload += tmp_size;
		int r = uuid_compare(id, instance_state->id);
		if(r < 0){
			//received id is lower than mine
			//reset mine, and take received as correct
			memcpy(instance_state->id, id, sizeof(uuid_t));

			switch(instance_state->op){
			case OP_AVG:
				aggregation_get_value(instance_state->val_def, &instance_state->current_val.val);
				instance_state->current_val.weight = 1;
				break;
			case OP_SUM:
				aggregation_get_value(instance_state->val_def, &instance_state->current_val.val);
				instance_state->current_val.weight = 0;
				break;
			case OP_COUNT:
				instance_state->current_val.val = 1;
				instance_state->current_val.weight = 0;
				break;
			default:
				break;
			}
			//process
			double value;
			double weight;

			memcpy(&value, payload, sizeof(double));
			memcpy(&weight, payload + sizeof(double), sizeof(double));
			push_sum_add_val(instance_state, value, weight);
		}else if(r == 0){
			//received id is mapping to my instance
			//process
			double value;
			double weight;

			memcpy(&value, payload, sizeof(double));
			memcpy(&weight, payload + sizeof(double), sizeof(double));
			push_sum_add_val(instance_state, value, weight);


		}//else received id is higher than mine, so ignore
	}
}

void* push_sum_single_init(void* args){
	proto_args* pargs = (proto_args*)args;

	protoId = pargs->protoID;
	reliable_proto = ((push_sum_attr*) pargs->protoAttr)->reliable_proto_id;
	discovery_proto = ((push_sum_attr*) pargs->protoAttr)->discovery_proto_id;

	queue_t* inBox = pargs->inBox;

	instance_state = NULL;

	queue_t_elem elem;

	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			push_sum_process_msg(&elem.data.msg, 1);

		}else if(elem.type == LK_TIMER){
			//received timer, ask from discovery a random node
			if(instance_state != NULL){
				push_sum_split(instance_state);
				request_destination();
				if(instance_state->timer.opt.max_rounds > 0){
					instance_state->timer.opt.max_rounds --;
				}else if(instance_state->timer.opt.max_rounds == 0){
					cancelTimer(&instance_state->timer.timer);
				}
			}else
				lk_log("PUSH SUM", "ERROR", "Could not find state to continue algorithm");

			free(elem.data.timer.payload);

		}else if(elem.type == LK_EVENT){
			//received some event, check who sent it?
			if(elem.data.event.proto_origin == reliable_proto && elem.data.event.notification_id == FAILED_DELIVERY){

				push_sum_process_msg((LKMessage*) elem.data.event.payload, 0);
				free(elem.data.event.payload);
			}else{
				lk_log("PUSH SUM", "WARNING", "Unknown event received");
			}
			//if reliable process it as message
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
							push_sum_init_agg(&instance_state, metainfo, 1, val);
							break;
						case OP_SUM:
							push_sum_init_agg(&instance_state, metainfo, 1, val);
							break;
						case OP_COUNT:
							push_sum_init_agg(&instance_state, metainfo, 1, 1);
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
						double result = instance_state->current_val.val / instance_state->current_val.weight;
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
					cancelTimer(&instance_state->timer.timer);
					free(instance_state);
					instance_state = NULL;
					destroy_send_lst();

				}else{
					lk_log("PUSH SUM", "WARNING", "Unknown request type");
				}

				if(elem.data.request.payload != NULL){
					free(elem.data.request.payload);
					elem.data.request.payload = NULL;
				}

			}else if(elem.data.request.request == REPLY){
				//random node to sent to from discovery, split val and weight and send it
				if(elem.data.request.proto_origin == discovery_proto && elem.data.request.request_type == DISC_REQ_GET_RND_NEIGH){

					int reply;
					memcpy(&reply, elem.data.request.payload, sizeof(int));
					if(reply ==  1){

						neigh* n = malloc(sizeof(neigh));

						void* payload = (elem.data.request.payload + sizeof(int));
						int len = elem.data.request.length - sizeof(int);

						if((len - sizeof(neigh)) >= 0){
							memcpy(n, payload, sizeof(neigh));

						}


						val v;
						void* info;
						unsigned short info_size = get_val_to_send(&v, &info);

						LKMessage msg;
						LKMessage_init(&msg, n->addr.data , protoId);

						LKMessage_addPayload(&msg, info, info_size);
						LKMessage_addPayload(&msg, (void*) &v.val, sizeof(double));
						LKMessage_addPayload(&msg, (void*) &v.weight, sizeof(double));

						dispatch(&msg);

						free(n);
						free(info);


					}else{
						//lk_log("APP", "DISCOVERY REPLY", "No neighbors");
					}

					free(elem.data.request.payload);
					elem.data.request.payload = NULL;

				}
			}else{
				lk_log("PUSH SUM", "WARNING", "LK Request, not a request nor a reply");
			}
		}else{
			lk_log("PUSH SUM", "WARNING", "Unknown element type popped");
		}
	}
}

