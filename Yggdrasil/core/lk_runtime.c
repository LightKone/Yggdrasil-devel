/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2017
 *********************************************************/

/**********************************************************
 * This is LK Runtime V0.1 (alpha) no retro-compatibility *
 * will be ensured across versions up to version 1.0      *
 **********************************************************/

#include "lk_runtime.h"
/**********************************************************
 * Private data structures
 **********************************************************/

typedef struct _thread_info {
	short id;
	void * (*proto)(void*);
	proto_args args;
	pthread_t* pthread;
}thread_info;

//List of events to which a protocol is interested
typedef struct _interest {
	short id;
	struct _interest* next;
}InterestList;

//List of events to which a protocol is interested
typedef struct _eventList {
	int nevents;
	InterestList** eventList;
}EventList;

typedef struct _proto {
	short protoID; //ids
	queue_t* protoQueue; //queues for each one
	queue_t* realProtoQueue; //real queues for each one
	pthread_t proto; //threads for each one
	EventList eventList;
}proto;

typedef struct _lk_protos_info { //lightkone protocols
	int nprotos; //how many
	proto* protos;
}lk_protos_info;

typedef struct _protos_info { //user defined protocols
	int nprotos; //how many
	proto* protos;
}protos_info;

typedef struct _app {
	short appID; //ids (starting from APP_0 (400))
	queue_t* appQueue; //queues for each one
	EventList eventList;
}app;

typedef struct _apps_info { //apps queues and ids
	int napps; //how many
	app* apps;
}apps_info;

/**********************************************************
 * Private global variables
 **********************************************************/

static lk_protos_info lk_proto_info;
static protos_info proto_info;
static apps_info app_info;

static int threads;
static thread_info* tinfo;

/**********************************************************
 * Global variables
 **********************************************************/

static Channel ch;
static WLANAddr bcastAddress;

static uuid_t myuuid;

/*********************************************************
 * Info struct utilities and manipulation
 *********************************************************/

static int registered(short protoID, proto* protos, int size){
	int i;
	for(i = 0; i < size; i ++){
		if(protoID == protos[i].protoID)
			return 1;
	}

	return 0;
}

static proto* findProto(short protoID, proto* protos, int size){
	int i;
	for(i = 0; i < size; i ++){
		if(protoID == protos[i].protoID)
			return protos + i;
	}
	return NULL;
}

static EventList* getEventList(short id){
	EventList* eventList = NULL;
	if(id >= APP_0){
		id = id - APP_0;
		if(id >= app_info.napps)
			return eventList;
		eventList = &app_info.apps[id].eventList;
	}
	else if(id >= PROTO_0) {
		proto* proto;
		if((proto = findProto(id, proto_info.protos, proto_info.nprotos)) == NULL)
			return eventList;
		eventList = &proto->eventList;
	}
	else{
		proto* proto;
		if((proto = findProto(id, lk_proto_info.protos, lk_proto_info.nprotos)) == NULL)
			return eventList;
		eventList = &proto->eventList;
	}
	return eventList;
}

static queue_t* getProtoQueue(short id){
	queue_t* dropbox = NULL;
	if(id >= APP_0){
		id = id - APP_0;
		if(id >= app_info.napps)
			return NULL;
		dropbox = app_info.apps[id].appQueue;
	}
	else if(id >= PROTO_0) {
		proto* proto;
		if((proto = findProto(id, proto_info.protos, proto_info.nprotos)) == NULL)
			return NULL;
		dropbox = proto->protoQueue;
	}
	else{
		proto* proto;
		if((proto = findProto(id, lk_proto_info.protos, lk_proto_info.nprotos)) == NULL)
			return NULL;
		dropbox = proto->protoQueue;
	}
	if(dropbox == NULL){
		lk_log("LK_RUNTIME", "PANIC", "drop box is null on getProtoqueue");
		lk_logflush();
		exit(1);
	}
	return dropbox;
}

static int filter(LKMessage* msg) {
	if(memcmp(msg->destAddr.data, ch.hwaddr.data, WLAN_ADDR_LEN) == 0 || memcmp(msg->destAddr.data, bcastAddress.data, WLAN_ADDR_LEN) == 0)
		return 0;
	return 1;
}


/*********************************************************
 * Initialize mandatory structs
 *********************************************************/

static int totalNumberOfProtocols = 0;
static int maxLKProtos = 0;
static int maxProtos = 0;
static int maxApps = 0;

