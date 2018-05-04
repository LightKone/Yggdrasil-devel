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

#include "drg_single.h"

#define transmission_time 5*1000*1000 //5 seconds

#define group_time_out 2*1000*1000 //2 seconds

typedef struct drg_opt_ {
	short max_rounds; //max rounds of the algorithm //TODO not implemented in first version
	short prob; //probability to become leader
}drg_opt;

typedef enum mode_type_{
	IDLE,
	MEMBER,
	LEADER
}mode_type;

typedef enum msg_type_{
	GCM,
	JACK,
	GAM,
}msg_type;

typedef struct group_participants_{
	uuid_t participant_id;
	double val;
	struct group_participants_* next;
}group_participants;

typedef struct group_info_{
	uuid_t group_id;

	double group_val;
	short n_participants; //for leader mode
	group_participants* participants;

	LKTimer* timer; //group life time

	struct group_info_* next;
}group_info;

typedef struct state_{
	agg_gen_op op;
	agg_val val_def;
	drg_opt opt;

	//aggregation_function func; //possible optimization

	double input_val; //why not?
	double current_estimation;

	mode_type mode;
	group_info* group;

	uuid_t id;

	LKTimer timer; //to start a new round

}state;


static short protoId;

static short discovery_proto;
static short reliable_proto;

static state* instance_state; //

static unsigned int round = 1;

static void destroy_groups(state* sta) {

	while(sta->group != NULL){
		group_info* torm = sta->group;
		while(torm->participants != NULL){
			group_participants* p = torm->participants;
			torm->participants = p->next;
			p->next = NULL;
			free(p);
			p = NULL;

		}

		sta->group = torm->next;
		torm->next = NULL;
		if(torm->timer != NULL){
			cancelTimer(torm->timer);
			free(torm->timer);
			torm->timer = NULL;
		}
		free(torm);
		torm = NULL;
	}

}

static void drg_add_opt(drg_opt* opt, drg_opt_code code, short val){
	switch(code){
	case PROB:
		opt->prob = val;
		break;
	default:
		break;
	}
}

static void drg_extract_opt(drg_opt* opt, void* meta_opt, unsigned short meta_opt_size){
	void* tmp = meta_opt;
	short opt_code;
	short opt_val;

	while(meta_opt_size >= sizeof(short) + sizeof(short)){
		memcpy(&opt_code, tmp, sizeof(short));
		tmp += sizeof(short);
		memcpy(&opt_val, tmp, sizeof(short));
		tmp += sizeof(short);

		drg_add_opt(opt, opt_code, opt_val);

		meta_opt_size -= sizeof(short) + sizeof(short);
	}

}

static unsigned short drg_get_info(state* sta, void** info){
	*info = malloc(sizeof(agg_gen_op) + sizeof(agg_val));

	memcpy(*info, &(sta->op), sizeof(agg_gen_op));
	memcpy(*info + sizeof(agg_gen_op), &(sta->val_def), sizeof(agg_val));

	return sizeof(agg_gen_op) + sizeof(agg_val);
}

static void drg_set_timer_info(state* sta, LKTimer* timer){
	void* info;
	unsigned short info_size = drg_get_info(sta, &info);

	LKTimer_addPayload(timer, info, info_size);
	free(info);
}

