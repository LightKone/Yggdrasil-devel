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

#ifndef CORE_UTILS_H_
#define CORE_UTILS_H_

#include <stdio.h>
#include <uuid/uuid.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "core/proto_data_struct.h"
#include "lightkone.h"


#define min(x,y) (((x) < (y))? (x) : (y))
#define max(x,y) (((x) > (y))? (x) : (y))


const char* getHostname();

void lk_loginit();
void lk_log(char* proto, char* event, char* desc);
void lk_logflush();

void genUUID(uuid_t id);
void genStaticUUID(uuid_t id);
int setDestToAddr(LKMessage* msg, char* addr); //WLAN addr
int setDestToBroadcast(LKMessage* msg);

/**
 * Fill the paramenter addr with the broadcast address
 * @param addr A pointer to the WLANAddr that was requested
 */
void setBcastAddr(WLANAddr* addr);

#endif /* CORE_UTILS_H_ */
