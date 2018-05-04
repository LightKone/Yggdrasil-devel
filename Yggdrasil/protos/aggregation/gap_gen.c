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

#include "gap_gen.h"

#define UND -50 // undefined integer value

//TODO define default vals

#define DEFAULT_LEVEL 50


#define TIMEOUT 2*1000*1000 //2 seconds

typedef void (*policy_function) ();

typedef enum status_{
	SELF,
	PEER,
	CHILD,
	PARENT
}status;

typedef struct neighTable_{
	uuid_t neigh_id;
	WLANAddr neigh_addr;

	status st; //(self, peer, child, parent)
	int level; //distance to root
	void* weight; //value

	struct neighTable_* next;
}neighTable;

typedef struct updateVector_{
	uuid_t neigh;
	void* weight;
	int level;
	uuid_t parent;
}updateVector;

static neighTable* neighsTable; //head of the list
static neighTable* self; //pointer to self in the neighs list
static neighTable* parent; //pointer to parent in the neighs list

static short protoId;
static short discoveryId; //should send new neigh and failed neigh

static policy_function policy = NULL;

static aggregation_function agg_func = NULL;
static value_size_t val_size;
static void* neutral_val = NULL;

static neighTable* find_neigh(uuid_t n){
	//find n in table

	neighTable* it_neigh = neighsTable;

	while(it_neigh != NULL){
		if(uuid_compare(it_neigh->neigh_id, n) == 0)
			return it_neigh;
		it_neigh = it_neigh->next;
	}


	return NULL;
}

static neighTable* newentry(uuid_t n, WLANAddr addr){
	//creates a table entre and returns n;
	//if(!neighs.contains(n))
	//neighs.add(n, s, l, w)
	//s = peer; l, w are defaults
	neighTable* entry = find_neigh(n);
	if(entry == NULL){
		entry = malloc(sizeof(neighTable));
		memcpy(entry->neigh_id, n, sizeof(uuid_t));
		memcpy(entry->neigh_addr.data, addr.data, WLAN_ADDR_LEN);
		entry->st = PEER;
		entry->weight = malloc(val_size);
		memcpy(entry->weight, neutral_val, val_size);
		entry->level = DEFAULT_LEVEL;
		entry->next = neighsTable;
		neighsTable = entry;
	}

	return entry;
}

static short removeentry(uuid_t n){
	//remove n from table if exists
	//if(neighs.contains(n))
	//neighs.rem(n)
	//return SUCCESS
	neighTable* it_neigh = neighsTable;
	if(it_neigh != NULL){
		if(uuid_compare(it_neigh->neigh_id, n) == 0){
			neighTable* torm = it_neigh;
			neighsTable = it_neigh->next;
			torm->next = NULL;
			free(torm->weight);
			torm->weight = NULL;
			free(torm);
			torm = NULL;
			return SUCCESS; //:)
		}else{
			while(it_neigh->next != NULL){
				if(uuid_compare(it_neigh->next->neigh_id, n) == 0){
					neighTable* torm = it_neigh->next;
					it_neigh->next = torm->next;
					torm->next = NULL;
					free(torm->weight);
					torm->weight = NULL;
					free(torm);
					torm = NULL;
					return SUCCESS; //:)
				}
				it_neigh = it_neigh->next;
			}
		}
	}


	return FAILED;
}

static void updateentry(uuid_t n, WLANAddr addr, void* w, int l, uuid_t p){
	//updates entry n with vals w and l
	//if p == self
	//n.status = child
	//?????
	neighTable* entry = find_neigh(n);
	if(entry == NULL){
		entry = malloc(sizeof(neighTable));
		memcpy(entry->neigh_addr.data, addr.data, WLAN_ADDR_LEN);
		memcpy(entry->neigh_id, n, sizeof(uuid_t));
		entry->weight = malloc(val_size);
		entry->st = PEER;

		entry->next = neighsTable;
		neighsTable = entry;
	}

	entry->level = l;
	memcpy(entry->weight, w, val_size);

	if(uuid_compare(p, self->neigh_id) == 0){
		entry->st = CHILD;
	}else if(entry->st == CHILD){
		entry->st = PEER;
	}

	char s[500];
	bzero(s, 500);
	char u[37];
	uuid_unparse(entry->neigh_id, u);
	double value;
	memcpy(&value, entry->weight, sizeof(double));
	sprintf(s, "entry: %s status: %d value: %f", u, entry->st, value);

	lk_log("GAP", "UPDATE ENTRY", s);
}

//static int get_level(uuid_t n){
//	neighTable* neigh = find_neigh(n);
//
//	if(neigh == NULL){
//		return UND;
//	}else{
//		return neigh->level;
//	}
//
//}
//
//static neighTable* get_parent(){
//	return parent;
//}

static void addVectorToMsg(LKMessage* msg, updateVector* v){
	LKMessage_addPayload(msg, (void*) v->neigh, sizeof(uuid_t));
	LKMessage_addPayload(msg, (void*) v->weight, val_size);
	LKMessage_addPayload(msg, (void*) &v->level, sizeof(int));
	if(v->parent != NULL)
		LKMessage_addPayload(msg, (void*) v->parent, sizeof(uuid_t));
}

