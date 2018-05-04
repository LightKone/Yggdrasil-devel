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

#include "gap_avg_bcast.h"

#define UND -50 // undefined integer value

//TODO define default vals

#define DEFAULT_LEVEL 50


#define TIMEOUT 2*1000*1000 //2 seconds

typedef void (*policy_function) ();

typedef enum status_{
	SELF,
	PEER,
	CHILD,
	PARENT,
	ROOT
}status;

typedef struct avg_val_{
	double sum;
	int count;
}avg_val;

typedef struct neighTable_{
	uuid_t neigh_id;
	WLANAddr neigh_addr;

	status st; //(self, peer, child, parent)
	int level; //distance to root
	avg_val val; //value

	struct neighTable_* next;
}neighTable;

typedef struct updateVector_{
	uuid_t neigh;
	avg_val val;
	int level;
	uuid_t parent;
	uuid_t root_id;
	int root_lvl;
	avg_val root_avg;
}updateVector;

static neighTable* neighsTable; //head of the list
static neighTable* self; //pointer to self in the neighs list
static neighTable* parent; //pointer to parent in the neighs list
static neighTable* root = NULL;

static short protoId;
static short discoveryId; //should send new neigh and failed neigh

static policy_function policy = NULL;


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
	//creates a table entry and returns n;
	//if(!neighs.contains(n))
	//neighs.add(n, s, l, w)
	//s = peer; l, w are defaults
	neighTable* entry = find_neigh(n);
	if(entry == NULL){
		entry = malloc(sizeof(neighTable));
		memcpy(entry->neigh_id, n, sizeof(uuid_t));
		memcpy(entry->neigh_addr.data, addr.data, WLAN_ADDR_LEN);
		entry->st = PEER;
		entry->val.count = 0;
		entry->val.sum = 0;
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

			free(torm);
			torm = NULL;
			return SUCCESS; //:)
		}else{
			while(it_neigh->next != NULL){
				if(uuid_compare(it_neigh->next->neigh_id, n) == 0){
					neighTable* torm = it_neigh->next;
					it_neigh->next = torm->next;
					torm->next = NULL;

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

static void update_root(uuid_t root_id, int root_lvl, avg_val root_avg) {
	if(root == NULL)
		root = malloc(sizeof(neighTable));

	memcpy(root->neigh_id, root_id, sizeof(uuid_t));
	root->level = root_lvl;
	root->val.count = root_avg.count;
	root->val.sum = root_avg.sum;
}

static void updateentry(uuid_t n, WLANAddr addr, avg_val* w, int l, uuid_t p, uuid_t root_id, int root_lvl, avg_val root_avg){
	//updates entry n with vals w and l
	//if p == self
	//n.status = child
	//?????
	neighTable* entry = find_neigh(n);
	if(entry == NULL){
		entry = malloc(sizeof(neighTable));
		memcpy(entry->neigh_addr.data, addr.data, WLAN_ADDR_LEN);
		memcpy(entry->neigh_id, n, sizeof(uuid_t));

		entry->st = PEER;

		entry->next = neighsTable;
		neighsTable = entry;
	}

	entry->level = l;
	entry->val.count = w->count;
	entry->val.sum = w->sum;

	if(uuid_compare(p, self->neigh_id) == 0){
		entry->st = CHILD;
	}else if(entry->st == CHILD){
		entry->st = PEER;
	}

	if(parent != NULL) {

		if(uuid_compare(n, parent->neigh_id) == 0) {
			update_root(root_id, root_lvl, root_avg);
		}
	}

#ifdef DEBUG
	char s[500];
	bzero(s, 500);
	char u[37];
	uuid_unparse(entry->neigh_id, u);
	double value =  entry->val.sum / entry->val.count;
	sprintf(s, "entry: %s status: %d value: %f = %f / %d", u, entry->st, value, entry->val.sum, entry->val.count);

	lk_log("GAP BCAST", "UPDATE ENTRY", s);
#endif
}


static void addVectorToMsg(LKMessage* msg, updateVector* v){
	LKMessage_addPayload(msg, (void*) v->neigh, sizeof(uuid_t));
	LKMessage_addPayload(msg, (void*) &v->val.sum, sizeof(double));
	LKMessage_addPayload(msg, (void*) &v->val.count, sizeof(int));
	LKMessage_addPayload(msg, (void*) &v->level, sizeof(int));
	if(v->parent != NULL) {
		LKMessage_addPayload(msg, (void*) v->parent, sizeof(uuid_t));
		if(v->root_id != NULL) {
			LKMessage_addPayload(msg, (void*) v->root_id, sizeof(uuid_t));
			LKMessage_addPayload(msg, (void*) &v->root_avg.sum, sizeof(double));
			LKMessage_addPayload(msg, (void*) &v->root_avg.count, sizeof(int));
			LKMessage_addPayload(msg, (void*) &v->root_lvl, sizeof(int));
		}
	}

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
			if(it_neigh != self) {
				if(parent != NULL) {
					if(it_neigh->level < parent->level){
						parent = it_neigh;

					}
				}else {
					parent = it_neigh;
				}
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

#ifdef DEBUG
	char s[500];
	bzero(s, 500);
	if(parent != NULL){
		char u[37];
		uuid_unparse(parent->neigh_id, u);
		sprintf(s, "Parent now is: %s my level: %d", u, self->level);
	}else{
		sprintf(s, "No parent yet my level: %d", self->level);
	}
	lk_log("GAP BCAST", "RESTORE TABLE INV", s);
#endif
}

static void updatevector(updateVector* v){
	v->level = self->level;
	memcpy(v->neigh, self->neigh_id, sizeof(uuid_t));
	if(parent != NULL)
		memcpy(v->parent, parent->neigh_id, sizeof(uuid_t));

	neighTable* it_neigh = neighsTable;
	avg_val val;
	val.sum = self->val.sum;
	val.count = self->val.count;

	while(it_neigh != NULL){

		if(it_neigh->st == CHILD){
			//agg it_neigh weight

			val.count += it_neigh->val.count;
			val.sum += it_neigh->val.sum;

#ifdef DEBUG
			char s[500];
			bzero(s, 500);
			char u[37];
			uuid_unparse(it_neigh->neigh_id, u);
			double value = it_neigh->val.sum / it_neigh->val.count;

			sprintf(s, "counting value: %f = %f / %d of child: %s", value, it_neigh->val.sum, it_neigh->val.count, u);
			lk_log("GAP BCAST", "UPDATE VECTOR", s);
#endif

		}

		it_neigh = it_neigh->next;
	}

	//agg self weight with agg it_neigh weight and put in v->weight

	v->val.sum = val.sum;
	v->val.count = val.count;

#ifdef DEBUG
	char s[500];
	bzero(s, 500);
	double value = v->val.sum / v->val.count;
	sprintf(s, "Updated vector value now is: %f = %f / %d", value, v->val.sum, v->val.count);
	lk_log("GAP BCAST", "UPDATE VECTOR", s);
#endif

	if(parent != NULL && parent->level < 0){
		parent->val.sum = v->val.sum;
		parent->val.count = v->val.count;
	}

	if(root != NULL) {

		memcpy(v->root_id, root->neigh_id, sizeof(uuid_t));
		v->root_avg.count = root->val.count;
		v->root_avg.sum = root->val.sum;
		v->root_lvl = root->level;
	}
}

//static short comp_vector(updateVector* v1, updateVector* v2){
//	return (v1->val.sum == v2->val.sum) && (v1->val.count == v2->val.count) && (v1->level == v2->level) && (uuid_compare(v1->neigh, v2->neigh) == 0) && (uuid_compare(v1->parent, v2->parent) == 0);
//}

static neighTable* addrootentry(uuid_t root_id){
	neighTable* entry = malloc(sizeof(neighTable));

	memcpy(entry->neigh_id, root_id, sizeof(uuid_t));
	entry->st = PARENT;
	entry->level = -1;
	entry->val.count = 0;
	entry->val.sum = 0;

	entry->next = neighsTable;
	neighsTable = entry;

	self->level = 0;

	return entry;
}

static void deserialize_vector(updateVector* other, LKMessage* msg) {

	void* payload = msg->data;
	short len = msg->dataLen;

	memcpy(other->neigh, payload, sizeof(uuid_t));
	payload += sizeof(uuid_t);
	memcpy(&other->val.sum, payload, sizeof(double));
	payload += sizeof(double);
	memcpy(&other->val.count, payload, sizeof(int));
	payload += sizeof(int);
	memcpy(&other->level, payload, sizeof(int));
	len -= (sizeof(uuid_t) + sizeof(double) + sizeof(int) + sizeof(int));

	if(len > 0){
		payload += sizeof(int);
		memcpy(other->parent, payload, sizeof(uuid_t));
		len -= sizeof(uuid_t);
		if(len > 0) {
			payload += sizeof(uuid_t);
			memcpy(other->root_id, payload, sizeof(uuid_t));
			payload += sizeof(uuid_t);
			memcpy(&other->root_avg.sum, payload, sizeof(double));
			payload += sizeof(double);
			memcpy(&other->root_avg.count, payload, sizeof(int));
			payload += sizeof(int);
			memcpy(&other->root_lvl, payload, sizeof(int));
		}
	}
}

void * gap_avg_bcast_init(void* args){

	proto_args* pargs = (proto_args*)args;

	protoId = pargs->protoID;
	gap_args* gargs = ((gap_args*) pargs->protoAttr);
	discoveryId = gargs->discoveryId;

	policy = NULL; //TODO get_policy(gargs->policy_type);

	queue_t* inBox = pargs->inBox;

	queue_t_elem elem;

	double val;
	aggregation_get_value(DEFAULT, &val);

	WLANAddr addr;
	setMyAddr(&addr);
	self = newentry(pargs->myuuid, addr);
	self->val.sum = val;
	self->val.count = 1;

	if(gargs->root){
		uuid_t root_id;
		genUUID(root_id);
		parent = addrootentry(root_id);
		root = parent;

#ifdef DEBUG
		lk_log("GAP BCAST", "DEBUG", "I am leader");
#endif
	}else{
#ifdef DEBUG
		lk_log("GAP BCAST", "DEBUG", "I am not leader");
#endif
		parent = NULL;

	}
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
			deserialize_vector(&other, &elem.data.msg);

			updateentry(other.neigh, elem.data.msg.srcAddr, &other.val, other.level, other.parent, other.root_id, other.root_lvl, other.root_avg);

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



			}else if(elem.data.event.proto_origin == 401){
				double newval;
				memcpy(&newval, elem.data.event.payload, sizeof(double));
				self->val.sum = newval;
			}
			if(elem.data.event.payload != NULL){
				free(elem.data.event.payload);
				elem.data.event.payload = NULL;
			}
		}else if(elem.type == LK_REQUEST){
			//APP process (aggregation_gen_interface)
			if(elem.data.request.request == REQUEST){
				//got a request to do something

				if(elem.data.request.request_type == AGG_INIT){
					//initiate some aggregation (avg, sum, count)

				}else if(elem.data.request.request_type == AGG_GET){
					//what is the estimated value?

					double myval = self->val.sum;
					double result;
					if(root != NULL)
						result = root->val.sum / root->val.count;
					else
						result = 0;

					elem.data.request.request = REPLY;
					elem.data.request.proto_dest = elem.data.request.proto_origin;
					elem.data.request.proto_origin = protoId;
					elem.data.request.request_type = AGG_VAL;

					if(elem.data.request.payload != NULL){
						free(elem.data.request.payload);
						elem.data.request.payload = NULL;
					}

					elem.data.request.payload = malloc(sizeof(double)+sizeof(double));
					memcpy(elem.data.request.payload, &myval, sizeof(double));
					memcpy(elem.data.request.payload + sizeof(double), &result, sizeof(double));

					elem.data.request.length = sizeof(double) + sizeof(double);

					deliverReply(&elem.data.request);

					free(elem.data.request.payload);
					elem.data.request.payload = NULL;

				}else if(elem.data.request.request_type == AGG_TERMINATE){
					//reset state


				}else{
					lk_log("GAP BCAST", "WARNING", "Unknown request type");
				}

				if(elem.data.request.payload != NULL){
					free(elem.data.request.payload);
					elem.data.request.payload = NULL;
				}

			}else if(elem.data.request.request == REPLY){
				lk_log("GAP BCAST", "WARNING", "LK Request, got an unexpected reply");
			}else{
				lk_log("GAP BCAST", "WARNING", "LK Request, not a request nor a reply");
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
		updatevector(newvector);

		if(newnode != NULL){
#ifdef DEBUG
			char s[500];
			bzero(s, 500);
			char u[37];
			uuid_unparse(newnode->neigh_id, u);
			sprintf(s, "entry: %s status: %d", u, newnode->st);

			lk_log("GAP BCAST", "ESTABLISH CONNECTION", s);
#endif
			sendUpdateVector(newnode, newvector);
			newnode = NULL;
		}


		if(timeout == 1){

#ifdef DEBUG
			char s[500];
			bzero(s, 500);
			char u[37];
			uuid_unparse(newvector->parent, u);
			double value = newvector->val.sum / newvector->val.count;
			sprintf(s, "parent: %s level: %d weight: %f", u, newvector->level, value);

			lk_log("GAP BCAST", "BROADCAST", s);
#endif
			broadcastUpdateVector(newvector);
			if(vector != NULL){
				free(vector);
			}
			vector = newvector;
			newvector = NULL;
			timeout = 0;

		}else{

			free(newvector);
			newvector = NULL;
		}

	}

}

