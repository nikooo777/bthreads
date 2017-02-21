#ifndef __TSEMAPHORE_H__
#define __TSEMAPHORE_H__

#include "tqueue.h"
typedef struct {
	unsigned int value;
	TQueue* waiting_list;
} bthread_sem_t;

// pshared is ignored, defined for compatibility with pthread
int bthread_sem_init(bthread_sem_t* m, int pshared, unsigned int value);

int bthread_sem_destroy(bthread_sem_t* m);

int bthread_sem_wait(bthread_sem_t* m);

int bthread_sem_post(bthread_sem_t* m);

#define bthread_sem_up(s) \
		bthread_sem_post( s);

#define bthread_sem_down(s) \
		bthread_sem_wait( s);

#endif
