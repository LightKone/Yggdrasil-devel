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

#ifndef CONTROL_PROTOCOL_UTILS_H_
#define CONTROL_PROTOCOL_UTILS_H_

#include <unistd.h>

#define COMMAND_CHECK_CONTROL_STRUCTURE 0
#define COMMAND_CREATE_CONTROL_STRUCTURE 1
#define COMMAND_ALPHA 1
#define COMMAND_BETA 2
#define COMMAND_QUIT 66

int readfully(int fd, void* buf, int len);
int writefully(int fd, void* buf, int len);

#endif /* CONTROL_PROTOCOL_UTILS_H_ */
