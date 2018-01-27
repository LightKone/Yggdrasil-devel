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

#include "limited_size_queue.h"

typedef struct _inner_lsqueue_t {
	sem_t empty;
	unsigned short element_size;
	unsigned short max_count;
	unsigned short occupation;
	void* data;
	void* writer;
	void* reader;
}inner_lsqueue_t;

typedef struct _lsqueue_t {
	unsigned short pid;
	pthread_mutex_t lock;
	sem_t full;
	inner_lsqueue_t iqs[TYPE_MAX];
}lsqueue_t;


void inner_lsqueue_init(inner_lsqueue_t* iq, unsigned short capacity, unsigned short element_size) {
	iq->max_count  = capacity;
	iq->element_size = element_size;
	iq->occupation = 0;
	iq->data = malloc(element_size * capacity);
	iq->writer = iq->data;
	iq->reader = iq->data;
	sem_init(&iq->empty, 0, iq->max_count);
}

void inner_lsqueue_destroy(inner_lsqueue_t* iq) {
	sem_destroy(&iq->empty);
	free(iq->data);
}

void lsqueue_init(lsqueue_t* q, unsigned short pid, unsigned short capacity) {
	pthread_mutex_init(&q->lock, NULL);
	q->pid = pid;
	int i;
	for(i = 0; i < TYPE_MAX; i++) {
		inner_lsqueue_init(&(q->iqs[i]), capacity, size_of_element_types[i]);
	}
	sem_init(&q->full, 0, 0);

}

void lsqueue_destroy(lsqueue_t* q) {
	sem_destroy(&q->full);
	pthread_mutex_destroy(&q->lock);
	int i;
	for(i = 0; i < TYPE_MAX; i++) {
		inner_lsqueue_destroy(&(q->iqs[i]));
	}
}

int inner_lsqueue_pop(inner_lsqueue_t* iq, queue_t_cont* cont) {

	if(iq->occupation > 0){
		memcpy(cont, iq->reader, iq->element_size);
		iq->occupation--;
		iq->reader+=iq->element_size;
		if(iq->reader >= iq->data + iq->max_count * iq->element_size) {
			iq->reader = iq->data;
			printf("Rotated reader\n");
		}
		sem_post(&iq->empty);
		return 1;
	}

	return 0;
}

void lsqueue_pop(lsqueue_t* q, queue_t_elem* elem) {

	while(sem_wait(&q->full) != 0);
	pthread_mutex_lock(&q->lock);
	unsigned short i = 0;
	while(i < TYPE_MAX && inner_lsqueue_pop(&(q->iqs[i]), &elem->data) == 0) {
		i++;
	}
	elem->type = i;
	pthread_mutex_unlock(&q->lock);
}

int lsqueue_try_timed_pop(lsqueue_t* q, struct timespec* uptotime, queue_t_elem* elem) {

	int r = 0;
	while(1) {
		r = sem_timedwait(&q->full, uptotime) != 0;
		if(r == 0)
			break;
		else if(errno == ETIMEDOUT)
			return 0;
	}
	pthread_mutex_lock(&q->lock);
	unsigned short i = 0;
	while(i < TYPE_MAX && inner_lsqueue_pop(&(q->iqs[i]), &elem->data) == 0) {
		i++;
	}
	elem->type = i;
	pthread_mutex_unlock(&q->lock);
	return 1;
}

void lsqueue_push(lsqueue_t* q, queue_t_elem* elem) {
	inner_lsqueue_t* target = &q->iqs[elem->type];

	while(sem_wait(&target->empty)!=0);
	pthread_mutex_lock(&q->lock);
	memcpy(target->writer, &elem->data, target->element_size);
	target->writer += target->element_size;
	if(target->writer >= target->data + target->max_count * target->element_size) {
		target->writer = target->data;
		printf("Rotated writer\n");
	}
	target->occupation++;
	pthread_mutex_unlock(&q->lock);
	sem_post(&q->full);

}

void lsqueue_try_push(lsqueue_t* q, queue_t_elem* elem) {
	inner_lsqueue_t* target = &q->iqs[elem->type];

	if(sem_trywait(&target->empty) == 0) {
		if(pthread_mutex_trylock(&q->lock) == 0) {
			memcpy(target->writer, &elem->data, target->element_size);
			target->writer += target->element_size;
			if(target->writer >= target->data + target->max_count * target->element_size) {
				target->writer = target->data;

			}
			target->occupation++;
			pthread_mutex_unlock(&q->lock);
			sem_post(&q->full);

		} else {
			sem_post(&target->empty); //Invert effect of the successful sem_trywait.
		}
	}
}

int lsqueue_try_timed_push(lsqueue_t* q, long time_ms, queue_t_elem* elem) {
	inner_lsqueue_t* target = &q->iqs[elem->type];

	struct timespec uptotime;
	gettimeofday((struct timeval *)&uptotime, NULL);
	uptotime.tv_nsec += time_ms * 1000000;
	if(uptotime.tv_nsec > 999999999) {
		uptotime.tv_sec += 1;
		uptotime.tv_nsec -= 1000000000;
	}

	int r = 0;
	while(1) {
		r = sem_timedwait(&target->empty, &uptotime) != 0;
		if(r == 0)
			break;
		else if(errno == ETIMEDOUT)
			return 0;
	}
	pthread_mutex_lock(&q->lock);
	memcpy(target->writer, &elem->data, target->element_size);
	target->writer += target->element_size;
	if(target->writer >= target->data + target->max_count * target->element_size) {
		target->writer = target->data;

	}
	target->occupation++;
	pthread_mutex_unlock(&q->lock);
	sem_post(&q->full);

	return 1;
}

short lsqueue_size(lsqueue_t* q, queue_t_elem_type type) {
	pthread_mutex_lock(&q->lock);
	unsigned short s = q->iqs[type].occupation;
	pthread_mutex_unlock(&q->lock);
	return s;
}

short lsqueue_totalSize(lsqueue_t* q) {
	pthread_mutex_lock(&q->lock);
	unsigned short s = q->iqs[0].occupation;
	int i;
	for(i = 1; i < TYPE_MAX; i++)
		s += q->iqs[i].occupation;
	pthread_mutex_unlock(&q->lock);
	return s;
}

//After the execution of this command attempting to manipulate
//this inner queue will result on unspecified behavior (most likely a crash)
//This should only only only be executed if this queue will never
//hold an element of the type provided as argument.
void destroy_inner_lsqueue(lsqueue_t* q, queue_t_elem_type type) {
	inner_lsqueue_destroy(&q->iqs[type]);
	memset(&q->iqs[type], 0, sizeof(inner_lsqueue_t));
}
