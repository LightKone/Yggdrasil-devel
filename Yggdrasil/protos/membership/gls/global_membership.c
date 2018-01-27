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

#include "global_membership.h"

// proto args
static unsigned short protoID;
static time_t announce_period;
static uuid_t myself;

// state
static members* membership;
static LKTimer* announce;
static time_t last_period;

static void timer_announce_reset() {

	cancel_timer(announce);

	//setup the timer for new times
	LKTimer_set(announce, (rand() % last_period * 1000 * 1000) + 2 * 1000 * 1000, 0);

	last_period = announce_period;
}

static void process_ev_neigh_up(const LKEvent* recv_event) {
	// process received data
	uuid_t recv_uuid;
	memcpy(recv_uuid, recv_event->payload, sizeof(uuid_t));

	short recv_counter = -1;
	memcpy(&recv_counter, recv_event->payload + sizeof(uuid_t), sizeof(short));

	// find if we already have the member
	int pos = find_member(membership, recv_uuid);


	if (pos == -1) {
		// if the member was not known, insert it

		//member* m =
		insert_member(membership, recv_uuid, recv_counter, DIRECT);

	} else {
		// if we already had it, update the counter as needed
		member* existing = membership->list + pos;
		if(existing->counter < recv_counter){
			existing->counter = recv_counter;
			existing->attr = DIRECT; // should it have died previously

		} else if(existing->counter == recv_counter && existing->attr != DEAD){
			existing->attr = DIRECT; // should it have died previously
		}
	}

	//cancel the times

	timer_announce_reset();

	free(recv_event->payload);
}

static void process_ev_neigh_down(const LKEvent* recv_event) {
	// process received data
	uuid_t recv_uuid;
	memcpy(recv_uuid, recv_event->payload, sizeof(uuid_t));

	int pos = find_member(membership, recv_uuid);
	if (pos == -1) {
		lk_log("GLS MEMEBERSHIP", "GLOBAL CHAOS", "Trying to remove unexisting node");
		return;
	}

	// we remove the dead from the list
	member* died = membership->list + pos;
	if(died->attr != DEAD){
		died->attr = DEAD;
		//		died->counter ++;


		// logging
		char k[12];
		memset(k, 0, 12);
		sprintf(k, "%d", membership->size);
		lk_log("GLS MEMBERSHIP", "NNEIGH", k);

		char x[4];
		memset(x, 0, 4);
		sprintf(x, "%d", pos);
		lk_log("GLS MEMBERSHIP", "DELNEIGH", x);
	}

	//cancel the times
	timer_announce_reset(); //network is instable should reset timer to see if everything is ok

	free(recv_event->payload);
}

static void serialiazeReply(LKRequest* req) {
	void* payload = malloc(sizeof(short) + (sizeof(member) * membership->size));
	memcpy(payload, &membership->size, sizeof(short));
	int len = sizeof(short);
	member* m = membership->list;

	req->payload = payload;
	payload += sizeof(short);
	if(membership->size > 0)
		memcpy(payload, m , sizeof(member)*membership->size);
	len += sizeof(member)*membership->size;

	req->length = len;

}

static void process_req_query(LKRequest* req) {

	req->proto_dest = req->proto_origin;
	req->proto_origin = protoID;
	req->request = REPLY;
	req->request_type = GLS_QUERY_RES;

	serialiazeReply(req);

	deliverReply(req);

	free(req->payload);
}


void GLS_deserialiazeReply(gls_query_reply* gqr, LKRequest* req) {

	void * payload = req->payload;

	memcpy(&gqr->nmembers, payload, sizeof(short));
	payload += sizeof(short);


	if(gqr->nmembers > 0){
		gqr->members = malloc(sizeof(member)*gqr->nmembers);
		memcpy(gqr->members, payload, sizeof(member)*gqr->nmembers);
	}
	free(req->payload);
}

static void send_ev_me_dead(short counter) {
	LKEvent me_dead_event;

	me_dead_event.proto_origin = protoID;
	me_dead_event.notification_id = GLS_ME_DEAD;
	me_dead_event.length = sizeof(short);

	me_dead_event.payload = malloc(sizeof(short));

	memcpy(me_dead_event.payload, &counter, sizeof(short));

#ifdef DEBUG
	lk_log("GLS MEMBERSHIP", "DEBUG", "I am not dead");
#endif

	deliverEvent(&me_dead_event);

	free(me_dead_event.payload);
}

