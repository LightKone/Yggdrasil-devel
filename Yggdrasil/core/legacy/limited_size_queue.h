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

#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <uuid/uuid.h>

#include "core/utils/queue_elem.h"
#include "lightkone.h"

#include "core/proto_data_struct.h"

typedef struct _lsqueue_t lsqueue_t;

void lsqueue_init(lsqueue_t* q, unsigned short pid, unsigned short capacity);
void lsqueue_destroy(lsqueue_t* q);
void lsqueue_pop(lsqueue_t* q, queue_t_elem* elem);
void lsqueue_push(lsqueue_t* q, queue_t_elem* elem);
short lsqueue_size(lsqueue_t* q, queue_t_elem_type type);
short lsqueue_totalSize(lsqueue_t* q);
void destroy_inner_lsqueue(lsqueue_t* q, queue_t_elem_type type);
int lsqueue_try_timed_pop(lsqueue_t* q, struct timespec* uptotime, queue_t_elem* elem);
void lsqueue_try_push(lsqueue_t* q, queue_t_elem* elem);

#endif /* QUEUE_H_ */