static int registThread(short protoID, pthread_t *thread, void * (*protoFunc)(void*) , queue_t* queue, void* protoAttr){
	if(threads > totalNumberOfProtocols)
		return FAILED;

	(tinfo+threads)->id = protoID;
	(tinfo+threads)->pthread = thread;
	(tinfo+threads)->proto = protoFunc;
	(tinfo+threads)->args.inBox = queue;
	(tinfo+threads)->args.protoAttr = protoAttr;
	memcpy((tinfo+threads)->args.myuuid, myuuid, sizeof(uuid_t));
	(tinfo+threads)->args.protoID = protoID;

	threads ++;


	return SUCCESS;
}

static int init_lk_proto_info(int totalProtos) {

	lk_proto_info.nprotos = 2; //start with two protocols
	lk_proto_info.protos = malloc(sizeof(proto) * totalProtos);

	proto* proto = lk_proto_info.protos;
	proto->protoID = PROTO_DISPATCH; //All will have the dispatcher to send and receive messages

	proto->realProtoQueue = queue_init(proto->protoID, DEFAULT_QUEUE_SIZE); //dispatcher

	proto->protoQueue = proto->realProtoQueue;

	proto->eventList.nevents = 0;
	proto->eventList.eventList = NULL;

	if(registThread(PROTO_DISPATCH, &proto->proto, &dispatcher_init, proto->realProtoQueue, &ch) == FAILED)
		return FAILED;
	proto++;

	proto->protoID = PROTO_TIMER; //All will have the timer for event management

	proto->realProtoQueue = queue_init(proto->protoID, DEFAULT_QUEUE_SIZE); //timer
	proto->protoQueue = proto->realProtoQueue;

	proto->eventList.nevents = 0;
	proto->eventList.eventList = NULL;

	return registThread(PROTO_TIMER, &proto->proto, &timer_init, proto->realProtoQueue, NULL);

}

/**********************************************************
 * Initialize runtime
 **********************************************************/

static void processTerminateSignal(int sig, siginfo_t *si, void *uc) {
	lk_logflush();
	exit(0);
}

static void setupTerminationSignalHandler() {
	/* Establish handler for timer signal */
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = processTerminateSignal;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIG, &sa, NULL) == -1) {
		fprintf(stderr, "Unable to setup termination signal handler. Shutdown will not be clean\n");
	}

}

int lk_runtime_init_static(NetworkConfig* ntconf, int n_lk_protos, int n_protos, int n_apps) {

	lk_loginit(); //Initialize logger

	createChannel(&ch, "wlan0");
	bindChannel(&ch);
	set_ip_addr(&ch);
	defineFilter(&ch, ntconf->filter);

	n_lk_protos += 2; //Add the dispatcher and timer to the protos

	totalNumberOfProtocols = n_lk_protos + n_protos;
	maxApps = n_apps;
	maxLKProtos = n_lk_protos;
	maxProtos = n_protos;

	setupTerminationSignalHandler();

	srand(time(NULL));   // should only be called once
	//genUUID(myuuid); //Use this line to generate a random uuid
	genStaticUUID(myuuid); //Use this to generate a static uuid based on the hostname

	tinfo = malloc(sizeof(thread_info) * totalNumberOfProtocols); //space for all protos
	threads = 0;

	if(init_lk_proto_info(n_lk_protos) == FAILED){ //Initialize the mandatory lightkone protocols (dispatcher and timer)
		lk_log("LK_RUNTIME", "SETUP ERROR", "Failed to setup mandatory lk protocols dispatcher and timer");
		lk_logflush();
		exit(1);
	}

	//Ready the structure to have the information about Apps
	if(maxApps > 0)
		app_info.apps = malloc(sizeof(app) * maxApps);
	else
		app_info.apps = NULL;

	app_info.napps = 0;

	//Ready the structure to have information about user defined protocols
	if(maxProtos > 0)
		proto_info.protos = malloc(sizeof(proto) * n_protos);
	else
		proto_info.protos = NULL;

	proto_info.nprotos = 0;

	str2wlan((char*)bcastAddress.data, WLAN_BROADCAST);

	return SUCCESS;
}


