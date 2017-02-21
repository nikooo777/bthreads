#ifndef __TMUTEX_H__
#define __TMUTEX_H__

#include "tqueue.h"
#include <assert.h>


typedef struct
{
  __bthread_private* owner;
  TQueue* waiting_list;
} bthread_mutex_t;

 // Defined only for "compatibility" with pthread
 typedef struct {} bthread_mutexattr_t;

// attr is ignored
int bthread_mutex_init(bthread_mutex_t* m, const bthread_mutexattr_t *attr);

int bthread_mutex_destroy(bthread_mutex_t* m);

int bthread_mutex_lock(bthread_mutex_t* m);
int bthread_mutex_trylock(bthread_mutex_t* m);
int bthread_mutex_unlock(bthread_mutex_t* m);
#endif
