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

#include <netlink/genl/genl.h>
#include <linux/nl80211.h>
#include <linux/filter.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "lk_utils.h"
#include "lk_errors.h"

#ifndef LK_DATA_STRUCT_H_
#define LK_DATA_STRUCT_H_

#define WLAN_ADDR_LEN 6
#define LK_HEADER_LEN 3

#define DEFAULT_MTU 1500

#define CH_BROAD 0X001
#define CH_SENDTO 0x002
#define CH_SEND 0x003

#define IP_TYPE 0x3901
#define WLAN_BROADCAST "ff:ff:ff:ff:ff:ff"

#define AF_LK 0x4C4B50 //LKP
#define AF_LK_ARRAY {0x4C, 0x4B, 0x50}

#define AF_LKT 0x4C4B54 //LKT
#define AF_LKT_ARRAY {0x4C, 0x4B, 0x54}

#pragma pack(1)
// Structure for a holding a mac address
typedef struct _WLANAddr{
   // address
   unsigned char data[WLAN_ADDR_LEN];
} WLANAddr;

// Structure of a frame header
typedef struct _WLANHeader{
    // destination address
    WLANAddr destAddr;
    // source address
    WLANAddr srcAddr;
    // type
    unsigned short type;
} WLANHeader;

#define WLAN_HEADER_LEN sizeof(struct _WLANHeader)

// Structure of a Lightkone message
typedef struct _LKHeader{
   // Protocol family identifation
   unsigned char data[LK_HEADER_LEN];
} LKHeader;

#define MAX_PAYLOAD DEFAULT_MTU -WLAN_HEADER_LEN -LK_HEADER_LEN -sizeof(unsigned short)

// Lightkone Protocol physical layer message
typedef struct _LKPhyMessage{
  //Physical Level header;
  WLANHeader phyHeader;
  //Lightkone Protocol header;
  LKHeader lkHeader;
  //PayloadLen
  unsigned short dataLen;
  //Payload
  char data[MAX_PAYLOAD];
} LKPhyMessage;

#pragma pack()

typedef struct _phy {
  int id;
  short modes[NUM_NL80211_IFTYPES];
  struct _phy* next;
} Phy;

typedef struct _interface {
	int id;
	char* name;
	int type;
	struct _interface* next;
} Interface;

typedef struct _Channel {
    // socket descriptor
    int sockid;
    // interface index
    int ifindex;
    // mac address
    WLANAddr hwaddr;
    // maximum transmission unit
    int mtu;
    //ip address (if any)
    char ip_addr[INET_ADDRSTRLEN];
} Channel;

typedef struct _Mesh {
	char pathSelection;
	char pathSelectionMetric;
	char congestionControl;
	char syncMethod;
	char authProtocol;
} Mesh;

typedef struct _Network {
	char* ssid;
	char mac_addr[20];
	char isIBSS;
	Mesh* mesh_info;
	int freq;
	struct _Network* next;
} Network;


/*************************************************
 * Structure to configure the network device
 ************************************************/
typedef struct _NetworkConfig {
	int type; //type of the required network (IFTYPE)
	int freq; //frequency of the signal
	int nscan; //number of times to perform network scans
	short mandatoryName; //1 if must connect to named network,; 0 if not
	char* name; //name of the network to connect
	struct sock_filter* filter; //filter for the network
	Interface* interfaceToUse; //interface to use
} NetworkConfig;
/*************************************************
 * Error codes (moved to lk_errors.h)
 *************************************************/
//#define NO_IF_INDEX_ERR 1
//#define NO_IF_ADDR_ERR 2
//#define NO_IF_MTU_ERR 3

/*************************************************
 * Global Vars
 *************************************************/
Network* knownNetworks;

/*************************************************
 * Data structure manipulation
 *************************************************/

/*************************************************
 * Phy
 *************************************************/
Phy* alloc_phy();
Phy* free_phy(Phy* var);
void free_all_phy(Phy* var);
int phy_count(Phy* var);
Phy* clone_phy(Phy* var);

/*************************************************
 * Interface
 *************************************************/
Interface* alloc_interface();
Interface* alloc_named_interface(char* name);
Interface* free_interface(Interface* var);
void free_all_interfaces(Interface* var);

/*************************************************
 * Network
 *************************************************/
Network* alloc_network(char* netName);
Network* free_network(Network* net);

void fillMeshInformation(Mesh* mesh_info, unsigned char *ie, int ielen);

/*************************************************
 * NetworkConfig
 *************************************************/
NetworkConfig* defineNetworkConfig(char* type, int freq, int nscan, short mandatory, char* name, const struct sock_filter* filter);

/*************************************************
 * LKMessage
 *************************************************/
int initLKPhyMessage(LKPhyMessage *msg); //initializes an empty payload lkmessage
int initLKPhyMessageWithPayload(LKPhyMessage *msg, char* buffer, short bufferlen); //initializes a non empty payload lkmessage

int addPayload(LKPhyMessage *msg, char* buffer);
int deserializeLKPhyMessage(LKPhyMessage *msg, unsigned short msglen, void* buffer, int bufferLen);


/*************************************************
 * Channel
 *************************************************/
int createChannel(Channel* ch, char* interface);
int bindChannel(Channel* ch);

#endif /* LK_DATA_STRUCT_H_ */