static void sendUpdateVector(neighTable* n, updateVector* v){
	//send message (update vector) to node n
	LKMessage msg;
	LKMessage_init(&msg, n->neigh_addr.data, protoId);

	addVectorToMsg(&msg, v);

	dispatch(&msg);

}

static void broadcastUpdateVector(updateVector* v){
	//broadcast update vector

	LKMessage msg;
	LKMessage_initBcast(&msg, protoId);

	addVectorToMsg(&msg, v);

	dispatch(&msg);
}

static void restoreTableInvariant(){
	//Ensures neighTable invariant
	//each node is associated with at most one row //assume by add and rem
	//exactly one row has status self, and node there is self //assume by add and rem

	//if neighs.size > 1 then neighs.contains(parent) == true
	if(neighsTable->next != NULL){

		neighTable* it_neigh = neighsTable;
		parent = it_neigh;
		while(it_neigh != NULL){

			if(it_neigh->st == PARENT)
				it_neigh->st = PEER;

			//parent has minimal level among other entries
			if(it_neigh != self && it_neigh->level < parent->level){
				parent = it_neigh;
			}

			it_neigh = it_neigh->next;
		}

		parent->st = PARENT;
		//level of parent is one less than the level of parent
		self->level = parent->level + 1;


	}
	//could ensure different policies
	if(policy != NULL){
		policy();
	}

	char s[500];
	bzero(s, 500);
	if(parent != NULL){
		char u[37];
		uuid_unparse(parent->neigh_id, u);
		sprintf(s, "Parent now is: %s my level: %d", u, self->level);
	}else{
		sprintf(s, "No parent yet my level: %d", self->level);
	}
	lk_log("GAP", "RESTORE TABLE INV", s);
}

static void updatevector(updateVector* v){
	v->level = self->level;
	memcpy(v->neigh, self->neigh_id, sizeof(uuid_t));
	if(parent != NULL)
		memcpy(v->parent, parent->neigh_id, sizeof(uuid_t));

	neighTable* it_neigh = neighsTable;
	void* val = malloc(val_size);
	memcpy(val, neutral_val, val_size);

	while(it_neigh != NULL){

		if(it_neigh->st == CHILD){
			//agg it_neigh weight

			agg_func(val, it_neigh->weight, val);

			char s[500];
			bzero(s, 500);
			char u[37];
			uuid_unparse(it_neigh->neigh_id, u);
			double value;
			memcpy(&value, it_neigh->weight, sizeof(double));
			sprintf(s, "counting value: %f of child: %s", value, u);
			lk_log("GAP", "UPDATE VECTOR", s);

		}

		it_neigh = it_neigh->next;
	}

	//agg self weight with agg it_neigh weight and put in v->weight
	agg_func(self->weight, val ,v->weight);

	char s[500];
	bzero(s, 500);
	double value;
	memcpy(&value, v->weight, sizeof(double));
	sprintf(s, "Updated vector value now is: %f", value);
	lk_log("GAP", "UPDATE VECTOR", s);
}

static short comp_vector(updateVector* v1, updateVector* v2){
	return (aggregation_valueDiff(v1->weight,v2->weight) == 0) && (v1->level == v2->level) && (uuid_compare(v1->neigh, v2->neigh) == 0) && (uuid_compare(v1->parent, v2->parent) == 0);
}

static void addrootentry(){
	neighTable* entry = malloc(sizeof(neighTable));

	genUUID(entry->neigh_id);
	entry->st = PARENT;
	entry->level = -1;
	entry->weight = malloc(val_size);
	memcpy(entry->weight, neutral_val, val_size);

	entry->next = neighsTable;
	neighsTable = entry;

	parent = entry;

	self->level = 0;
}

static void deserialize_vector(updateVector* other, LKMessage* msg) {

	void* payload = msg->data;
	short len = msg->dataLen;

	memcpy(other->neigh, payload, sizeof(uuid_t));
	payload += sizeof(uuid_t);
	memcpy(other->weight, payload, val_size);
	payload += val_size;
	memcpy(&other->level, payload, sizeof(int));
	len -= (sizeof(uuid_t) + val_size + sizeof(int));

	if(len > 0){
		payload += sizeof(int);
		memcpy(other->parent, payload, sizeof(uuid_t));
	}
}

