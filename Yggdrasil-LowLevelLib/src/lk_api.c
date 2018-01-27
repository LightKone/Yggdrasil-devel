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

#include "lk_api.h"

/*********************************************************
 * Setup
 *********************************************************/
int setupSimpleChannel(Channel* ch, NetworkConfig* ntconf){ //more parameters could be added to support multiple types of decision making on the interface to operate on

	extern int nl_error;

	int type = ntconf->type;
	if(type == -1){
		printf("Type %d does not exist", ntconf->type); //TODO better error message
		return FAILED;
	}

	Phy* phys = NULL;

	int r = listDevices(&phys, type); //get all wireless devices that support type and put them in phys
	if(r!=0) { printNetlinkError(r); fprintf(stderr, "\n"); exit(1);}

	int numberOfPhysicalDevices = phy_count(phys);

	Interface** interfaces = (Interface**) malloc(sizeof(Interface*) * numberOfPhysicalDevices);
	memset(interfaces, 0, sizeof(Interface*) * numberOfPhysicalDevices);

	Interface* interfaceToUse = NULL;

	Phy* temp = phys;
	Interface* it = NULL;
	//int success = 0;
	int phyPos = 0;
	while(temp != NULL) { //for each wireless device in phys

		r = listInterfaces(&(interfaces[phyPos]), temp);
		if(r!=0) { printNetlinkError(nl_error); fprintf(stderr, "\n"); exit(2);}

		it = interfaces[phyPos];
		while(it != NULL) { // for each interface in phy

			//check if the interface is of type
			if(it->type == type) { //found useful interface (first one is the best one?)
				interfaceToUse = it;

			} else { //if not, for now, skip all the interfaces that are connected
				Channel chtmp;
				r = createChannel(&chtmp, it->name);

				int interfaceUp = checkInterfaceUP(&chtmp, it->name);
				int interfaceConnected = checkInterfaceConnected(&chtmp, it->name);

				if(interfaceUp != 0 && interfaceUp != -1 && interfaceConnected != 0 && interfaceConnected != -1) {
					fprintf(stderr, "Skipping device %d as interface %s is connected\n",temp->id,it->name);
					free_all_interfaces(interfaces[phyPos]);
					interfaces[phyPos] = NULL;
					break;
				}
			}
			if(interfaceToUse != NULL)
				break;

			it = it->next;
		}
		if(interfaceToUse != NULL)
			break;

		phyPos++;
		temp = temp->next;
	}

	if(interfaceToUse == NULL) { //case there was no configured interface
		//Need to create an adequate interface in one of the devices.
		temp = phys;
		phyPos = 0;
		int use_this_interface = 1;
		while(temp != NULL) {
			it = interfaces[phyPos];

			if(it == NULL) {
				//This physical device has an active and connected interface... move along...
				phyPos++;
				temp = temp->next;
				fprintf(stderr,"Skipping device %d as there are active interfaces\n",temp->id);
				continue;
			}

			use_this_interface = 1;
			while(it != NULL) { //find available wireless device to create new interface
				Channel chtmp;
				createChannel(&chtmp, it->name);

				int sucess = 0;
				int interfaceUp = checkInterfaceUP(&chtmp, it->name);

				if( interfaceUp != 0 && interfaceUp != -1) {
					sucess = setInterfaceDOWN(&chtmp, it->name);
				}
				if(sucess < 0) {
					fprintf(stderr,"Skipping device %d as interface %s could not be put DOWN\n",temp->id,it->name);
					use_this_interface = 0;
					break;
				}
				it = it->next;
			}

			if(use_this_interface != 0) {
				Interface* newIte = alloc_named_interface("lk_if"); //TODO to be changed if necessary to create arbitrary interfaces
				newIte->type = type;
				newIte->next = NULL;
				r = createInterface(temp, newIte);

				if(r!=0) { fprintf(stderr, "Could not create interface %s\n", newIte->name); printNetlinkError(nl_error); fprintf(stderr, "\n");}

				else { //double check if interface was created
					free_all_interfaces(interfaces[phyPos]);
					r = listInterfaces(&(interfaces[phyPos]), temp);

					if(r!=0) { fprintf(stderr, "Could not re-list interfaces after creation\n"); printNetlinkError(nl_error); fprintf(stderr, "\n"); exit(2);}

					it = interfaces[phyPos];
					while(it != NULL && strcmp(it->name,newIte->name) != 0)
						it = it->next;

					interfaceToUse = it;

					if(it == NULL) { fprintf(stderr, "Error listing the newly created interface\n");}
				}
			}

			if(interfaceToUse != NULL)
				break;
			phyPos++;
			temp = temp->next;
		}
	}

	if(interfaceToUse == NULL && numberOfPhysicalDevices > 0) {
		fprintf(stderr, "No suitable radio device was found to create the required interface, attempting to reconfigure existing one\n");
		r = setInterfaceType(interfaces[0]->id, type);
		if(r == 0) {
			interfaceToUse = interfaces[0];//TODO should be a parameter
		}else{
			fprintf(stderr, "Failed to reconfigure interface, exiting\n"); exit(100);
		}
	} else if(interfaceToUse == NULL) {
		fprintf(stderr, "No suitable radio device was found to create the required interface\n"); exit(100);
	}

	r = createChannel(ch, interfaceToUse->name);
	if(r != 0) {
		printError(r); fprintf(stderr,"\n"); exit(10);
	}
	if(checkInterfaceUP(ch, interfaceToUse->name) <= 0)
		setInterfaceUP(ch, interfaceToUse->name);

	ntconf->interfaceToUse = interfaceToUse;
	return SUCCESS;
}

