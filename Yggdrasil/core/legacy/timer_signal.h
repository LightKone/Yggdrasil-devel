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

#ifndef TIMER_H_
#define TIMER_H_

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include "core/utils/queue.h"
#include "lightkone.h"
#include "core/proto_data_struct.h"
#include "core/lk_runtime.h"


#define CLOCKID CLOCK_REALTIME
#define SIG_TS SIGUSR1
//SIGRTMIN

/* Not elegant at all...*/
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)



void* timer_signal_init(void* args);


#endif /* TIMER_H_ */
