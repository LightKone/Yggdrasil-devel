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

#include "discovery_CTR.h"

// proto args
static unsigned short protoID;
static time_t announce_period;

// state
static neighbors_ctr* neighs_ctr;
static LKTimer* announce;
static time_t last_period;

static void send_ev_neigh_up(const short origin_id, neigh_ctr* neigh) {
	LKEvent neighbor_up_event;

	neighbor_up_event.proto_origin = origin_id;
	neighbor_up_event.notification_id = DISC_CTR_NEIGHBOR_UP;
	neighbor_up_event.length = UUID_SIZE + CTR_SIZE;

	neighbor_up_event.payload = malloc(neighbor_up_event.length);

	memcpy(neighbor_up_event.payload, neigh->uuid, UUID_SIZE);
	memcpy(neighbor_up_event.payload + UUID_SIZE, &neigh->counter, CTR_SIZE);


	deliverEvent(&neighbor_up_event);

	free(neighbor_up_event.payload);
}

static void process_ev_neigh_down(const LKEvent* recv_event) {
	const unsigned char* recv_uuid = (const unsigned char*) recv_event->payload;

	int neigh_pos = find_neigh(neighs_ctr, recv_uuid);
	if (neigh_pos == -1) {
		lk_log("DISCOVERY_CTR", "GLOBAL CHAOS", "Trying to remove the dead");
		return;
	}

	// we remove the dead from the list
	remove_neigh(neighs_ctr, neigh_pos);

	// logging
	char k[12];
	memset(k, 0, 12);
	sprintf(k, "%d", neighs_ctr->known);
	lk_log("DISCOVERY_CTR", "NNEIGH", k);

	char s[33];
	memset(s, 0, 33);
	wlan2asc(&(neighs_ctr->list + neigh_pos)->addr, s);
	lk_log("DISCOVERY_CTR", "DELNEIGH", s);

	free(recv_event->payload);
}

static void process_ev_me_dead(const LKEvent* recv_event) {
	// process received data
	short recv_counter = -1;
	memcpy(&recv_counter, recv_event->payload, sizeof(short));

	if(recv_counter >= neighs_ctr->myctr){
		neighs_ctr->myctr = max(recv_counter + 1, neighs_ctr->myctr + 1); //reset anounce period. or send explicit anounce?


		neigh_ctr* me = malloc(sizeof(neigh_ctr));
		memcpy(me->uuid, neighs_ctr->myuuid, sizeof(uuid_t));
		me->counter = neighs_ctr->myctr;
		send_ev_neigh_up(protoID, me);
		free(me);

		//cancel the timer
		cancel_timer(announce);

		//setup the timer for new times
		LKTimer_set(announce, (rand() % last_period * 1000 * 1000) + 2 * 1000 * 1000, 0);
		last_period = announce_period;
	}

	free(recv_event->payload);
}

static void send_announcement() {
	// create the announcement msg
	LKMessage msg;
	push_msg_info(&msg, protoID);
	push_msg_uuid_ctr(&msg, neighs_ctr->myuuid, neighs_ctr->myctr);

	dispatch(&msg);
	lk_log("DISCOVERY_CTR", "ANUNCE", "");
}

static void process_lkmsg(LKMessage* recv_msg) {

	// process received data
	uuid_t recv_uuid;
	memcpy(recv_uuid, recv_msg->data, UUID_SIZE);

	short recv_counter = -1;
	memcpy(&recv_counter, recv_msg->data + UUID_SIZE, sizeof(short));

	// find if we already know the address
	int pos = find_neigh(neighs_ctr, recv_uuid);

	if(pos == -1) {
		char s[37];
		memset(s, 0, 37);
		uuid_unparse(recv_uuid, s);
		lk_log("DISCOVERY_CTR", "NEWNEIGH", s);

		// add new neighbour to view
		neigh_ctr* new_neigh = insert_neigh(neighs_ctr, recv_msg->srcAddr, recv_uuid, recv_counter);

		// notify the upper layers of the neighbor
		send_ev_neigh_up(protoID, new_neigh);


		char k[12];
		memset(k, 0, 12);
		sprintf(k, "%d", neighs_ctr->known);
		lk_log("DISCOVERY_CTR", "NNEIGH", k);

		//cancel the times
		cancel_timer(announce);

		//setup the timer for new times
		LKTimer_set(announce, (rand() % last_period * 1000 * 1000) + 2 * 1000 * 1000, 0);
		last_period = announce_period;

	} else if(neighs_ctr->list[pos].counter < recv_counter) {
		neigh_ctr* neigh = neighs_ctr->list + pos;
		neigh->counter = recv_counter;
		send_ev_neigh_up(protoID, neigh);
	}

}

void* discovery_Full_FT_CTR_init(void* arg) { //argument is the time for the anuncement
	proto_args* pargs = arg;

	protoID = pargs->protoID;
	announce_period = (long)pargs->protoAttr;

	// init neighbour list
	neighs_ctr = malloc(sizeof(neighbors_ctr));
	init_neighs_ctr(neighs_ctr, pargs->myuuid);

	// create the announcement message, and fill it with the stuff that it needs
	send_announcement();

	neigh_ctr* me = malloc(sizeof(neigh_ctr));
	memcpy(me->uuid, neighs_ctr->myuuid, sizeof(uuid_t));
	me->counter = neighs_ctr->myctr;
	send_ev_neigh_up(protoID, me);
	free(me);

	// create the announcement timer
	announce = malloc(sizeof(LKTimer));
	setup_self_timer(announce, protoID, announce_period);
	last_period = announce_period;

	// start receiving and processing
	queue_t* inBox = pargs->inBox;
	queue_t_elem elem;
	while(1){

		memset(&elem, 0, sizeof(elem));

		queue_pop(inBox, &elem);

		if(elem.type == LK_TIMER) {
			// time to announce myself to neighbours

			// announce i'm alive
			send_announcement();

			// set-up new timer
			last_period *= 2;
			LKTimer_set(announce,  last_period * 1000 * 1000 + (rand() % 2 * 1000 * 1000), 0);

		} else if(elem.type == LK_MESSAGE) {
			LKMessage recv_msg = elem.data.msg;
			char s[33];
			memset(s, 0, 33);
			lk_log("DISCOVERY CTR", "RECVANUNCE", wlan2asc(&elem.data.msg.srcAddr, s));

			process_lkmsg(&recv_msg);

		} else if(elem.type == LK_EVENT) {

			LKEvent recv_event = elem.data.event;

			if (recv_event.proto_origin == PROTO_FT_LB) {
				process_ev_neigh_down(&recv_event);
			} else if (recv_event.proto_origin == PROTO_GLOBAL) {
				lk_log("DISCOVERY CTR", "DEBUG", "Received event from global");
				process_ev_me_dead(&recv_event);
			}

		} else {
			char s[100];
			sprintf(s, "Got weird thing, type = %u", elem.type);
			lk_log("DISCOVERY_CTR", "ALERT", s);
		}
	}
}
