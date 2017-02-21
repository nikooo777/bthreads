#include "tmutex.h"
#include "bthread.h"

int bthread_mutex_init(bthread_mutex_t* m, const bthread_mutexattr_t *attr) {

	if (m == NULL)
	{
		m = (bthread_mutex_t*) malloc(sizeof(bthread_mutex_t));
	}

	m->owner = NULL;
	m->waiting_list = NULL;

	return 0;
}

int bthread_mutex_destroy(bthread_mutex_t* m) {

	assert(m->owner == NULL);
	assert(tqueue_size(&m->waiting_list) == 0);
	free(m);
	return 0;
}

int bthread_mutex_lock(bthread_mutex_t* m) {

	bthread_block_timer_signal();
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();
	__bthread_private* bthread = scheduler->current_item->thread;

	if (m->owner == NULL)
	{
		m->owner = bthread;
		bthread_unblock_timer_signal();
	}
	else
	{

		bthread->state = __BTHREAD_BLOCKED;
		tqueue_enqueue(&m->waiting_list, bthread);
		while (bthread->state != __BTHREAD_READY)
		{
			bthread_yield();
		}
	}

	return 0;
}

int bthread_mutex_trylock(bthread_mutex_t* m) {

	bthread_block_timer_signal();
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();
	__bthread_private* bthread = scheduler->current_item->thread;

	if (m->owner == NULL)
	{
		m->owner = bthread;
		bthread_unblock_timer_signal();
	}
	else
	{
		bthread_unblock_timer_signal();
		return -1;
	}

	return 0;
}

int bthread_mutex_unlock(bthread_mutex_t* m) {

	bthread_block_timer_signal();
	assert(m->owner != NULL);
	assert(m->owner == bthread_get_scheduler()->current_item->thread);
	__bthread_private* unlock = tqueue_pop(&m->waiting_list);
	if (unlock != NULL)
	{
		m->owner = unlock;
		unlock->state = __BTHREAD_READY;
		bthread_yield();
		return 0;
	}
	else
	{
		m->owner = NULL;
	}
	bthread_unblock_timer_signal();
	return 0;
}
