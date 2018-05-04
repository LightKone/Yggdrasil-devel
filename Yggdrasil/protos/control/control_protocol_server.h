/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2018
 *********************************************************/

#ifndef CONTROL_PROTOCOL_SERVER_H_
#define CONTROL_PROTOCOL_SERVER_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "lightkone.h"
#include "core/lk_runtime.h"
#include "core/utils/utils.h"
#include "core/proto_data_struct.h"
#include "protos/control/control_protocol_utils.h"
#include "protos/control/control_command_tree.h"

void * control_protocol_server_init(void * args);

#endif /* CONTROL_PROTOCOL_SERVER_H_ */