int lk_runtime_init(NetworkConfig* ntconf, int n_lk_protos, int n_protos, int n_apps) {

	lk_loginit(); //Initialize logger

	if(setupSimpleChannel(&ch, ntconf) != SUCCESS){ //try to configure the physical device and open a socket on it
		lk_log("LK_RUNTIME", "SETUP ERROR", "Failed to setup channel for communication");
		lk_logflush();
		exit(1);
	}

	if(setupChannelNetwork(&ch, ntconf) != SUCCESS){ //try to connect to a network which was specified in the NetworkConfig structure
		lk_log("LK_RUNTIME", "SETUP ERROR", "Failed to setup channel network for communication");
		lk_logflush();
		exit(1);
	}

	n_lk_protos += 2; //Add the dispatcher and timer to the protos

	totalNumberOfProtocols = n_lk_protos + n_protos;
	maxApps = n_apps;
	maxLKProtos = n_lk_protos;
	maxProtos = n_protos;

	setupTerminationSignalHandler();

	srand(time(NULL));   // should only be called once
	//genUUID(myuuid); //Use this line to generate a random uuid
	genStaticUUID(myuuid); //Use this to generate a static uuid based on the hostname

	tinfo = malloc(sizeof(thread_info) * totalNumberOfProtocols); //space for all protos
	threads = 0;

	if(init_lk_proto_info(n_lk_protos) == FAILED){ //Initialize the mandatory lightkone protocols (dispatcher and timer)
		lk_log("LK_RUNTIME", "SETUP ERROR", "Failed to setup mandatory lk protocols dispatcher and timer");
		lk_logflush();
		exit(1);
	}

	//Ready the structure to have the information about Apps
	if(maxApps > 0)
		app_info.apps = malloc(sizeof(app) * maxApps);
	else
		app_info.apps = NULL;

	app_info.napps = 0;

	//Ready the structure to have information about user defined protocols
	if(maxProtos > 0)
		proto_info.protos = malloc(sizeof(proto) * n_protos);
	else
		proto_info.protos = NULL;

	proto_info.nprotos = 0;

	str2wlan((char*)bcastAddress.data, WLAN_BROADCAST);

	return SUCCESS;
}

const char* getChannelIpAddress() {
	if(ch.ip_addr[0] == '\0')
		set_ip_addr(&ch);

	return ch.ip_addr;

}

int lk_runtime_start() {

	int i = 0;
	pthread_attr_t patribute;

	while(i < threads){
		int err = 0;
		const int off = i;
#ifdef DEBUG
		char m[2000];
		memset(m, 0, 2000);
		sprintf(m,"threads to start: %d; on starting %d; starting thread with id %d\n",threads, i, (tinfo+off)->id);
		lk_log("LK_RUNTIME","INFO", m);
#endif
		pthread_attr_init(&patribute);
		err = pthread_create((tinfo+off)->pthread, &patribute, (tinfo+off)->proto, (void *)&((tinfo+off)->args));

		if(err != 0){
			switch(err){
			case EAGAIN: lk_log("LK_RUNTIME","PTHREAD ERROR","No resources to create thread"); lk_logflush(); exit(err);
			break;
			case EPERM: lk_log("LK_RUNTIME","PTHREAD ERROR","No permissions to create thread"); lk_logflush(); exit(err);
			break;
			case EINVAL: lk_log("LK_RUNTIME","PTHREAD ERROR","Invalid attributes on create thread"); lk_logflush(); exit(err);
			break;
			default:
				lk_log("LK_RUNTIME","PTHREAD ERROR","Unknown error on create thread"); lk_logflush(); exit(1);
			}
		}
		i = i + 1;
	}

	return SUCCESS;
}

static void configProto(proto* proto, short protoID, int max_event_type){
	proto->protoID = protoID;

	proto->realProtoQueue = queue_init(proto->protoID, DEFAULT_QUEUE_SIZE); //timer
	proto->protoQueue = proto->realProtoQueue;

	proto->eventList.nevents = max_event_type;

	if(max_event_type > 0)
		proto->eventList.eventList = malloc(sizeof(InterestList*)*max_event_type);
	else
		proto->eventList.eventList = NULL;

	int i = 0;
	for(; i < max_event_type; i++){
		proto->eventList.eventList[i] = NULL;
	}

}

int addLKProtocol(short protoID, void * (*protoFunc)(void*), void* protoAttr, int max_event_type) {
	if(!registered(protoID, lk_proto_info.protos, lk_proto_info.nprotos)) {
		int n = lk_proto_info.nprotos + 1;
		if(n > maxLKProtos)
			return FAILED;
		proto* proto = (lk_proto_info.protos + lk_proto_info.nprotos);
		configProto(proto, protoID, max_event_type);

		lk_proto_info.nprotos ++;

		return registThread(protoID, &proto->proto, protoFunc, proto->realProtoQueue, protoAttr);
	}

	return FAILED;
}


