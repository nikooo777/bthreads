#include "tsemaphore.h"
#include "bthread.h"


int bthread_sem_init(bthread_sem_t* m, int pshared, unsigned int value)
{
	m = (bthread_sem_t*) malloc(sizeof(bthread_sem_t));
	m->value = value;
	m->waiting_list = (TQueue*) malloc(sizeof(TQueue));
	return 0;
}

int bthread_sem_destroy(bthread_sem_t* m)
{
	free(m->waiting_list);
	free(m);
	return 0;
}

int bthread_sem_wait(bthread_sem_t* m)
{
	if (m->value > 0)
	{
		m->value--;
	} else
	{
		__bthread_private* current_thread = bthread_get_scheduler()->current_item->thread;
		current_thread->state= __BTHREAD_SLEEPING;
		tqueue_enqueue(&(m->waiting_list), current_thread);
	}
	return 0;
}

int bthread_sem_post(bthread_sem_t* m)
{
	m->value++;

	if (tqueue_size(&(m->waiting_list)) > 0)
	{
		tqueue_pop(&(m->waiting_list))->state = __BTHREAD_READY;
		m->value--;
	}
	return 0;
}
