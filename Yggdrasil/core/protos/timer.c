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

#include "timer.h"

typedef struct timer_elem_ {
	LKTimer* timer;
	struct timer_elem_* next;
} timer_elem;

static unsigned short pendingTimersSize;
static timer_elem* pendingTimers;
static queue_t* inBox;

static void processExpiredTimers() {

	time_t now = time(NULL);

	while(pendingTimers != NULL && (pendingTimers->timer->config.first_notification - now) <= 0 ) {

		//Remove it from the queue
		timer_elem* elem = pendingTimers;
		pendingTimers = elem->next;
		elem->next = NULL;

		//Check if this is a repeatable timer, if so reinsert
		if(elem->timer->config.repeat_interval_ms > 0) {
			timer_elem* tmp = malloc(sizeof(timer_elem));
			tmp->timer = malloc(sizeof(LKTimer));
			tmp->next = NULL;
			memcpy(tmp->timer, elem->timer, sizeof(LKTimer));

			int increment = tmp->timer->config.repeat_interval_ms/1000;
			tmp->timer->config.first_notification = time(NULL) + (increment > 0 ? increment : 1);
			if(pendingTimers == NULL || (pendingTimers->timer->config.first_notification > tmp->timer->config.first_notification)) {
				tmp->next = pendingTimers;
				pendingTimers = tmp;
			} else {
				timer_elem* current = pendingTimers;
				while(current->next != NULL) {
					if(current->next->timer->config.first_notification > tmp->timer->config.first_notification) {
						break;
					}
					current = current->next;
				}
				tmp->next = current->next;
				current->next = tmp;
			}
		} else {
			pendingTimersSize--;
		}

		deliverTimer(elem->timer);
		free(elem->timer);
		free(elem);

		//Update the time, just for compensation of the time spent in the delivery... (others might have expired...)
		time(&now);
	}
}

void* timer_init(void* args) {
	proto_args* pargs = args;
	inBox = pargs->inBox;
	//destroy_inner_queue(inBox, LK_MESSAGE); //We will never receive LKMessages here

	pendingTimersSize = 0;
	pendingTimers = NULL;

	queue_t_elem el;

	while(1){

		int gotRequest = 1;
		if(pendingTimers == NULL) {

			queue_pop(inBox, &el);
		} else {
			struct timespec timeout;
			timeout.tv_sec = pendingTimers->timer->config.first_notification;
			timeout.tv_nsec = 0;

			gotRequest = queue_try_timed_pop(inBox, &timeout, &el);
		}

		if(gotRequest == 1) {

			//If this is not something we understand we stop here
			//should never have
			if(el.type != LK_TIMER) {
				lk_log("TIMER", "ERROR", "received something on queue that is not a Timer.");
				continue;
			}

			if(el.data.timer.proto_dest != PROTO_TIMER || el.data.timer.proto_origin != PROTO_TIMER) {

				timer_elem* newTimerEvent = malloc(sizeof(timer_elem));
				newTimerEvent->timer = malloc(sizeof(LKTimer));
				newTimerEvent->next = NULL;
				memcpy(newTimerEvent->timer, &el.data.timer, sizeof(LKTimer));

				if(newTimerEvent->timer->config.first_notification == 0 && newTimerEvent->timer->config.repeat_interval_ms == 0) {
					//The goal now is to cancel a timer
					if(pendingTimers != NULL && uuid_compare(pendingTimers->timer->id, newTimerEvent->timer->id) == 0) {
						//We are canceling the first timer
						timer_elem* tmp = pendingTimers;
						pendingTimers = tmp->next;
						pendingTimersSize--;
						free(tmp->timer);
						free(tmp);
					} else if(pendingTimers != NULL) {
						timer_elem* current = pendingTimers;
						while(current->next != NULL && uuid_compare(current->next->timer->id, newTimerEvent->timer->id) != 0) {
							current = current->next;
						}
						if(current->next != NULL) {
							timer_elem* tmp = current->next;
							current->next = tmp->next;
							free(tmp->timer);
							free(tmp);
							pendingTimersSize--;
							break;
						}
					} else {
						char s[100];
						sprintf(s,"Received a cancellation request when I have no pending timers (origin %d).", newTimerEvent->timer->proto_origin);
						lk_log("TIMER", "WARNING", s);
					}
					free(newTimerEvent->timer);
					free(newTimerEvent);
				} else {
					if(pendingTimers == NULL || pendingTimers->timer->config.first_notification - newTimerEvent->timer->config.first_notification > 0) {
						newTimerEvent->next = pendingTimers;
						pendingTimers = newTimerEvent;
						pendingTimersSize++;
					} else {
						timer_elem* current = pendingTimers;
						while(current->next != NULL) {
							if(current->next->timer->config.first_notification - newTimerEvent->timer->config.first_notification > 0)
								break;
							current = current->next;
						}
						newTimerEvent->next = current->next;
						current->next = newTimerEvent;
						pendingTimersSize++;
					}
				}
			}
		}

		processExpiredTimers();

	}

	return NULL;
}
