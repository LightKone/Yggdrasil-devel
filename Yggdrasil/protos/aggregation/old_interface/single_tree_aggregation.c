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

#include "single_tree_aggregation.h"

typedef struct stree_agg_value_ {
	void* value;
	//size_t size;
	aggregation_function func;
} stree_agg_value;

typedef struct stree_agg_state {
	stree_agg_value agg_value;

	node* to_be_accounted;
	node* children;
	node* parent;

	short received;
	short sink;
	short done;
} stree_agg_state;

typedef struct aggregation_instance_ {
	uuid_t id;
	agg_op op;
	short sequence_number;

	stree_agg_state state;
	LKRequest* request;
	struct aggregation_instance_* next;
} aggregation_instance;

static aggregation_instance* instances = NULL;
static node* neighbours = NULL;

static short protoId;
static uuid_t my_id;
static short my_sequence_number;

static aggregation_instance* stree_find_instance(const uuid_t* id, const short sequence_number, const agg_op op);
static void stree_add_instance(aggregation_instance* new_instance);
//static void stree_remove_instance(aggregation_instance* instance);
static aggregation_instance* stree_register_instance(const uuid_t* id, const short sequence_number, const agg_op op);
//static void stree_destroy_instance(aggregation_instance* instance);

static void stree_send_query(const aggregation_instance* instance);
static void stree_send_child(const aggregation_instance* instance, WLANAddr addr, child_response response);
static void stree_send_value(const aggregation_instance* instance);

static void stree_receive_query(aggregation_instance* instance, const WLANAddr sender_addr);
static void stree_receive_child(aggregation_instance* instance, const uuid_t sender_id, const WLANAddr sender_addr, void* msg_payload);
static void stree_receive_value(aggregation_instance* instance, const uuid_t sender_id, const WLANAddr sender_addr, void* msg_payload);

static void stree_report(aggregation_instance* my_instance);

/********************************* INSTANCES *********************************/
static aggregation_instance* stree_find_instance(const uuid_t* id, const short sequence_number, const agg_op op) {
	aggregation_instance* it = instances;
	while(it != NULL){
		if(uuid_compare(*id, it->id) == 0 && it->sequence_number == sequence_number && it->op == op){
			return it;
		}
		it = it->next;
	}

	return NULL;
}

static void stree_add_instance(aggregation_instance* newinstance) {
	if(instances == NULL){
		instances = newinstance;
		instances->next = NULL;
		return;
	}

	aggregation_instance* it = instances;
	while(it->next != NULL){
		it = it->next;
	}

	it->next = newinstance;
	newinstance->next = NULL;
}

//static void stree_remove_instance(aggregation_instance* instance) {
//	aggregation_instance* it = instances;
//	aggregation_instance* prev = NULL;
//
//	while(it->next != NULL && it->id == instance->id && it->sequence_number == instance->sequence_number){
//		prev = it;
//		it = it->next;
//	}
//
//	prev->next = it->next;
//}

static aggregation_instance* stree_register_instance(const uuid_t* id, const short sequence_number, const agg_op op) {
	printf("Registering EXISTING instance\n");

	// create local version of the instance
	aggregation_instance* instance = malloc(sizeof(aggregation_instance));

	memcpy(instance->id, id, sizeof(uuid_t));
	instance->sequence_number = sequence_number;
	instance->op = op;

	// insert the request that originated the instance
	instance->request = NULL; // TODO??

	// default values
	instance->state.received = 0;
	instance->state.sink = 0;
	instance->state.done = 0;

	instance->state.agg_value.func = aggregation_function_get(instance->op);

	// get the local value for initialisation
	instance->state.agg_value.value = malloc(aggregation_function_get_valueSize(instance->op));
	if(!aggregation_function_get_value(instance->op, instance->state.agg_value.value)){
		lk_log("STREE AGG", "ERROR", "Could not get value");
	}

	// fill to_be_accounted with my neighbours
	instance->state.to_be_accounted = stree_node_deep_clone(neighbours);

	stree_add_instance(instance);
	return instance;
}

//static void stree_destroy_instance(aggregation_instance* instance) {
//	free(instance->state.agg_value.value);
//
//	//free(instance->state.toBeAccounted);
//	free(instance->state.children);
//	free(instance->state.parent);
//
//	free(instance->request);
//	free(instance);
//}
/******************************* END INSTANCES *******************************/
static const size_t msg_header_size = sizeof(msg_type) + 2 * sizeof(uuid_t) + sizeof(short) + sizeof(agg_op);
static void fill_header_msg(char* data, const msg_type type, const uuid_t id, const short sequence_number, const agg_op op, const uuid_t sender_id) {
	memcpy(data, &type, sizeof(msg_type));
	memcpy(data + sizeof(msg_type), id, sizeof(uuid_t));
	memcpy(data + sizeof(msg_type) + sizeof(uuid_t), &sequence_number, sizeof(short));
	memcpy(data + sizeof(msg_type) + sizeof(uuid_t) + sizeof(short), &op, sizeof(agg_op));
	memcpy(data + sizeof(msg_type) + sizeof(uuid_t) + sizeof(short) + sizeof(agg_op), sender_id, sizeof(uuid_t));
}

