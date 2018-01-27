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

#include "timer_signal.h"


typedef struct timer_elem_ {
	LKTimer* timer;
	struct timer_elem_* next;
} timer_elem;

static unsigned short pendingTimersSize;
static timer_elem* pendingTimers;
static timer_elem* timersToDeliver;
static timer_t timerid;
static queue_t* inBox;

static void ts_cancelTimer() {
	struct itimerspec its;
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	if (timer_settime(timerid, 0, &its, NULL) == -1)
		errExit("timer_settime on cancel");
}

static void setupSignalTimer(time_t atTime) {

	struct itimerspec its;
	its.it_value.tv_nsec =  0; //nanoseconds

	its.it_value.tv_sec = atTime; //Seconds;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	if (timer_settime(timerid, TIMER_ABSTIME, &its, NULL) == -1)
		errExit("timer_settime");

}

static void processTimeHandler(int sig, siginfo_t *si, void *uc) {

	LKTimer* timer = NULL;

	//Now process the events in the pending events that have expired.
	time_t now = time(NULL);

	while(pendingTimers != NULL && (pendingTimers->timer->config.first_notification - now) <= 0 ) {

		//Process this event...
		timer = malloc(sizeof(LKTimer));
		memcpy(timer, pendingTimers->timer, sizeof(LKTimer));

		//Remove it from the queue
		timer_elem* elem = pendingTimers;
		pendingTimers = elem->next;

		//Check if this is a repeatable timer, if so reinsert
		if(elem->timer->config.repeat_interval_ms > 0) {

			elem->timer->config.first_notification = elem->timer->config.first_notification + (elem->timer->config.repeat_interval_ms/1000) + (elem->timer->config.repeat_interval_ms % 1000 >= 500 ? 1:0);
			if(pendingTimers == NULL || (pendingTimers->timer->config.first_notification - elem->timer->config.first_notification) > 0) {
				elem->next = pendingTimers;
				pendingTimers = elem;
			} else {
				timer_elem* current = pendingTimers;
				while(current->next != NULL) {
					if((current->next->timer->config.first_notification - elem->timer->config.first_notification) > 0)
						break;
					current = current->next;
				}
				elem->next = current->next;
				current->next = elem;
			}
		} else {
			pendingTimersSize--;
		}

		elem->next = timersToDeliver;
		timersToDeliver = elem;

		//Update the time, just for compensation of the time spent in the delivery... (others might have expired...)
		time(&now);
	}

	if(timersToDeliver != NULL) {
		queue_t_elem marker;
		marker.type = LK_TIMER;
		marker.data.timer.proto_dest = PROTO_TIMER;
		marker.data.timer.proto_origin = PROTO_TIMER;
		queue_push(inBox, &marker);
	}

	if(pendingTimers != NULL) {
		setupSignalTimer(pendingTimers->timer->config.first_notification);
	}
}

void* timer_signal_init(void* args) {
	proto_args* pargs = args;
	inBox = pargs->inBox;
	destroy_inner_queue(inBox, LK_MESSAGE); //We will never receive LKMessages here

	pendingTimersSize = 0;
	pendingTimers = NULL;
	timersToDeliver = NULL;

	/* Establish handler for timer signal */
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = processTimeHandler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIG_TS, &sa, NULL) == -1)
		errExit("sigaction");

	/* Create the timer */
	struct sigevent sev;
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG_TS;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
		errExit("timer_create");

	sigset_t mask;
	queue_t_elem el;

	while(1){

		queue_pop(inBox, &el);

		/* Block timer signal temporarily */
		sigemptyset(&mask);
		sigaddset(&mask, SIG_TS);
		if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
			errExit("sigprocmask");

		//If this is not something we understand we stop here
		//should never have
		if(el.type != LK_TIMER) {
			//RE-ENABLE SIGNALS
			if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
				errExit("sigprocmask");
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
					if(pendingTimers == NULL)
						ts_cancelTimer();
					else
						setupSignalTimer(pendingTimers->timer->config.first_notification);
					free(tmp);
				} else {
					timer_elem* current = pendingTimers;
					while(current->next != NULL && uuid_compare(current->next->timer->id, newTimerEvent->timer->id) == 0) {
						current = current->next;
					}
					if(current->next != NULL) {
						timer_elem* tmp = current->next;
						current->next = tmp->next;
						free(tmp->timer);
						free(tmp);
						pendingTimersSize--;
					}
				}
				free(newTimerEvent->timer);
				free(newTimerEvent);
			} else {
				//This is a request to create a new timer request

				if(pendingTimers == NULL || pendingTimers->timer->config.first_notification - newTimerEvent->timer->config.first_notification > 0) {
					//Will setup on starting position
					newTimerEvent->next = pendingTimers;
					pendingTimers = newTimerEvent;
					//Will update the current alarm time
					setupSignalTimer(newTimerEvent->timer->config.first_notification);
					pendingTimersSize++;
				} else {
					//Will setup on middle position
					timer_elem* current = pendingTimers;
					while(current->next != NULL) {
						//difftime is buggy
						if(difftime(current->next->timer->config.first_notification,newTimerEvent->timer->config.first_notification) > 0)
							break;
						current = current->next;
					}
					newTimerEvent->next = current->next;
					current->next = newTimerEvent;
					pendingTimersSize++;
				}
			}

		}

		while(timersToDeliver != NULL) {
			timer_elem* d= timersToDeliver;
			timersToDeliver = d->next;
			//Deliver the timer notification

			deliverTimer(d->timer);
			free(d->timer);
			free(d);
		}

		/* Unlock the timer signal, so that timer notification can be delivered again*/
		if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
			errExit("sigprocmask");
	}
}