static void send_announcement() {
	// create the announcement msg
	LKMessage msg;
	push_msg_info(&msg, protoID);

	int len = 0;

	// collect members to send
	const size_t member_size = sizeof(uuid_t) + sizeof(short) + sizeof(member_attr);
	const short members_to_write = min((MAX_PAYLOAD - sizeof(short)) / member_size, membership->size);

	// start by putting the number of members

	memcpy(msg.data, &members_to_write, sizeof(short));

	len += sizeof(short);

	char* toWrite = msg.data + sizeof(short);

	members* tmp = malloc(sizeof(members));
	tmp->size = membership->size;
	tmp->list = malloc(sizeof(member)*membership->size);
	memcpy(tmp->list, membership->list, sizeof(member)*membership->size);


	// for each member we send the uuid, counter and attr (dead or alive)
	int i;
	for(i = 0; i < members_to_write; i++) {
		member to_write;
		randomSelect(tmp, &to_write);
		char * write_pos = toWrite + (i * member_size);

#ifdef DEBUG
		char m[2000];
		memcpy(write_pos, to_write.uuid, sizeof(uuid_t));
		memset(m, 0, 2000);
		char u[37];
		uuid_unparse(to_write.uuid, u);
		sprintf(m, "Send announce, uuid_t of neight %s", u);
		lk_log("GLS MEMBERSHIP", "DEBUG", m);
#endif

		memcpy(write_pos + sizeof(uuid_t), &to_write.counter, sizeof(short));

#ifdef DEBUG
		memset(m, 0, 2000);
		sprintf(m, "Send announce, counter of neight %d", to_write.counter);
		lk_log("GLS MEMBERSHIP", "DEBUG", m);
#endif

		memcpy(write_pos + sizeof(uuid_t) + sizeof(short), &to_write.attr, sizeof(member_attr));

#ifdef DEBUG
		memset(m, 0, 2000);
		sprintf(m, "Send announce, attr of neight %d", to_write.attr);
		lk_log("GLS MEMBERSHIP", "DEBUG", m);
#endif

		len += member_size;
	}

	msg.dataLen = len;
	dispatch(&msg);

	destroy_membership(tmp);
}

static void send_new_members(members* new_members) {
	// create the announcement msg
	LKMessage msg;
	push_msg_info(&msg, protoID);

	int len = 0;

	// collect members to send
	const size_t member_size = sizeof(uuid_t) + sizeof(short) + sizeof(member_attr);
	const short members_to_write = min((MAX_PAYLOAD - sizeof(short)) / member_size, new_members->size);

	// start by putting the number of members
	memcpy(msg.data, &members_to_write, sizeof(short));

	len += sizeof(short);

	char* toWrite = msg.data + sizeof(short);

	// for each member we send the uuid, counter and attr (dead or alive)
	int i;
	for(i = 0; i < members_to_write; i++) {
		member* to_write;
		to_write = new_members->list + i; //TODO could be optimized to send random membership if there is still space left

		char * write_pos = toWrite + (i * member_size);

		memcpy(write_pos, to_write->uuid, sizeof(uuid_t));
		memcpy(write_pos + sizeof(uuid_t), &to_write->counter, sizeof(short));
		memcpy(write_pos + sizeof(uuid_t) + sizeof(short), &to_write->attr, sizeof(member_attr));
		len += member_size;
	}

	msg.dataLen = len;

#ifdef DEBUG
	char s[2000];
	memset(s, 0, 2000);
	sprintf(s, "sending new members, n members %d", members_to_write);
	lk_log("GLS MEMBERSHIP", "DEBUG", s);
#endif

	dispatch(&msg);
}

