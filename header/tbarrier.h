#ifndef __TBARRIER_H__
#define __TBARRIER_H__

#include "tqueue.h"
#include "bthread.h"
typedef struct bthread_barrier {

	TQueue* waiting_list;
	unsigned count;
	unsigned barrier_size;

} bthread_barrier_t;

// Defined only for "compatibility" with pthread
typedef struct {
} bthread_barrierattr_t;

// attr is ignored
int bthread_barrier_init(bthread_barrier_t* b,const bthread_barrierattr_t* attr,unsigned count);

int bthread_barrier_destroy(bthread_barrier_t* b);

int bthread_barrier_wait(bthread_barrier_t* b);

#endif