int setupChannelNetwork(Channel* ch, NetworkConfig* ntconf) {
	int i = 0;
	int r;
	Interface* interfaceToUse = ntconf->interfaceToUse;
	Network* nt = NULL;
	for(; i < ntconf->nscan; i++) {
		Network* nets = scanNetworks(ch);
		nt = nets;
		while(nt != NULL) {
			if(ntconf->type == IFTYPE_ADHOC && nt->isIBSS == 1){
				if(ntconf->mandatoryName && ntconf->freq == 0){
					if(strcmp(ntconf->name, nt->ssid))
						break;
				} else if(ntconf->freq != 0){
					if(ntconf->freq == nt->freq)
						break;
				}
				else
					break;
			}
			else if(ntconf->type == IFTYPE_MESH && nt->mesh_info != NULL){
				if(ntconf->mandatoryName && ntconf->freq == 0){
					if(strcmp(ntconf->name, nt->ssid))
						break;
				} else if(ntconf->freq != 0){
					if(ntconf->freq == nt->freq)
						break;
				}
				else
					break;
			}
			nt = nt->next;
		}
		if(nt != NULL) break;
	}
	if(nt == NULL) {
		nt = alloc_network(ntconf->name);
		if(ntconf->freq != 0)
			nt->freq = ntconf->freq;
		else
			nt->freq = DEFAULT_FREQ;
	}

	int connectedInterface = checkInterfaceConnected(ch, interfaceToUse->name);
	if(connectedInterface != 0) {
		if(ntconf->type == IFTYPE_ADHOC){
			r = leaveAdHoc(interfaceToUse);
			if(r!=0) { fprintf(stderr, "Error leaving previously joined AdHoc network: "); printNetlinkError(nl_error); fprintf(stderr, "\n");}
		}
		else if(ntconf->type == IFTYPE_MESH){
			r = leaveMesh(interfaceToUse);
			if(r!=0) { fprintf(stderr, "Error leaving previously joined Mesh network: "); printNetlinkError(nl_error); fprintf(stderr, "\n");}
		}
	}

	if(ntconf->type == IFTYPE_ADHOC){
		r = connectToAdHoc(interfaceToUse, nt);
		if(r!=0) { fprintf(stderr,"Error connecting: "); printNetlinkError(nl_error); fprintf(stderr, "\n"); exit(5); }
		fprintf(stderr, "Connect successful to AdHoc %s\n", nt->ssid);
	}
	else if(ntconf->type == IFTYPE_MESH){
		r = connectToMesh(interfaceToUse, nt);
		if(r!=0) { fprintf(stderr,"Error connecting: "); printNetlinkError(nl_error); fprintf(stderr, "\n"); exit(5); }
		fprintf(stderr, "Connect successful to Mesh %s\n", nt->ssid);
	}

	r = bindChannel(ch);

	set_ip_addr(ch);
	if(r!=0) {fprintf(stderr, "Error binding channel: %s\n", strerror(r)); exit(130);}

	if(ntconf->filter != NULL){
		r = defineFilter(ch, ntconf->filter);
		if(r!=0) {fprintf(stderr, "Error defining socket filter: %s\n", strerror(r));}
	}
	return SUCCESS;
}

/*********************************************************
 * Basic I/O
 *********************************************************/

int chsend(Channel* ch, LKPhyMessage* message) {
	// send a frame

	memcpy(message->phyHeader.srcAddr.data, ch->hwaddr.data, WLAN_ADDR_LEN);

	struct sockaddr_ll to = {0};
	setToAddress(&message->phyHeader.destAddr, ch->ifindex, &to);

	int sent=sendto(
			ch->sockid,
			message, (WLAN_HEADER_LEN+LK_HEADER_LEN+(sizeof(unsigned short))+message->dataLen), //sizeof(LKMessage)
			0,
			(struct sockaddr*) &to, sizeof(to));

	if(sent < 0) {
		return CHANNEL_SENT_ERROR;
	}


#ifdef DEBUG
	fprintf(stderr,"Transmitted %d bytes\n", sent);
#endif
	return sent;
}

// Send
int chsendTo(Channel* ch, LKPhyMessage* message, char* addr) {

	memcpy(message->phyHeader.destAddr.data, addr, WLAN_ADDR_LEN);

	return chsend(ch, message);
}

// Send
int chbroadcast(Channel* ch, LKPhyMessage* message) {

	char mcaddr[WLAN_ADDR_LEN];
	str2wlan(mcaddr, WLAN_BROADCAST); //translate addr to machine addr

	return chsendTo(ch, message, mcaddr);
}


// Receive
int chreceive(Channel* ch, LKPhyMessage* message) {
	struct sockaddr_ll from;
	socklen_t fromlen=sizeof(struct sockaddr_ll);

	memset(message, 0, DEFAULT_MTU);//sizeof(LKMessage));
	// wait and receive a frame
	int recv = recvfrom(ch->sockid, message, DEFAULT_MTU,//sizeof(LKMessage),
			0, (struct sockaddr *) &from, &fromlen);

	if(recv < 0) return CHANNEL_RECV_ERROR;

#ifdef DEBUG
	fprintf(stderr,"Received %d bytes\n", recv);
#endif
	return recv;
}