void * gap_init(void* args){

	proto_args* pargs = (proto_args*)args;

	protoId = pargs->protoID;
	gap_args* gargs = ((gap_args*) pargs->protoAttr);
	discoveryId = gargs->discoveryId;

	policy = NULL; //TODO get_policy(gargs->policy_type);

	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;

	neighTable* newnode = NULL;

	updateVector* vector = NULL;
	short timeout = 0;

	LKTimer timer;
	LKTimer_init(&timer, protoId, protoId);
	LKTimer_set(&timer, TIMEOUT, TIMEOUT);

	setupTimer(&timer);

	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			//process (update, n, w, l, p)
			//deserialze msg to get updatevector of sending node
			updateVector other;
			other.weight = malloc(val_size);
			deserialize_vector(&other, &elem.data.msg);

			updateentry(other.neigh, elem.data.msg.srcAddr, other.weight, other.level, other.parent);

			free(other.weight);
			other.weight = NULL;

		}else if(elem.type == LK_TIMER){
			//process (timeout)
			timeout = 1;

		}else if(elem.type == LK_EVENT){
			if(elem.data.event.proto_origin == discoveryId){

				if(elem.data.event.notification_id == DISCOV_FT_LB_EV_NEIGH_DOWN){
					//process (fail, n)
					uuid_t n;
					//extract n from event payload
					void* data = elem.data.event.payload;
					memcpy(n, data, sizeof(uuid_t));

					removeentry(n);
				}else if(elem.data.event.notification_id == DISCOV_FT_LB_EV_NEIGH_UP){
					//process (new, n)
					uuid_t n;
					WLANAddr addr;
					//extract n and addr from event payload
					void* data = elem.data.event.payload;
					memcpy(n, data, sizeof(uuid_t));
					memcpy(addr.data, data+sizeof(uuid_t), WLAN_ADDR_LEN);

					newnode = newentry(n, addr);

				}else{
					//undefined
				}

				if(elem.data.event.payload != NULL){
					free(elem.data.event.payload);
					elem.data.event.payload = NULL;
				}

			}
		}else if(elem.type == LK_REQUEST){
			//APP process (aggregation_gen_interface)
			if(elem.data.request.request == REQUEST){
				//got a request to do something

				if(elem.data.request.request_type == AGG_INIT){
					//initiate some aggregation (avg, sum, count)
					agg_meta* metainfo = aggregation_deserialize_agg_meta(elem.data.request.length, elem.data.request.payload);
					val_size = aggregation_gen_function_getValsize(metainfo->op);
					void* val = malloc(val_size);
					if(aggregation_get_value_for_op(metainfo->val, metainfo->op, val) == FAILED){
						elem.data.request.request = REPLY;
						elem.data.request.request_type = AGG_FAILED;
						deliverReply(&elem.data.request);

					}else{
						switch(metainfo->op){
						case OP_MAX:
						case OP_MIN:
						case OP_AVG:
						case OP_SUM:
						case OP_COUNT:
							agg_func = aggregation_gen_function_get(metainfo->op);

							neutral_val = realloc(neutral_val, val_size);
							aggregation_gen_function_setNeutral(metainfo->op, neutral_val);

							//neighsTable = malloc(sizeof(neighTable));
							WLANAddr addr;
							setMyAddr(&addr);
							self = newentry(pargs->myuuid, addr);
							memcpy(self->weight, val, val_size);

							free(val);
							val = NULL;

							if(gargs->root){
								addrootentry();
								lk_log("GAP", "DEBUG", "I am leader");
							}else{
								lk_log("GAP", "DEBUG", "I am not leader");
								parent = NULL;
							}

							//							vector = malloc(sizeof(updateVector));
							//							vector->weight = malloc(val_size);
							//							updatevector(vector);

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

					double result;
					memcpy(&result, self->weight, sizeof(double));

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

				}else if(elem.data.request.request_type == AGG_TERMINATE){
					//reset state


				}else{
					lk_log("GAP", "WARNING", "Unknown request type");
				}

				if(elem.data.request.payload != NULL){
					free(elem.data.request.payload);
					elem.data.request.payload = NULL;
				}

			}else if(elem.data.request.request == REPLY){
				lk_log("GAP", "WARNING", "LK Request, got an unexpected reply");
			}else{
				lk_log("GAP", "WARNING", "LK Request, not a request nor a reply");
			}

			if(elem.data.request.payload != NULL){
				free(elem.data.request.payload);
				elem.data.request.payload = NULL;
			}
		}else{
			//undefined (error/warning?)
		}

		restoreTableInvariant();
		updateVector* newvector = malloc(sizeof(updateVector));
		newvector->weight = malloc(val_size);
		updatevector(newvector);

		if(newnode != NULL){

			char s[500];
			bzero(s, 500);
			char u[37];
			uuid_unparse(newnode->neigh_id, u);
			sprintf(s, "entry: %s status: %d", u, newnode->st);

			lk_log("GAP", "ESTABLISH CONNECTION", s);

			sendUpdateVector(newnode, newvector);
			newnode = NULL;
		}


		if((vector == NULL || !comp_vector(vector, newvector)) && timeout == 1){

			char s[500];
			bzero(s, 500);
			char u[37];
			uuid_unparse(newvector->parent, u);
			double value;
			memcpy(&value, newvector->weight, sizeof(double));
			sprintf(s, "parent: %s level: %d weight: %f", u, newvector->level, value);

			lk_log("GAP", "BROADCAST", s);

			broadcastUpdateVector(newvector);
			if(vector != NULL){
				free(vector->weight);
				vector->weight = NULL;
				free(vector);
			}
			vector = newvector;
			newvector = NULL;
			timeout = 0;

		}else{
			free(newvector->weight);
			newvector->weight = NULL;
			free(newvector);
			newvector = NULL;
		}

	}

}

