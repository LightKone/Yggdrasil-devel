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

#include "lk_errors.h"

void printError(int errorCode) {
	switch(errorCode) {
	case NO_IF_INDEX_ERR:
		fprintf(stderr, "Could not extract the id for the wireless interface: %s",strerror(errno));
		break;
	case NO_IF_ADDR_ERR:
		fprintf(stderr, "Could not extract the physical address of the interface: %s",strerror(errno));
		break;
	case NO_IF_MTU_ERR:
		fprintf(stderr, "Could not extract the MTU of the interface: %s",strerror(errno));
		break;
	case SOCK_BIND_ERR:
		fprintf(stderr, "Could not bind the socket adequately: %s",strerror(errno));
		break;
	case CHANNEL_SENT_ERROR:
		fprintf(stderr, "Error transmitting message: %s",strerror(errno));
		break;
	case CHANNEL_RECV_ERROR:
		fprintf(stderr, "Error receiving message: %s",strerror(errno));
		break;
	default:
		fprintf(stderr, "An error has occurred within the COMM layer: %s",strerror(errno));
		break;
	}
}

void printNetlinkError(int netlinkErrorCode) {
	switch(netlinkErrorCode) {
	case NETCTL_PHYLST_ERROR:
		fprintf(stderr, "Error accessing the list of physical interfaces: %s",nl_geterror(-netlinkErrorCode));
		break;
	case NETCTL_INTERFACE_LIST_ERROR:
		fprintf(stderr, "Error accessing the list of interfaces: %s",nl_geterror(-netlinkErrorCode));
		break;
	case NETCTL_INTERFACE_CREATE_ERROR:
		fprintf(stderr, "Error creating an interface: %s",nl_geterror(-netlinkErrorCode));
		break;
	case NETCTL_INTERFACE_DELETE_ERROR:
		fprintf(stderr, "Error deleting an interfaces: %s",nl_geterror(-netlinkErrorCode));
		break;
	case NETCTL_MESH_JOIN_ERROR:
		fprintf(stderr, "Error joining a mesh network: %s", nl_geterror(-netlinkErrorCode));
		break;
	case NETCTL_ADHOC_JOIN_ERROR:
		fprintf(stderr, "Error joining an adhoc network: %s", nl_geterror(-netlinkErrorCode));
		break;
	case NETCTL_ADHOC_LEAVE_ERROR:
		fprintf(stderr, "Error leaving an adhoc network: %s", nl_geterror(-netlinkErrorCode));
		break;
	case NETCTL_MESH_LEAVE_ERROR:
		fprintf(stderr, "Error leaving a mesh network: %s", nl_geterror(-netlinkErrorCode));
		break;
	default:
		fprintf(stderr, "An error has occurred within the MONITOR/NET_CTRL layer: %s",nl_geterror(-netlinkErrorCode));
		break;
	}
}