static void stree_send_query(const aggregation_instance* instance) { //1
	LKMessage msg;
	LKMessage_initBcast(&msg, protoId);

	char payload[msg_header_size];
	fill_header_msg(payload, QUERY, instance->id, instance->sequence_number, instance->op, my_id);

	LKMessage_addPayload(&msg, payload, msg_header_size);
	dispatch(&msg);
}

static void stree_send_child(const aggregation_instance* instance, WLANAddr addr, child_response response) { //2
	LKMessage msg;
	LKMessage_init(&msg, addr.data, protoId);

	size_t len = msg_header_size + sizeof(child_response);
	char payload[len];
	fill_header_msg(payload, CHILD, instance->id, instance->sequence_number, instance->op, my_id);

	// add the response to the payload
	memcpy(payload + msg_header_size, &response, sizeof(child_response));

	LKMessage_addPayload(&msg, payload, len);
	dispatch(&msg);
}

static void stree_send_value(const aggregation_instance* instance) { //3
	char uuid_str21[37];
	memset(uuid_str21,0,37);
	uuid_unparse(instance->state.parent->uuid, uuid_str21);
	printf("PARENT ID %s ",uuid_str21);

	LKMessage msg;
	LKMessage_init(&msg, instance->state.parent->addr.data, protoId);

	size_t len = msg_header_size + aggregation_function_get_valueSize(instance->op);
	char payload[len];
	fill_header_msg(payload, VALUE, instance->id, instance->sequence_number, instance->op, my_id);

	// add the value to the payload
	memcpy(payload + msg_header_size, instance->state.agg_value.value, aggregation_function_get_valueSize(instance->op));

	LKMessage_addPayload(&msg, payload, len);
	dispatch(&msg);
	printf("Sent value\n");
}

static void stree_receive_query(aggregation_instance* instance, const WLANAddr sender_addr) {
	node* n = stree_node_create(instance->id, sender_addr);
	//agg_op op = instance->op;

	if (!instance->state.received) {
		instance->state.received = 1;
		instance->state.parent = n;
		stree_send_child(instance, sender_addr, YES);

		/*
		// not needed 1 hop bcast
		node* sendList = node_get_copy_with_exclude(instance->state.toBeAccounted, instance->state.parent->uuid);
		while (sendList) {
			stree_send_query(&sendList->addr, op);
			node* tmp = sendList;
			sendList = sendList->next;
			tmp->next = NULL;
			free(tmp);
		}*/

		stree_send_query(instance);
		printf("Recv query; Sent YES child + sent query\n");
	} else {
		stree_send_child(instance, sender_addr, NO);
		printf("Recv query; Sent NO child\n");
	}
}

static void stree_receive_child(aggregation_instance* instance, const uuid_t sender_id, const WLANAddr sender_addr, void* msg_payload) {
	// deserialise response from message
	child_response r;
	memcpy(&r, msg_payload, sizeof(child_response));

	if (r == YES)
		stree_node_add(&instance->state.children, sender_id, sender_addr);

	char uuid_str2[37];
	memset(uuid_str2,0,37);
	uuid_unparse(sender_id, uuid_str2);
	printf("removal %s\n",uuid_str2);

	stree_node_remove(&instance->state.to_be_accounted, sender_id);

	stree_report(instance);
	printf("Received child with %s\n", r == YES? "YES":"NO");
}

static void stree_receive_value(aggregation_instance* instance, const uuid_t sender_id, const WLANAddr sender_addr, void* msg_payload) {
	agg_op op = instance->op;
	if (op < 0 || op > 4) {
		lk_log("SINGLE TREE AGG", "ERROR", "No support for this type of aggregation");
		return;
	}

	void* recv_val = malloc(aggregation_function_get_valueSize(instance->op));
	memcpy(recv_val, msg_payload, aggregation_function_get_valueSize(op));

	// execute the function locally
	aggregation_function func = aggregation_function_get(op);
	func(instance->state.agg_value.value, recv_val, instance->state.agg_value.value);

	free(recv_val);

	// remove the node whose value we got
	stree_node_remove(&instance->state.children, sender_id);
	//free(n);

	stree_report(instance);
	printf("Received value\n");
}