int addProtocol(short protoID, void * (*protoFunc)(void*), void* protoAttr, int max_event_type) {
	if(!registered(protoID, proto_info.protos, proto_info.nprotos)) {
		int n = proto_info.nprotos + 1;
		if(n > maxProtos)
			return FAILED;
		proto* proto = (proto_info.protos + proto_info.nprotos);
		configProto(proto, protoID, max_event_type);

		proto_info.nprotos ++;

		return registThread(protoID, &proto->proto, protoFunc, proto->realProtoQueue, protoAttr);
	}
	return FAILED;
}

short registerApp(queue_t** app_inBox, int max_event_type){

	short appID;

	int n = app_info.napps + 1;
	if(n > maxApps)
		return -1;

	app* app = (app_info.apps + app_info.napps);

	appID = APP_0 + app_info.napps;

	app->appID = appID;

	app->appQueue = queue_init(app->appID, DEFAULT_QUEUE_SIZE);

	app->eventList.nevents = max_event_type;
	if(max_event_type > 0)
		app->eventList.eventList = malloc(sizeof(InterestList*)*max_event_type);
	else
		app->eventList.eventList = NULL;
	int i = 0;
	for(; i < max_event_type; i++){
		app->eventList.eventList[i] = NULL;
	}

	(*app_inBox) = app->appQueue;

	app_info.napps ++;

	return appID;
}

int changeDispatcherFunction(void * (*dispatcher)(void*)) {
	//TODO
	tinfo->proto = dispatcher;

	return SUCCESS;
}

int updateProtoAttr(short protoID, void* protoAttr) {
	int i;
	for (i = 0; i < threads; i ++) {
		if((tinfo + i)->id == protoID){
			(tinfo + i)->args.protoAttr = protoAttr;
			return SUCCESS;
		}
	}
	return FAILED;
}


queue_t* interceptProtocolQueue(short protoID, short myID) {

	queue_t* ret = getProtoQueue(protoID);

	if(protoID >= APP_0){
		protoID = protoID - APP_0;
		app_info.apps[protoID].appQueue = getProtoQueue(myID);
	}
	else if(protoID >= PROTO_0) {
		findProto(protoID, proto_info.protos, proto_info.nprotos)->protoQueue = getProtoQueue(myID);
	}
	else
		findProto(protoID, lk_proto_info.protos, lk_proto_info.nprotos)->protoQueue = getProtoQueue(myID);

	return ret;
}

int registInterestEvents(short protoID, short* events, int nEvents, short myID) {
	EventList* eventList = getEventList(protoID);

	int i = 0;
	for(; i < nEvents; i++){
		if(events[i] < eventList->nevents){
			InterestList* interestLst = eventList->eventList[events[i]];
			if(interestLst != NULL){
				while(interestLst->next != NULL){
					interestLst = interestLst->next;
				}
				interestLst->next = malloc(sizeof(InterestList));
				interestLst->next->id = myID;
				interestLst->next->next = NULL;
			}else{
				eventList->eventList[events[i]] = malloc(sizeof(InterestList));
				eventList->eventList[events[i]]->id = myID;
				eventList->eventList[events[i]]->next = NULL;
			}
		}
	}
	return SUCCESS;
}

/********************************************************
 * Internal functions
 ********************************************************/

/********************************************************
 * Runtime functions
 ********************************************************/

int dispatch(LKMessage* msg) {

	queue_t_elem elem;
	elem.type = LK_MESSAGE;
	memcpy(&elem.data.msg, msg, sizeof(LKMessage));

	queue_push(lk_proto_info.protos->protoQueue, &elem);

	return SUCCESS;
}

int setupTimer(LKTimer* timer){
	queue_t_elem elem;
	elem.type = LK_TIMER;

	memcpy(&elem.data.timer, timer, sizeof(LKTimer));
#ifdef DEBUG
	char msg[100];
	memset(msg,0,100);
	sprintf(msg, "Setting up timer with proto source %u and proto dest %u at time %ld", timer->proto_origin, timer->proto_dest, timer->config.first_notification);
	lk_log("LKRUNTIME", "SETTING UP TIMER", msg);
#endif
	queue_push(getProtoQueue(PROTO_TIMER), &elem);
	return SUCCESS;
}

