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

#ifndef LK_UTILS_H_
#define LK_UTILS_H_

#include "lk_data_struct.h"
#include "lk_constants.h"

/*************************************************
 * Standart method return values
 *************************************************/
#define SUCCESS 0
#define FAILED 1

/*************************************************
 * Global Vars
 *************************************************/
int lk_error_code;

/*************************************************
 * Utilities
 *************************************************/

int nameToType(char* name);
char * wlan2asc(WLANAddr* addr, char str[]);
int str2wlan(char machine[], char human[]);
void mac_addr_n2a(char *mac_addr, unsigned char *arg);

/*************************************************
 * Auxiliary Functions
 *************************************************/

void setToAddress(WLANAddr *daddr, int ifindex, struct sockaddr_ll *to); //change or add a new to apply directly to LKMessage
char* getSSID(unsigned char *ie, int ielen);

/*************************************************
 * Verification Functions
 *************************************************/

int isLightkoneProtocolMessage(void* buffer, int bufferLen);
int isLightkoneProtocolTimer(void* buffer, int bufferLen);
char isMesh(unsigned char *ie, int ielen);
int existsNetwork(char* netName);

#endif /* LK_UTILS_H_ */
