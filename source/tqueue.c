#include "tqueue.h"

/*
 * Add a new element at the end of the queue, returns its position afterwards
 * If the queue doesn't have any elements, a new one is added,
 * the next element points to itself
 * Else a new element is allocated with the next element pointing to the head of the queue
 * the element is then pointed at from the end of the queue.
 *
 */
unsigned long int tqueue_enqueue(TQueue** queue, __bthread_private* new_thread)
{

	unsigned long int position=0;

	TQueue* new_element = (TQueue*) calloc(1, sizeof (TQueue));
	new_element->thread = new_thread;

	if (*queue==NULL)
	{
		*queue = new_element;
		(*queue)->next=*queue;
		return position;
	}
	position=1;
	TQueue* last_item = *queue;


	//while the next element is different than the element on the start of the queue
	while(last_item->next != *queue)//it breaks here now
	{
		last_item = last_item->next;
		position++;
	}
	new_element->next = *queue;
	last_item->next = new_element;

	return position;
}

/* Removes and returns the element at the beginning of the list, NULL if the
 queue is empty */
__bthread_private* tqueue_pop(TQueue** q)
{
	__bthread_private *tmpthread;

	if (*q==NULL)
	{
		return NULL;
	}
	else
	{
		tmpthread = (*q)->thread;
		if((*q)->next == (*q))
		{
			free(*q);
			*q = NULL;
			return tmpthread;
		}
		else
		{
			TQueue *cur= (*q);
			while(cur->next != *q)
			cur = cur->next;

			cur->next=(*q)->next;
			free(*q);
			*q= cur->next;
			return tmpthread;
		}
	}
}

/* Returns the number of elements in the list */
unsigned long int tqueue_size(TQueue** q)
{
	if (*q == NULL)
		return 0;

	int position = 1;
	TQueue *cur = (*q);

	while (cur->next != *q)
	{
		cur = cur->next;
		position++;
	}
	return position;
}

/* Returns the element at the given position, NULL if the queue is empty */
TQueue* tqueue_at(TQueue** q, unsigned long int position)
{
	if (tqueue_size(q) < position)
		return NULL;

	TQueue *cur = *q;

	for (int i = 0; i < position; i++)
		cur = cur->next;

	return cur;
}
