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


#include "multi_root_aggregation.h"

//PROTO CONSTANTS
#define BEACON_INTERVAL 2*1000*1000

typedef enum neigh_state_{
	ACTIVE,
	PASSIVE
}neigh_state;

//PROTO STRUCTURES
typedef struct neigh_{
	uuid_t id;
	uuid_t tree_id;
	double val;
	unsigned short level;
	neigh_state status;
	struct neigh_* next;
}neigh;

typedef struct trees_{
	uuid_t tree;
	time_t last_seen;
	struct trees_* next;
}trees;


//YGGDRASIL STATE VARS:
static short protoId;

static short fault_detector_id;

//STATE:
static trees* trees_seen;
static trees* my_tree;

//static uuid_t tree_id;

static neigh* neigh_lst = NULL;

static neigh* self;
static neigh* parent;

static double aggval;

//AUX FUNCTIONS:

static trees* find_tree(uuid_t tree_id){
	trees* it = trees_seen;

	while(it != NULL){
		if(uuid_compare(it->tree, tree_id) == 0){
			return it;
		}
		it = it->next;
	}
	return NULL;
}

static trees* create_tree(uuid_t tree_id){
	trees* tree = malloc(sizeof(trees));

	memcpy(tree->tree, tree_id, sizeof(uuid_t));

	tree->next = trees_seen;
	trees_seen = tree;

	return tree;
}

static neigh* find_nei(uuid_t nei_id){
	neigh* it = neigh_lst;

	while(it != NULL){
		if(uuid_compare(it->id, nei_id) == 0)
			return it;

		it = it->next;
	}
	return NULL;
}

static short rm_neigh(uuid_t nei_id){
	if(neigh_lst != NULL){
		if(uuid_compare(neigh_lst->id,nei_id) == 0){
			neigh* torm = neigh_lst;
			neigh_lst = torm->next;
			torm->next = NULL;
			if(parent == torm)
				parent = NULL;
			free(torm);
			return SUCCESS;
		}else{
			neigh* it = neigh_lst;
			while(it->next != NULL){
				if(uuid_compare(it->next->id, nei_id) == 0){
					neigh* torm = it->next;
					it->next = torm->next;
					torm->next = NULL;
					if(parent == torm)
						parent = NULL;
					free(torm);
					return SUCCESS;
				}

				it = it->next;
			}
			return FAILED;
		}
	}else{
		return FAILED;
	}
}

static neigh* add_neigh(uuid_t nei_id, uuid_t nei_tree, unsigned short lvl, double nei_val){
	neigh* newnei = malloc(sizeof(neigh));
	newnei->status = PASSIVE;
	newnei->val = nei_val;
	newnei->level = lvl;
	memcpy(newnei->id, nei_id, sizeof(uuid_t));
	memcpy(newnei->tree_id, nei_tree, sizeof(uuid_t));

	newnei->next = neigh_lst;
	neigh_lst = newnei;

	return newnei;
}

static void update_val(){
	neigh* it = neigh_lst;
	double newval = self->val;
	while(it != NULL){
		if(it->status == ACTIVE){
			newval += it->val;

			char u[37];
			uuid_unparse(it->id, u);
			char s[500];
			bzero(s, 500);
			sprintf(s, "Counting Active node %s %f", u, it->val);
			lk_log("MULTI ROOT", "UPDATE VAL", s);
		}

		it = it->next;
	}

	aggval = newval;

	char s[500];
	bzero(s, 500);
	sprintf(s, "New val: %f", aggval);
	lk_log("MULTI ROOT", "UPDATE VAL", s);
}

static void update_entry(uuid_t nei_id, uuid_t nei_tree, unsigned short nei_lvl, double nei_val){
	//TODO
	neigh* nei = find_nei(nei_id);
	if(nei == NULL){
		nei = add_neigh(nei_id, nei_tree, nei_lvl, nei_val);
	}else{
		nei->level = nei_lvl;
		nei->val = nei_val;
		memcpy(nei->tree_id, nei_tree, sizeof(uuid_t));
	}

}