int cancelTimer(LKTimer* timer) {
	queue_t_elem elem;
	elem.type = LK_TIMER;

	timer->config.first_notification = 0;
	timer->config.repeat_interval_ms = 0;

	memcpy(&elem.data.timer, timer, sizeof(LKTimer));
#ifdef DEBUG
	char msg[100];
	memset(msg,0,100);
	sprintf(msg, "Setting up timer with proto source %u and proto dest %u at time %ld", timer->proto_origin, timer->proto_dest, timer->config.first_notification);
	lk_log("LKRUNTIME", "SETTING UP TIMER", msg);
#endif
	queue_push(getProtoQueue(PROTO_TIMER), &elem);
	return SUCCESS;
}

int deliver(LKMessage* msg) {

	short id = msg->LKProto;
	queue_t* dropBox = getProtoQueue(id);

	queue_t_elem elem;
	elem.type = LK_MESSAGE;
	memcpy(&elem.data.msg, msg, sizeof(LKMessage));

	if(dropBox == NULL)
		return FAILED;
	queue_push(dropBox, &elem);

	return SUCCESS;
}

int filterAndDeliver(LKMessage* msg) {

	if(filter(msg) == 0){
		short id = msg->LKProto;
		queue_t* dropBox = getProtoQueue(id);

		queue_t_elem elem;
		elem.type = LK_MESSAGE;
		memcpy(&elem.data.msg, msg, sizeof(LKMessage));

		if(dropBox == NULL)
			return FAILED;
		queue_push(dropBox, &elem);

		return SUCCESS;
	}
	return SUCCESS;
}

int deliverTimer(LKTimer* timer) {

	short id = timer->proto_dest;
	queue_t* dropBox = getProtoQueue(id);

	queue_t_elem elem;
	elem.type = LK_TIMER;
	memcpy(&elem.data.timer, timer, sizeof(LKTimer));

	if(dropBox == NULL)
		return FAILED;
	queue_push(dropBox, &elem);

	return SUCCESS;
}


int deliverEvent(LKEvent* event) {

	short id = event->proto_origin;
	short eventType = event->notification_id;

	EventList* evlst = getEventList(id);

	if(evlst->nevents > eventType) {
		InterestList* interestList = evlst->eventList[eventType];
		while(interestList != NULL){

			short pid = interestList->id;
			queue_t* dropBox = getProtoQueue(pid);
			if(dropBox != NULL) {
				queue_t_elem elem;
				elem.type = LK_EVENT;
				memcpy(&elem.data.event, event, sizeof(LKEvent));
				queue_push(dropBox, &elem);
				interestList = interestList->next;
			}
		}
	}else {
		lk_log("LK_RUNTIME", "DELIVER EVENT", "an event was sent that does not exist ignoring");
	}

	return SUCCESS;
}

int deliverRequest(LKRequest* req) {
	if(req->request == REQUEST){
		short id = req->proto_dest;
		queue_t* dropBox = getProtoQueue(id);

		queue_t_elem elem;
		elem.type = LK_REQUEST;
		memcpy(&elem.data.request, req, sizeof(LKRequest));

		if(dropBox == NULL)
			return FAILED;
		queue_push(dropBox, &elem);

		return SUCCESS;
	}
	return FAILED;
}

int deliverReply(LKRequest* res) {
	if(res->request == REPLY){
		short id = res->proto_dest;

#ifdef DEBUG
		char desc[200];
		bzero(desc, 200);
		sprintf(desc, "Got a reply to %d", id);
		lk_log("RUNTIME", "ALIVE", desc);
#endif
		queue_t* dropBox = getProtoQueue(id);

		queue_t_elem elem;
		elem.type = LK_REQUEST;
		memcpy(&elem.data.request, res, sizeof(LKRequest));

		if(dropBox == NULL)
			return FAILED;
		queue_push(dropBox, &elem);

		return SUCCESS;
	}
	return FAILED;
}

/********************************************************
 * Other useful read Functions
 ********************************************************/

char* getMyAddr(char* s2){
	return wlan2asc(&ch.hwaddr, s2);
}

void setMyAddr(WLANAddr* addr){
	memcpy(addr->data, ch.hwaddr.data, WLAN_ADDR_LEN);
}

double getValue() {
	const char* hostname = getHostname();
	return atoi(hostname+(strlen(hostname)-2));
}