static void receive_announcement(LKMessage* recv_msg) {
	// process received data

	members* new_members = malloc(sizeof(members));
	init_members(new_members);

	short nr_members = -1;
	memcpy(&nr_members, recv_msg->data, sizeof(short));

	char* toRead = recv_msg->data + sizeof(short);

	int i;
	for(i = 0; i < nr_members; i++) {
		// decode member information
		uuid_t recv_uuid;
		memcpy(recv_uuid, toRead, sizeof(uuid_t));

		toRead = toRead + sizeof(uuid_t); //advance buffer

		short recv_counter = -1;
		memcpy(&recv_counter, toRead, sizeof(short));

		toRead = toRead + sizeof(short);

		member_attr recv_attr = -1;
		memcpy(&recv_attr, toRead, sizeof(member_attr));

		toRead = toRead + sizeof(member_attr);

		// process info
		// direct of others becomes our indirect
		if(recv_attr == DIRECT)
			recv_attr = INDIRECT;

		// check if member was present
		int pos = find_member(membership, recv_uuid);
		if(pos == -1) {
			// if the member was not known, insert it
			insert_member(membership, recv_uuid, recv_counter, recv_attr);
			insert_member(new_members, recv_uuid, recv_counter, recv_attr);

#ifdef DEBUG
			char msg[2000];
			char u[37];
			uuid_unparse(recv_uuid, u);
			memset(msg, 0, 2000);
			sprintf(msg, "adding new member to list: uuid %s counter %d status %d", u, recv_counter, recv_attr);
			lk_log("GLS MEMBERSHIP", "DEBUG", msg);
#endif

		} else {
			member* existing = membership->list + pos;
			if(uuid_compare(existing->uuid, myself) == 0 && recv_attr == DEAD) {
				// i'm not dead! increment the counter
				send_ev_me_dead(recv_counter);
			} else if(existing->counter == recv_counter && recv_attr == DEAD && existing->attr != recv_attr) {
				existing->attr = recv_attr;
				insert_member(new_members, recv_uuid, recv_counter, recv_attr);

#ifdef DEBUG
				char msg[2000];
				char u[37];
				uuid_unparse(recv_uuid, u);
				memset(msg, 0, 2000);
				sprintf(msg, "updating new member list: uuid %s counter %d status %d", u, recv_counter, recv_attr);
				lk_log("GLS MEMBERSHIP", "DEBUG", msg);
#endif

			} else if(existing->counter < recv_counter){
				// update counter
				existing->counter = recv_counter;
				existing->attr = recv_attr;

				insert_member(new_members, recv_uuid, recv_counter, recv_attr);

#ifdef DEBUG
				char msg[2000];
				char u[37];
				uuid_unparse(recv_uuid, u);
				memset(msg, 0, 2000);
				sprintf(msg, "updating new member list: uuid %s counter %d status %d", u, recv_counter, recv_attr);
				lk_log("GLS MEMBERSHIP", "DEBUG", msg);
#endif

			}
		}

	}

	if(new_members->size > 0)
		send_new_members(new_members);
	destroy_membership(new_members);

}

void * global_membership_init(void * args) {
	proto_args* pargs = args;
	protoID = pargs->protoID;
	memcpy(myself, pargs->myuuid, UUID_SIZE);

	// initialise state structs
	membership = malloc(sizeof(members));
	init_members(membership);


	// create the announcement timer
	announce = malloc(sizeof(LKTimer));
	setup_self_timer(announce, protoID, 30);
	announce_period = 10;
	last_period = announce_period;

	// one-hop broadcast address
	WLANAddr bcastAddr;
	setBcastAddr(&bcastAddr);

	queue_t* inBox = pargs->inBox;
	queue_t_elem elem;
	while(1){
		queue_pop(inBox, &elem);

		if(elem.type == LK_MESSAGE) {
			LKMessage recv_msg = elem.data.msg;
			receive_announcement(&recv_msg);

		} else if(elem.type == LK_TIMER) {
			send_announcement();

			// set-up new timer
			last_period *= 2;
			LKTimer_set(announce, last_period * 1000 * 1000 + (rand() % 2 * 1000 * 1000), 0);

		} else if(elem.type == LK_EVENT) {
			LKEvent recv_event = elem.data.event;

			// events come from discovery (neigh up) or fault detector (neigh down)
			if(recv_event.proto_origin == PROTO_DISCOV_CTR) {
				process_ev_neigh_up(&recv_event);
			} else if(recv_event.proto_origin == PROTO_FT_LB) {
				process_ev_neigh_down(&recv_event);
			} else {
				lk_log("GLS MEMBERSHIP", "WARNING", "Received unknow event");
			}
		} else if(elem.type == LK_REQUEST){
			LKRequest req = elem.data.request;
			if(req.request == REQUEST){
				if(req.request_type == GLS_QUERY_REQ) {
					process_req_query(&req);
				}
			}
		} else{
			lk_log("GLS MEMBERSHIP", "WARNING", "Got something unexpected");
		}
	}
}
