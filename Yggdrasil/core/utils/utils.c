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

#include "utils.h"

static char* hostname;
static pthread_mutex_t loglock;

const char* getHostname() {
	return hostname;
}

void lk_loginit(){
	pthread_mutexattr_t atr;
	pthread_mutexattr_init(&atr);
	pthread_mutex_init(&loglock, &atr);

	FILE* f = fopen("/etc/hostname","r");
	if(f > 0) {
		struct stat s;
		if(fstat(fileno(f), &s) < 0){
			perror("FSTAT");
		}
		hostname = malloc(s.st_size+1);
		if(hostname == NULL) {

			lk_log("LK_RUNTIME", "INIT", "Hostname is NULL");
			lk_logflush();
			exit(1);
		}

		memset(hostname, 0, s.st_size+1);
		if(fread(hostname, 1, s.st_size, f) <= 0){
			if(s.st_size < 8) {
				free(hostname);
				hostname = malloc(9);
				memset(hostname,0,9);
			}
			int r = rand() % 10000;
			sprintf(hostname, "host%04d", r);
		}else{

			int i;
			for(i = 0; i < s.st_size + 1; i++){
				if(hostname[i] == '\n') {
					hostname[i] = '\0';
					break;
				}
			}
		}
	} else {

		hostname = malloc(9);
		memset(hostname,0,9);

		int r = rand() % 10000;
		sprintf(hostname, "host%04d", r);
	}
	lk_log("LK_RUNTIME", "INIT", "Initialized logging");
}

void lk_log(char* proto, char* event, char* desc){
	char buffer[26];
	struct tm* tm_info;

	struct timeval tv;
	gettimeofday(&tv,NULL);
	pthread_mutex_lock(&loglock);

	tm_info = localtime(&tv.tv_sec);

	strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);

	printf("<%s> TIME: %s %ld :: [%s] : [%s] %s\n", hostname, buffer, tv.tv_usec, proto, event, desc);
	//printf("<%s> TIME: %ld %ld :: [%s] : [%s] %s\n", hostname, tv.tv_sec, tv.tv_usec, proto, event, desc);
	pthread_mutex_unlock(&loglock);
}

void lk_logflush() {
	char buffer[26];
	struct tm* tm_info;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	tm_info = localtime(&tv.tv_sec);

	strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
	printf("<%s> TIME: %s %ld :: [%s] : [%s] %s\n", hostname, buffer, tv.tv_usec, "LK_RUNTIME", "QUIT", "Stopping process");
	//printf("<%s> TIME: %ld %ld :: [%s] : [%s] %s\n", hostname, tv.tv_sec, tv.tv_usec, "LK_RUNTIME", "QUIT", "Stopping process");
	fflush(stdout);
}

void genStaticUUID(uuid_t id) {
	char* uuid_txt = malloc(37);
	memset(uuid_txt, 0, 37);
	sprintf(uuid_txt, "66600666-1001-1001-1001-0000000000%s", hostname+(strlen(hostname)-2));
	if(uuid_parse(uuid_txt, id) != 0)
		genUUID(id);
	else
		lk_log("UTILS", "GEN STATIC UUID", uuid_txt);
	free(uuid_txt);
}

void genUUID(uuid_t id){
	int tries = 0;
	while(uuid_generate_time_safe(id) < 0 && tries < 3){
		tries ++;
	}
}

int setDestToAddr(LKMessage* msg, char* addr){

	memcpy(msg->destAddr.data, addr, WLAN_ADDR_LEN);
	return SUCCESS;
}

int setDestToBroadcast(LKMessage* msg) {
	char mcaddr[WLAN_ADDR_LEN];
	str2wlan(mcaddr, WLAN_BROADCAST); //translate addr to machine addr
	return setDestToAddr(msg, mcaddr);
}

void setBcastAddr(WLANAddr* addr){
	str2wlan((char*) addr->data, WLAN_BROADCAST);
}

