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

#ifndef DISPATCHER_LOGGER_H_
#define DISPATCHER_LOGGER_H_

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "core/utils/queue.h"
#include "core/proto_data_struct.h"
#include "core/lk_runtime.h"

#include "core/protos/dispatcher.h"

#include "lightkone.h"

void* dispatcher_logger_init(void* args);

#endif /* DISPATCHER_LOGGER_H_ */