static void drg_init_agg(state** sta, agg_meta* metainfo, double val){
	short init = 0;
	if(*sta == NULL){
		*sta = malloc(sizeof(state));
		init = 1;
	}

	if(init){

		(*sta)->input_val = val;
		(*sta)->current_estimation = val;
		(*sta)->op = metainfo->op;
		(*sta)->val_def = metainfo->val;
		(*sta)->opt.prob = 50;

		genUUID((*sta)->id);

		if(metainfo->opt != NULL){
			drg_extract_opt(&((*sta)->opt), metainfo->opt, metainfo->opt_size);
		}

		LKTimer_init(&((*sta)->timer), protoId, protoId);
		LKTimer_set(&((*sta)->timer), transmission_time, transmission_time);

		drg_set_timer_info(*sta, &(*sta)->timer);


		setupTimer(&((*sta)->timer));

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

static void fill_instance_id(LKMessage* msg, state* sta, group_info* info, msg_type type){
	LKMessage_addPayload(msg, (void*) sta->id, sizeof(uuid_t));
	LKMessage_addPayload(msg, (void*) &sta->op, sizeof(agg_gen_op));
	LKMessage_addPayload(msg, (void*) &sta->val_def, sizeof(agg_val));

	LKMessage_addPayload(msg, (void*) &sta->opt.prob, sizeof(short));

	LKMessage_addPayload(msg, (void*) &type, sizeof(msg_type));
	LKMessage_addPayload(msg, (void*) info->group_id, sizeof(uuid_t));

}

static void drg_add_group_info(state* sta, group_info* info){

	info->next = sta->group;
	sta->group = info;

}

static void group_add_participant(group_info* info, group_participants* participant){

	if(info->participants == NULL){
		info->participants = participant;
	}else{
		group_participants* tmp = info->participants;
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = participant;
	}

	info->n_participants ++;
}

static void drg_send_leader_msg(state* sta, uuid_t myid) {
	group_info* info = malloc(sizeof(group_info));
	genUUID(info->group_id);

	info->group_val = sta->current_estimation;
	info->n_participants = 0;

	group_participants* participant = malloc(sizeof(group_participants));
	participant->val = sta->current_estimation;
	memcpy(participant->participant_id, myid, sizeof(uuid_t));
	participant->next = NULL;

	group_add_participant(info, participant);

	info->timer = malloc(sizeof(LKTimer));

	LKTimer_init_with_uuid(info->timer,info->group_id, protoId, protoId);
	LKTimer_set(info->timer, group_time_out, 0);

	drg_set_timer_info(sta, info->timer);

	setupTimer(info->timer);

	info->next = NULL;

	drg_add_group_info(sta, info);
	sta->mode = LEADER;

	LKMessage msg;
	LKMessage_initBcast(&msg,protoId);

	fill_instance_id(&msg, sta, info, GCM);

	dispatch(&msg);

	char s[500];
	bzero(s, 500);
	char u[37];
	uuid_unparse(info->group_id, u);
	sprintf(s, "I am leader of group %s for round %d", u, round);
	lk_log("DRG", "LEADER", s);
}


static void drg_send_announcement_msg(state* sta, group_info* group){

	//PROCESS GROUP INFO

	LKMessage msg;
	LKMessage_initBcast(&msg,protoId);

	fill_instance_id(&msg, sta, group, GAM);

	LKMessage_addPayload(&msg, (void*) &group->group_val, sizeof(double));
	LKMessage_addPayload(&msg, (void*) &group->n_participants, sizeof(short));

	group_participants* ps = group->participants;
	while(ps != NULL){
		LKMessage_addPayload(&msg, (void*) &ps->participant_id, sizeof(uuid_t));
		ps = ps->next;
	}

	dispatch(&msg);

	sta->current_estimation = group->group_val;
	sta->mode = IDLE;
}

static void drg_process_GCM(void* payload, state* sta, WLANAddr* leader_addr, uuid_t myid){
	if(sta->mode == IDLE){
		//retrive group id and send join message with value
		group_info* info = malloc(sizeof(group_info));
		memcpy(info->group_id, payload, sizeof(uuid_t));

		info->next = NULL;
		info->timer = NULL;

		drg_add_group_info(sta, info);

		sta->mode = MEMBER;

		//SEND JACK
		LKMessage msg;
		LKMessage_init(&msg, leader_addr->data, protoId);

		fill_instance_id(&msg, sta, info, JACK);

		LKMessage_addPayload(&msg, (void*) myid, sizeof(uuid_t));
		LKMessage_addPayload(&msg, (void*) &sta->current_estimation, sizeof(double));

		dispatch(&msg);

		char s[500];
		bzero(s, 500);
		char u[37];
		uuid_unparse(info->group_id, u);
		sprintf(s, "I want to be member of group %s for round %d", u, round);
		lk_log("DRG", "MEMBER", s);

	}
	//else ignore
}

static group_info* drg_get_group(state* sta, uuid_t group_id){
	group_info* group = sta->group;

	while(group != NULL && uuid_compare(group->group_id, group_id) != 0){
		group = group->next;
	}

	return group;
}

static void drg_process_JACK(void* payload, state* sta){
	if(sta->mode == LEADER){
		//retrive group id, find it (should be the first one) add info about joining node
		uuid_t group_id;
		memcpy(group_id, payload, sizeof(uuid_t));
		payload += sizeof(uuid_t);

		group_info* group = drg_get_group(sta, group_id);

		if(group != NULL){
			if(group->timer != NULL){
				group_participants* participant = malloc(sizeof(group_participants));
				memcpy(participant->participant_id, payload, sizeof(uuid_t));
				payload += sizeof(uuid_t);
				memcpy(&participant->val, payload, sizeof(double));
				participant->next = NULL;
				group_add_participant(sta->group, participant);

				char s[500];
				bzero(s, 500);
				char u[37];
				char u2[37];
				uuid_unparse(participant->participant_id, u2);
				uuid_unparse(group->group_id, u);
				sprintf(s, "%s is member of my group %s for round %d", u2 , u, round);
				lk_log("DRG", "LEADER", s);

			}else{
				lk_log("DRG", "WARNING", "Received JACK for group that has already ended");
			}
		}else{
			lk_log("DRG", "WARNING", "Received JACK for unexisting group");
		}
	}
	//else ignore
}

static void drg_process_GAM(void* payload, state* sta, uuid_t myid){
	if(sta->mode == MEMBER){
		//retrive group id, check if valid, update value

		uuid_t group_id;
		memcpy(group_id, payload, sizeof(uuid_t));
		payload += sizeof(uuid_t);

		group_info* group = drg_get_group(sta, group_id);

		if(group != NULL){
			memcpy(&group->group_val, payload, sizeof(double));
			payload += sizeof(double);
			memcpy(&group->n_participants, payload, sizeof(short));
			payload += sizeof(short);
			int i;
			for(i = 0; i < group->n_participants; i++){
				uuid_t participant_id;
				memcpy(participant_id, payload, sizeof(uuid_t));
				if(uuid_compare(participant_id, myid) == 0){
					sta->current_estimation = group->group_val;

					char s[500];
					bzero(s, 500);
					sprintf(s, "updated estimation for round %d: %f", round, sta->current_estimation);
					lk_log("DRG", "ESTIMATION", s);

					break;
				}

				payload += sizeof(uuid_t);
			}
		}

		round ++;
		sta->mode = IDLE;
	}
	//else ignore
}

static void drg_calculate_group_val(group_info* group) {
	double group_val = 0;
	group_participants* ps = group->participants;
	while(ps != NULL){
		group_val += ps->val;
		ps = ps->next;
	}

	group->group_val = group_val / group->n_participants; //CALCULATES GROUP AVERAGE; SHOULD GENERALIZE TO OBTAIN MAX and MIN
}


void* drg_single_init(void* args){
	proto_args* pargs = (proto_args*)args;

	protoId = pargs->protoID;
	discovery_proto = ((drg_attr*) pargs->protoAttr)->discovery_proto_id;
	reliable_proto = ((drg_attr*) pargs->protoAttr)->reliable_proto_id;

	queue_t* inBox = pargs->inBox;

	instance_state = NULL;

	queue_t_elem elem;


	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){

			//to what instance does this message belong?
			void* payload = elem.data.msg.data;
			uuid_t instance_id;
			agg_gen_op op;
			agg_val valdef;
			short prob;

			memcpy(instance_id, payload, sizeof(uuid_t));
			payload += sizeof(uuid_t);

			memcpy(&op, payload, sizeof(agg_gen_op));
			payload += sizeof(agg_gen_op);

			memcpy(&valdef, payload, sizeof(agg_val));
			payload += sizeof(agg_val);

			memcpy(&prob, payload, sizeof(short));
			payload += sizeof(short);

			//state* sta = find_instance(instance_id, op, valdef);
			state* sta = instance_state;
			if(sta == NULL){
				double val;
				agg_meta* meta = malloc(sizeof(agg_meta));
				meta->op = op;
				meta->val = valdef;
				aggregation_meta_add_opt(meta, PROB, prob);

				if(aggregation_get_value(meta->val, &val) == SUCCESS){
					drg_init_agg(&instance_state, meta, val);
					//memcpy(instance_state->id, instance_id, sizeof(uuid_t)
				}else{
					lk_log("DRG", "ERROR", "Unable to get value");
					exit(-1);
				}
				free(meta);
				meta = NULL;
			}

			//find what type of message is this
			msg_type type;
			memcpy(&type, payload, sizeof(msg_type));
			payload += sizeof(msg_type);

			switch(type){
			case GCM:
				drg_process_GCM(payload, sta, &elem.data.msg.srcAddr, pargs->myuuid);
				break;
			case JACK:
				drg_process_JACK(payload, sta);
				break;
			case GAM:
				drg_process_GAM(payload, sta, pargs->myuuid);
				break;
			default:
				lk_log("DRG", "WARNING", "Unknown message type");
				break;
			}
			//to what round?

		}else if(elem.type == LK_TIMER){

			//got a timer, to what instance does it belong?
			state* sta = instance_state;
			switch(sta->mode){
			case LEADER:
				//check if right timer
				if(uuid_compare(sta->timer.id, elem.data.timer.id) != 0){
					//time to send GAM
					group_info* group = drg_get_group(sta, elem.data.timer.id);
					if(group != NULL){
						if(group->timer != NULL){
							drg_calculate_group_val(group);
							drg_send_announcement_msg(sta, group);

							char s[500];
							bzero(s, 500);
							sprintf(s, "leader calculated for round %d: %f", round, sta->current_estimation);
							lk_log("DRG", "ESTIMATION", s);

							round ++;

							free(group->timer);
							group->timer = NULL;
						}else{
							lk_log("DRG", "WARNING", "Got a timer to a round that has already ended");
						}
					}else{
						lk_log("DRG", "WARNING", "Got an unknown timer to process when LEADER");
					}
				}
				break;
			case IDLE:
				//become leader?
				if(uuid_compare(sta->timer.id, elem.data.timer.id) == 0){
					if((rand()%100) < sta->opt.prob){
						//become leader :)

						drg_send_leader_msg(sta, pargs->myuuid);
					}//else continue being idle

				}else{
					lk_log("DRG", "WARNING", "Got an unexcepted timer when IDLE");
				}
				break;
			default:
				//nothing to do (ignore)
				break;
			}

			//what do we need to do? become group leader? timeout on join message?

			free(elem.data.timer.payload);

		}else if(elem.type == LK_EVENT){
			//received some event, check who sent it?
			//TODO USE EVENT TO DEAL WITH FAULTS
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
							if(instance_state == NULL){
								drg_init_agg(&instance_state, metainfo, val);
								drg_send_leader_msg(instance_state, pargs->myuuid);
							}
							break;
						case OP_MIN:
							break;
						case OP_MAX:
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
						double result = instance_state->current_estimation;
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
					destroy_groups(instance_state);
					free(instance_state);
					instance_state = NULL;


				}else{
					lk_log("DRG", "WARNING", "Unknown request type");
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
				lk_log("DRG", "WARNING", "Got an unexcepted reply");
			}else{
				lk_log("DRG", "WARNING", "LK Request, not a request nor a reply");
			}
		}else{
			lk_log("DRG", "WARNING", "Unknown element type popped");
		}
	}
}