static void stree_report(aggregation_instance* my_instance) {
	printf("##try Report value##\n");
	printf("##");
	node* curr = my_instance->state.to_be_accounted;
	while (curr) {
		char uuid_str21[37];
		memset(uuid_str21,0,37);
		uuid_unparse(curr->uuid, uuid_str21);
		printf("%s ",uuid_str21);

		curr = curr->next;
	}
	printf("##\n");

	printf("##");
	curr = my_instance->state.children;
	while (curr) {
		char uuid_str211[37];
		memset(uuid_str211,0,37);
		uuid_unparse(curr->uuid, uuid_str211);
		printf("%s ",uuid_str211);

		curr = curr->next;
	}
	printf("##\n");

	if (my_instance->state.to_be_accounted || my_instance->state.children)
		return;

	printf("Report value\n");
	my_instance->state.done = 1;

	if (!my_instance->state.sink) {
		stree_send_value(my_instance);
	} else {
		printf("parent reports to super parent its value\n");
		// pack it up, boys
		double val;
		memcpy(&val, my_instance->state.agg_value.value, sizeof(double));
		aggregation_reportValue(my_instance->request, protoId, val);
		printf("done\n");

		//stree_remove_instance(my_instance);
		//stree_destroy_instance(my_instance);
	}
}

static void stree_process_fault(const uuid_t id) {
	aggregation_instance* inst = instances;

	while(inst) {
		if(!inst->state.done) {
			stree_node_remove(&inst->state.to_be_accounted, id);
			stree_node_remove(&inst->state.children, id);
		}

		inst = inst->next;
	}
}

/******************************** MSG GETTERS ********************************/
static msg_type stree_extract_type(void** payload) {
	msg_type type;
	memcpy(&type, *payload, sizeof(msg_type));

	// increment pointer
	*payload += sizeof(msg_type);

	return type;
}

static void stree_extract_id(void** payload, uuid_t* id) {
	memcpy(*id, *payload, sizeof(uuid_t));

	// increment pointer
	*payload += sizeof(uuid_t);
}

static short stree_extract_seqnum(void** payload) {
	short seqnum;
	memcpy(&seqnum, *payload, sizeof(short));

	// increment pointer
	*payload += sizeof(short);

	return seqnum;
}

static agg_op stree_extract_op(void** payload) {
	agg_op op;
	memcpy(&op, *payload, sizeof(agg_op));

	// increment pointer
	*payload += sizeof(agg_op);

	return op;
}

static void stree_extract_sender_id(void** payload, uuid_t* id) {
	memcpy(*id, *payload, sizeof(uuid_t));

	// increment pointer
	*payload += sizeof(uuid_t);
}

/****************************** END MSG GETTERS ******************************/

static agg_op stree_req_get_op(const LKRequest* req) {
	switch (req->request_type) {
	case AGG_REQ_DO_SUM:
		return SUM;
	case AGG_REQ_DO_MAX:
		return MAX;
	case AGG_REQ_DO_MIN:
		return MIN;
	case AGG_REQ_DO_AVG:
		return AVG;
	default:
		return NONE; // enum can't return NULL
	}
}

static aggregation_instance* stree_create_instance(const LKRequest* req) {
	printf("Creating NEW instance\n");
	aggregation_instance* new_instance = malloc(sizeof(aggregation_instance));

	// instance id comes from protocol
	memcpy(new_instance->id, my_id, sizeof(uuid_t));

	// sequence number increases from last one
	new_instance->sequence_number = my_sequence_number;
	my_sequence_number++;

	new_instance->op = stree_req_get_op(req);

	// insert the request that originated the instance
	new_instance->request = malloc(sizeof(LKRequest));
	memcpy(new_instance->request, req, sizeof(LKRequest));

	new_instance->state.agg_value.func = aggregation_function_get(new_instance->op);

	// get my value to add to instance
	//new_instance->state.agg_value.size = aggregation_function_get_valueSize(new_instance->op);
	new_instance->state.agg_value.value = (void*)malloc(aggregation_function_get_valueSize(new_instance->op));
	aggregation_function_get_value(new_instance->op, new_instance->state.agg_value.value);

	// default values
	new_instance->state.received = 1; // to answer "NO" to child msgs
	new_instance->state.sink = 1;
	new_instance->state.done = 0;

	// fill to_be_accounted with my neighbours
	new_instance->state.to_be_accounted = stree_node_deep_clone(neighbours);

	return new_instance;
}

