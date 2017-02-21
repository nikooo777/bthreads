#include "tbarrier.h"

int bthread_barrier_init(bthread_barrier_t* b,const bthread_barrierattr_t* attr,unsigned count)
{
	b = (bthread_barrier_t*) malloc (sizeof(bthread_barrier_t));
	b->count = count;
	b->waiting_list = (TQueue*) malloc(sizeof(TQueue));
	// attr is ignored
	return 0;
}

int bthread_barrier_destroy(bthread_barrier_t* b)
{
	free(b->waiting_list);
	free(b);
	return 0;
}

int bthread_barrier_wait(bthread_barrier_t* b)
{
    // Blocca il segnale del timer per togliere la preemption, proteggere variabili condivise (queue)
    bthread_block_timer_signal();

    // Faccio restituire lo scheduler
    __bthread_scheduler_private* scheduler = bthread_get_scheduler();
    // Current thread
    __bthread_private* current_thread = scheduler->current_item->thread;

    b->count++;
    __bthread_private* last_arrived;
    if(b->count == b->barrier_size)
    {
        b->count = 0;
        while((last_arrived = tqueue_pop(&b->waiting_list)) != NULL)
        {
            last_arrived->state = __BTHREAD_READY;
        }
    }
    else
    {
        current_thread->state = __BTHREAD_BLOCKED;
        tqueue_enqueue(&b->waiting_list, current_thread);
        bthread_yield();
    }
    bthread_unblock_timer_signal();
    return 0;
}
