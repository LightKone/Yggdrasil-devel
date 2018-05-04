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
#ifndef LOGGER_BYTE_MSG_LOGGER_H_
#define LOGGER_BYTE_MSG_LOGGER_H_

#include <stdlib.h>
#include <string.h>

#include "../../core/lk_runtime.h"
#include "../../core/proto_data_struct.h"
#include "../../core/utils/queue.h"
#include "../../core/utils/queue_elem.h"

void* byte_logger_init(void* attr);

#endif /* LOGGER_BYTE_MSG_LOGGER_H_ */
