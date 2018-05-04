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

#ifndef CONTROL_COMMAND_TREE_H_
#define CONTROL_COMMAND_TREE_H_

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>

#include <arpa/inet.h>

#include "lightkone.h"
#include "core/utils/queue_elem.h"
#include "core/lk_runtime.h"
#include "core/utils/queue.h"
#include "protos/control/control_discovery.h"
#include "control_protocol_utils.h"
#include "commands.h"

void* control_command_tree_init(void* arg);
void check_running_operations_completion();

#endif /* CONTROL_COMMAND_TREE_H_ */