/******************************** PROCESSORS ********************************/
static void stree_process_lkmessage(LKMessage * msg) {
	printf("s-----------------\n");
	void* payload = msg->data;

	// get the necessary fields from the payload
	const msg_type type = stree_extract_type(&payload);

	uuid_t id;
	stree_extract_id(&payload, &id);

	const short seq_num = stree_extract_seqnum(&payload);
	const agg_op op = stree_extract_op(&payload);

	uuid_t sender_id;
	stree_extract_sender_id(&payload, &sender_id);

	char uuid_str2111[37];
	memset(uuid_str2111,0,37);
	uuid_unparse(sender_id, uuid_str2111);
	printf("%s\n", uuid_str2111);

	// find the local instance, if it exists
	aggregation_instance* my_instance = stree_find_instance(&id, seq_num, op);
	if(!my_instance){
		char uuid_str2[37];
		memset(uuid_str2,0,37);
		uuid_unparse(id, uuid_str2);
		printf("%s %d %d\n", uuid_str2, seq_num, op);
		my_instance = stree_register_instance(&id, seq_num, op);
	} else
		printf("already had, didnt register again\n");

	if(my_instance->state.done)
		return;

	/*printf("instances\n");
	aggregation_instance *x = instances;
	while(x) {
		char uuid_str21[37];
		memset(uuid_str21,0,37);
		uuid_unparse(x->id, uuid_str21);

		printf("%s %d %d\n", uuid_str21, x->sequence_number, x->op);
		x = x->next;
	}*/

	printf("instance type %d\n", type);

	switch (type) {
	case QUERY:
		printf("will receive query\n");
		stree_receive_query(my_instance, msg->srcAddr);
		break;
	case CHILD:
		printf("will receive child\n");
		stree_receive_child(my_instance, sender_id, msg->srcAddr, payload);
		break;
	case VALUE:
		printf("will receive value\n");
		stree_receive_value(my_instance, sender_id, msg->srcAddr, payload);
		break;
	default:
		lk_log("SINGLE TREE AGG", "WARNING", "Unknown message type received");
		break;
	}
	printf("e-----------------\n");
}

static void stree_process_lkreq(LKRequest * req) {
	if (req->request == REQUEST) {
		// create the new instance
		aggregation_instance* new_instance = stree_create_instance(req);
		stree_add_instance(new_instance);

		// send query request to everyone
		printf("sending query from root\n");
		stree_send_query(new_instance);
	} else {
		lk_log("STREE", "WARNING", "Unknown Request type");
	}
}

static void stree_process_lkevent(LKEvent* ev) {
	if(ev->notification_id == DISCOV_FT_LB_EV_NEIGH_UP) {
		uuid_t id;
		memcpy(id, ev->payload, sizeof(uuid_t));

		char uuid_str2[37];
		memset(uuid_str2,0,37);
		uuid_unparse(id, uuid_str2);
		printf("got neigh %s\n", uuid_str2);

		WLANAddr addr;
		memcpy(addr.data, ev->payload + sizeof(uuid_t), WLAN_ADDR_LEN);

		// add node to neighs
		stree_node_add(&neighbours, id, addr);

		// TODO decide what to do with existing instances
		// maybe let them run to the end without the new neigh
	} else if(ev->notification_id == DISCOV_FT_LB_EV_NEIGH_DOWN) {
		uuid_t id;
		memcpy(id, ev->payload, sizeof(uuid_t));

		char uuid_str2[37];
		memset(uuid_str2,0,37);
		uuid_unparse(id, uuid_str2);
		printf("lost neigh %s\n", uuid_str2);

		//WLANAddr addr;
		//memcpy(addr.data, ev->payload + sizeof(uuid_t), WLAN_ADDR_LEN);

		// remove node from neighs
		stree_node_remove(&neighbours, id);

		// now remove from all running instances waiting for that node
		stree_process_fault(id);

		// TODO decide what to do with existing instances
		// maybe send a destroy instance msg to all
	} else {
		lk_log("STREE", "WARNING", "Unknown Event type");
	}

	if(ev->payload && !ev->length)
		free(ev->payload);
}

/****************************** END PROCESSORS ******************************/

void* stree_init(void* args) {
	proto_args* pargs = (proto_args*) args;

	queue_t* inBox = pargs->inBox;
	protoId = pargs->protoID;
	memcpy(my_id, pargs->myuuid, sizeof(uuid_t));
	my_sequence_number = 0;

	while (1) {
		queue_t_elem elem;
		queue_pop(inBox, &elem);

		if (elem.type == LK_MESSAGE) {
			LKMessage msg = elem.data.msg;
			stree_process_lkmessage(&msg);
		} else if (elem.type == LK_REQUEST) {
			LKRequest req = elem.data.request;
			stree_process_lkreq(&req);
		} else if (elem.type == LK_EVENT) {
			LKEvent ev = elem.data.event;
			stree_process_lkevent(&ev);
		} else {
			lk_log("SINGLE TREE AGG", "WARNING", "Unknown type received from queue");
		}
	}
}