static void update_tree(uuid_t nei_tree, time_t nei_time, unsigned short nei_lvl, uuid_t nei_parent, uuid_t nei_id, double nei_val){

	char u[37];
	uuid_unparse(my_tree->tree, u);
	char u2[37];
	uuid_unparse(nei_tree, u2);
	char u3[37];
	uuid_unparse(nei_id, u3);
	char u4[37];
	uuid_unparse(nei_parent, u4);

	char s[500];
	lk_log("MULTI ROOT", "UPDATE TREE", "START");

	bzero(s, 500);
	sprintf(s, "nei: %s", u3);
	lk_log("MULTI ROOT", "UPDATE TREE", s);

	int treediff = uuid_compare(nei_tree, my_tree->tree);

	bzero(s, 500);
	sprintf(s, "my_tree: %s nei_tree: %s treediff: %d", u, u2, treediff);
	lk_log("MULTI ROOT", "UPDATE TREE", s);

	neigh* nei = find_nei(nei_id);
	if(nei == NULL){
		nei = add_neigh(nei_id, nei_tree, nei_lvl, nei_val);
	}

	if(treediff == 0){

		bzero(s, 500);
		sprintf(s, "my_tree_time: %ld nei_time: %ld my_tree->last_seen < nei_time: %s", my_tree->last_seen, nei_time, my_tree->last_seen < nei_time ? "TRUE" : "FALSE");
		lk_log("MULTI ROOT", "UPDATE TREE", s);

		if(my_tree->last_seen < nei_time)
			my_tree->last_seen = nei_time;

		bzero(s, 500);
		sprintf(s, "new my_tree_time: %ld", my_tree->last_seen);
		lk_log("MULTI ROOT", "UPDATE TREE", s);
	}

	if(uuid_compare(nei_parent, self->id) == 0){
		//he is my child
		lk_log("MULTI ROOT", "UPDATE TREE", "Nei is my child");
		if(nei != parent){
			if(treediff == 0){
				//all good, we are in the same tree
				nei->status = ACTIVE;

			}else{
				//we are not in the same tree, what is the correct tree?
				//if he thinks I am his parent then we should be in the same tree:
				bzero(s, 500);
				sprintf(s, "=======> nei says he is my child but is in different tree in level %d I am in level: %d", nei_lvl, self->level);
				lk_log("MULTI ROOT", "UPDATE TREE", s);

			}
		}else{ //deal with cases when nei things i am his parent and I think he is my parent
			//force reconvergence
			my_tree = find_tree(self->id);
			if(my_tree == NULL){
				my_tree = create_tree(self->id);
			}
			//memcpy(tree_id, self->id, sizeof(uuid_t));
			self->level = 0;
			parent = self;
		}
	}else if(nei == parent){
		//he is my parent
		lk_log("MULTI ROOT", "UPDATE TREE", "Nei is my parent");

		if(treediff == 0){
			//all good, we are in the same tree
			nei->status = ACTIVE;
			self->level = parent->level + 1;

		}else{
			//we are not in the same tree, what is the correct tree?
			//my parent should be correct
			if(uuid_compare(nei_tree, self->id) < 0){
				my_tree = find_tree(nei_tree);
				if(my_tree == NULL){
					my_tree = create_tree(nei_tree);
				}
				//TODO UPDATE TIME
				//memcpy(tree_id, nei_tree, sizeof(uuid_t));
				nei->status = ACTIVE;
				self->level = parent->level + 1;

				bzero(s, 500);
				sprintf(s, "changed to my parent tree in level %d", self->level);
				lk_log("MULTI ROOT", "UPDATE TREE", s);

			}else{
				my_tree = find_tree(self->id);
				if(my_tree == NULL){
					my_tree = create_tree(self->id);
				}

				//memcpy(tree_id, self->id, sizeof(uuid_t));
				self->level = 0;
				parent = self;


				bzero(s, 500);
				sprintf(s, "I created tree in level %d", self->level);
				lk_log("MULTI ROOT", "UPDATE TREE", s);
			}

		}
	}else{
		//he is my peer

		lk_log("MULTI ROOT", "UPDATE TREE", "Nei says he is my peer");

		if(treediff >= 0)
			nei->status = PASSIVE;
		else if(treediff < 0){ //TODO else
			trees* tree = find_tree(nei_tree);
			if(tree == NULL){
				my_tree = create_tree(nei_tree);
				nei->status = ACTIVE;
				parent = nei;
				self->level = parent->level + 1;

				bzero(s, 500);
				sprintf(s, "nei tree is smaller and I havent seen it changed now in level %d he is my parent now", self->level);
				lk_log("MULTI ROOT", "UPDATE TREE", s);

			}else{

				bzero(s, 500);
				sprintf(s, "I have seen nei tree in %ld he have seen it in %ld tree->last_seen < nei_time : %s", tree->last_seen, nei_time, (tree->last_seen < nei_time ? "TRUE" : "FALSE"));
				lk_log("MULTI ROOT", "UPDATE TREE", s);

				if(tree->last_seen < nei_time){
					tree->last_seen = nei_time;
					my_tree = tree;
					nei->status = ACTIVE;
					parent = nei;
					self->level = parent->level + 1;

					bzero(s, 500);
					sprintf(s, "changed to nei tree in level %d he is my parent now", self->level);
					lk_log("MULTI ROOT", "UPDATE TREE", s);

				}else{
					nei->status = PASSIVE;
				}
			}

		}

	}

	neigh* it = neigh_lst;
	while(it != NULL){

		if(it->status == PASSIVE && (uuid_compare(it->tree_id, my_tree->tree) == 0) && it->level < parent->level){
			parent->status = PASSIVE;
			parent = it;

			self->level = parent->level + 1;
			parent->status = ACTIVE;
		}

		it = it->next;
	}

	uuid_unparse(my_tree->tree, u);
	uuid_unparse(parent->id, u2);
	bzero(s, 500);
	sprintf(s, "Tree id: %s tree time: %ld parent: %s parent_lvl: %d mylvl: %d nei_status: %s nei_lv: %d", u, my_tree->last_seen, u2, parent->level, self->level, nei->status == 1 ? "PASSIVE" : "ACTIVE", nei_lvl);
	lk_log("MULTI ROOT", "UPDATE TREE", s);

	//	if(treediff == 0){
	//		//It is equal, we are in the same tree
	//		if(uuid_compare(nei_parent, self->id) == 0){
	//			//he is my child
	//			nei->status = ACTIVE;
	//		}else if(nei == parent){
	//			//he is my parent
	//			nei->status = ACTIVE;
	//			self->level = parent->level + 1;
	//		}else{
	//			nei->status = PASSIVE;
	//		}
	//
	//		char u[37];
	//		uuid_unparse(tree_id, u);
	//		char u2[37];
	//		uuid_unparse(parent->id, u2);
	//		char u3[37];
	//		uuid_unparse(nei->id, u3);
	//		char s[500];
	//		bzero(s, 500);
	//		sprintf(s, "Tree: %s parent: %s lvl: %d mylvl: %d nei: %s neilvl: %d neistatus: %d", u, u2, parent->level, self->level, u3, nei->level, nei->status);
	//		lk_log("MULTI ROOT", "UPDATE TREE", s);
	//
	//	}else if(treediff < 0) {
	//		//we are not in the same tree, nei is in lowest or is my parent, change to that tree
	//		memcpy(tree_id, nei_tree, sizeof(uuid_t));
	//
	//		nei->status = ACTIVE;
	//		parent = nei;
	//		self->level = parent->level + 1;
	//
	//		char u[37];
	//		uuid_unparse(tree_id, u);
	//		char u2[37];
	//		uuid_unparse(parent->id, u2);
	//		char s[500];
	//		bzero(s, 500);
	//		sprintf(s, "Changed tree id: %s parent: %s lvl: %d mylvl: %d", u, u2, parent->level, self->level);
	//		lk_log("MULTI ROOT", "UPDATE TREE", s);
	//
	//	}

	lk_log("MULTI ROOT", "UPDATE TREE", "END");
}

