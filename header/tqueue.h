#ifndef THREADING_LIBRARY_SRC_HEADER_TQUEUE_H_
#define THREADING_LIBRARY_SRC_HEADER_TQUEUE_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct __bthread_private __bthread_private;

typedef struct TQueue
{
	struct TQueue* next;
	__bthread_private* thread;
}TQueue;

/* Add a new element at the end of the list, returns its position */
unsigned long int tqueue_enqueue(TQueue** q, __bthread_private* thread);

/* Removes and returns the element at the beginning of the list, NULL if the
 queue is empty */
__bthread_private* tqueue_pop(TQueue** q);

/* Returns the number of elements in the list */
unsigned long int tqueue_size(TQueue** q);

/* Returns the element at the given position, NULL if the queue is empty */
TQueue* tqueue_at(TQueue** q, unsigned long int position);

#endif /* THREADING_LIBRARY_SRC_HEADER_TQUEUE_H_ */