//EVENT PROCESSOR:
static void processNeiDown(uuid_t nei){
	rm_neigh(nei);

	if(parent == NULL){
		neigh* it = neigh_lst;
		neigh* candidate = self;
		while(it != NULL){
			if(it->status == PASSIVE && (uuid_compare(it->tree_id, my_tree->tree) == 0)){
				if(it->level < self->level){ //TODO it->level < candidate->lvl
					candidate = it;
				}
			}
			it = it->next;
		}
		parent = candidate;
		if(parent == self){
			my_tree = find_tree(self->id);
			if(my_tree == NULL){
				my_tree = create_tree(self->id);
			}
			self->level = 0;
		}else{
			self->level = parent->level + 1;
		}

	}

	char u[37];
	uuid_unparse(my_tree->tree, u);
	char u2[37];
	uuid_unparse(parent->id, u2);
	char s[500];
	bzero(s, 500);
	sprintf(s, "Tree id: %s parent: %s parent_lvl: %d mylvl: %d", u, u2, parent->level, self->level);
	lk_log("MULTI ROOT", "NEIGH DOWN", s);
}

static void sendMsg(){

	update_val();

	LKMessage msg;
	LKMessage_initBcast(&msg, protoId);

	if(uuid_compare(my_tree->tree, self->id) == 0){
		my_tree->last_seen = time(NULL);
	}

	char s[500];
	bzero(s, 500);
	sprintf(s, "my_tree_time: %ld", my_tree->last_seen);
	lk_log("MULTI ROOT", "MSG SEND", s);

	//Tree id
	LKMessage_addPayload(&msg, (void*) my_tree->tree, sizeof(uuid_t));
	LKMessage_addPayload(&msg, (void*) &my_tree->last_seen, sizeof(time_t));
	LKMessage_addPayload(&msg, (void*) &self->level, sizeof(unsigned short));
	LKMessage_addPayload(&msg, (void*) parent->id, sizeof(uuid_t));


	//my values
	LKMessage_addPayload(&msg, (void*) self->id, sizeof(uuid_t));
	LKMessage_addPayload(&msg, (void*) &self->val, sizeof(double));

	//neighs offset to me
	neigh* it = neigh_lst;
	while(it != NULL){
		LKMessage_addPayload(&msg, (void*) it->id, sizeof(uuid_t));
		double tosend = (aggval - it->val);
		LKMessage_addPayload(&msg, (void*) &tosend, sizeof(double));

		it = it->next;
	}

	dispatch(&msg);
}

static void processMsg(LKMessage* msg){

	void* payload = msg->data;
	unsigned short payload_len = msg->dataLen;

	//tree id of sender
	uuid_t other_tree;
	time_t other_time;
	unsigned short other_lvl;
	uuid_t other_parent;

	memcpy(other_tree, payload,  sizeof(uuid_t));
	payload += sizeof(uuid_t);
	memcpy(&other_time, payload, sizeof(time_t));
	payload += sizeof(time_t);
	memcpy(&other_lvl, payload, sizeof(unsigned short));
	payload += sizeof(unsigned short);
	memcpy(other_parent, payload, sizeof(uuid_t));
	payload += sizeof(uuid_t);

	//values of sender
	uuid_t other_node_id;
	double other_val;

	memcpy(other_node_id, payload, sizeof(uuid_t));
	payload += sizeof(uuid_t);
	memcpy(&other_val, payload, sizeof(double));
	payload += sizeof(double);

	payload_len -= ((sizeof(uuid_t)*3) + sizeof(time_t) + sizeof(unsigned short) + sizeof(double));

	while(payload_len >= (sizeof(uuid_t) + sizeof(double))){
		uuid_t nei;
		double nei_val;
		memcpy(nei, payload, sizeof(uuid_t));
		payload += sizeof(uuid_t);
		if(uuid_compare(nei, self->id) == 0){
			memcpy(&nei_val, payload, sizeof(double));
			update_entry(other_node_id, other_tree, other_lvl, nei_val);
			break;
		}
		payload += sizeof(double);
		payload_len -= (sizeof(uuid_t) + sizeof(double));
	}

	update_tree(other_tree, other_time, other_lvl, other_parent, other_node_id, other_val);
}

//MAIN LOOP:
void* multi_root_init(void* args){

	proto_args* pargs = (proto_args*) args;

	queue_t* inBox = pargs->inBox;
	protoId = pargs->protoID;

	fault_detector_id = *((short*)pargs->protoAttr);

	self = malloc(sizeof(neigh));
	memcpy(self->id, pargs->myuuid, sizeof(uuid_t));

	aggregation_get_value(DEFAULT, &self->val);

	self->level = 0;

	parent = self;
	//memcpy(tree_id, parent->id, sizeof(uuid_t));

	trees_seen = malloc(sizeof(trees));
	memcpy(trees_seen->tree, parent->id, sizeof(uuid_t));
	trees_seen->last_seen = time(NULL);

	trees_seen->next = NULL;

	my_tree = trees_seen;

	aggval = self->val;

	LKTimer beacon;
	LKTimer_init(&beacon, protoId, protoId);
	LKTimer_set(&beacon, BEACON_INTERVAL, BEACON_INTERVAL);

	setupTimer(&beacon);

	queue_t_elem elem;


	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE){
			//Upon recv:
			processMsg(&elem.data.msg);
		}else if(elem.type == LK_TIMER){
			//beacon
			sendMsg();
		}else if(elem.type == LK_EVENT){
			//event neigh_down
			if(elem.data.event.proto_origin == fault_detector_id){
				if(elem.data.event.notification_id == DISCOV_FT_LB_EV_NEIGH_DOWN){
					uuid_t torm;
					memcpy(torm, elem.data.event.payload, sizeof(uuid_t));
					//restore tree
					processNeiDown(torm);
				}
			}
			if(elem.data.event.payload != NULL){
				free(elem.data.event.payload);
				elem.data.event.payload = NULL;
			}

		}else if(elem.type == LK_REQUEST){
			//TODO aggregation_gen_interface (APP process)
		}
	}
}
