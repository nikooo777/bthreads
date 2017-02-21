#include "bthread.h"

//here a scheduler is initializated (only once) and then the same pointer is always returned
/**
 * This function creates, maintains and returns a static pointer to the singleton instance of
 * __bthread_scheduler_private. Other functions will call this method to obtain this pointer.
 */
__bthread_scheduler_private* bthread_get_scheduler()
{
	static __bthread_scheduler_private* instance = NULL;
	if (instance == NULL)
	{
		instance = (__bthread_scheduler_private*)calloc(1,sizeof(__bthread_scheduler_private));
	}

	return instance;
}

// We don't know where the start routine will go, we suppose *arg is a pointer to queue
// *bthread should be a pointer to long int, so is an accessible number (tid)
// We're still unaware of what value must be returned here. Perhpas the TID? (not a long int tho)

/**
 * Creates a new thread structure and puts it at the end of the queue. The thread identifier (stored
 * in the buffer pointed by bthread) corresponds to the position in the queue. The thread is not
 * started when calling this function. Attributes passed through the attr argument are ignored
 * (thus it is possible to pass a NULL pointer). Newly created threads are in the
 * __BTHREAD_UNINITIALIZED state.
 */
int bthread_create(bthread_t *bthread, const bthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
	//check inputs
	if (!bthread)
		return 1;

	//allocate memory for the thread
	__bthread_private *thread = (__bthread_private *) calloc(1, sizeof(__bthread_private ));

	//reference to pointer to the queue
	TQueue* current_queue_item = bthread_get_scheduler()->queue;

	int tid = tqueue_enqueue(&(current_queue_item), thread);
	//set the attributes of the thread
	thread->attr = *attr; //attributes passed through the attr argument are ignored thus it's possible to pass a NULL pointer
	thread->state = __BTHREAD_UNINITIALIZED;
	thread->tid = tid;
	thread->body = *start_routine;
	//set priority 0
	thread->priority = 0;
	thread->stop_time = 0;
	//initialized to 0
	thread->cancel_req = 0;
	thread->arg = arg;

	//store the thread ID in the allocated memory space given by the user
	*bthread = thread->tid;

	//printf("Thread created: %lu\n", thread->tid);

	//initialize the current item
	bthread_get_scheduler()->queue = current_queue_item;

	if (bthread_get_scheduler()->current_item == NULL)
	{
		bthread_get_scheduler()->current_item = bthread_get_scheduler()->queue;
	}

	return 0;
}

/*
 * Set the priority on the thread
 * */
int bthread_set_priority(bthread_t *bthread, int priority)
{

	TQueue *queue = tqueue_at(&(bthread_get_scheduler()->queue), *bthread);
	//Set priority
	queue->thread->priority = priority;

	return 0;
}

/**
 * Waits for the thread specified by bthread to terminate (i.e. __BTHREAD_ZOMBIE state), by
 * scheduling all the threads. In the following section we will discuss some specific details about
 * this procedure.
 */
/****************************
 * Comments v2.0
 *
 * The program should begin with a join to start the scheduler
 */
int bthread_join(bthread_t bthread, void **retval)
{
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();

	bthread_block_timer_signal();
	bthread_setup_timer();

	// sigsetjmp returns 0 if success
	if (sigsetjmp(scheduler->context, 1) == 0)
	{
		//printf("Context = 0\n");
		bthread_initialize_next();
		siglongjmp(scheduler->context, 1);
	} else
	{

		do
		{
			//bthread_printf("checking thread %d state %i\n", scheduler->current_item->thread->tid, scheduler->current_item->thread->state);
			if (bthread_check_zombie(bthread, retval))
			{
				//bthread_printf("Finish thread %i\n",bthread);
				return 0;
			}

			//scheduling algorithm not set
			if((*(scheduler->scheduling_routine))==NULL)
			{
				scheduler->current_item = scheduler->current_item->next;
				//	next_schedulable_thread = scheduler->current_item->thread;
			}
			else
			{
				(*(scheduler->scheduling_routine))(scheduler);
			}

			//if thread sleep
			if(scheduler->current_item->thread->state == __BTHREAD_SLEEPING && scheduler->current_item->thread->wake_up_time < get_current_time_millis())
			{
				scheduler->current_item->thread->state = __BTHREAD_READY;
			}

		}while (scheduler->current_item->thread->state != __BTHREAD_READY);

		siglongjmp(scheduler->current_item->thread->context, 1);
	}
	return -1;
}
/**
 * Saves the thread context and then checks whether the thread that follows in the queue is in the
 * __BTHREAD_UNINITIALIZED state: if so, a cushion frame is created and the corresponding
 * thread routine is called, otherwise bthread_yield long-jumps to the scheduler context. Saving
 * the thread context is achieved using sigsetjmp, which is similar to setjmp but can also save
 * the signal mask if the provided additional parameter is not zero (to restore both the context and
 * the signal mask the corresponding call is siglongjmp). Saving and restoring the signal mask is
 * required for implementing preemption.
 */
void bthread_yield()
{
	bthread_block_timer_signal();
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();

	__bthread_private* current_thread = scheduler->current_item->thread;

	// Save the context
	if (sigsetjmp(current_thread->context, 1) == 0)
	{
		// Check if state of next thread is equal to __BTHREAD_UNINITIALIZED
		__bthread_private *next_thread = scheduler->current_item->next->thread;
		if (next_thread->state == __BTHREAD_UNINITIALIZED)
		{
			scheduler->current_item = scheduler->current_item->next;
			bthread_create_cushion(next_thread);
		}
		// Restore context
		siglongjmp(scheduler->context, 1);
	}
	bthread_unblock_timer_signal();
}

/**
 * Terminates the calling thread and returns a value via retval that will be available to another
 * thread in the same process that calls bthread_join. Between bthread_exit and the
 * corresponding bthread_join the thread stays in the __BTHREAD_ZOMBIE state.
 */
void bthread_exit(void * retval)
{
	//get the scheduler
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();

	//get the current running thread
	TQueue *current_thread = scheduler->current_item;

	//the thread remains in zombie state until bhtread_join is called
	current_thread->thread->state=__BTHREAD_ZOMBIE;

	//pass the returned value
	current_thread->thread->retval = retval;

	//pass control to the scheduler
	bthread_yield();
}

/**
 * Frees memory allocated for threads and the scheduler. This procedure can either be called
 * explicitly by the programmer (before returning from the main procedure) or automatically (as
 * explained in the following section).
 */
void bthread_cleanup()
{
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();
	TQueue *queue = scheduler->queue;
	__bthread_private *tmpThread;

	// free all queue + threads
	while((tmpThread = tqueue_pop(&queue)) != NULL)
	{
		free(tmpThread);
	}
	// free scheduler
	free(scheduler);
}

/**
 * Creates a stack cushion for the given thread, sets its state to __BTHREAD_READY, and then calls
 * the start routine with the argument provided through bthread_create. The call to the start
 * routine should be wrapped into bthread_exit to correctly handle termination.
 */
void bthread_create_cushion(__bthread_private *t_data)
{
	//printf("CREO CUSHION\n");
	char cushion[CUSHION_SIZE];
	cushion[CUSHION_SIZE - 1] = cushion[0];

	t_data->state = __BTHREAD_READY;
	bthread_unblock_timer_signal();
	bthread_exit(t_data->body(t_data->arg));
}

/**
 * Checks whether the next thread in the queue is in the __BTHREAD_UNINITIALIZED state: if
 * so, a cushion frame is created and the corresponding thread routine is called.
 */
void bthread_initialize_next()
{
	//get the scheduler
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();

	//get the current running thread
	TQueue *current_thread = scheduler->current_item;
	if(current_thread->thread->state == __BTHREAD_UNINITIALIZED)
	{
		bthread_create_cushion(current_thread->thread);
	}
	else
	{
		//get next element
		TQueue * next_element = current_thread->next;

		//check if the element is not the only one in the queue
		if (next_element->thread->state == __BTHREAD_UNINITIALIZED)
		{
			//create a cushion for the next thread and after setting the state to ready calls the routine
			bthread_create_cushion(next_element->thread);
		}
	}
}

/**
 * Checks whether the current thread has reached a zombie state and if it is being waited for.
 * When that thread terminates, its state changes from __BTHREAD_ZOMBIE to
 * __BTHREAD_EXITED: if retval is not NULL the exit status of the target thread (i.e. the value that
 * was supplied to bthread_exit) is copied into the location pointed to by *retval. If the
 * current thread was not zombie this function returns 0, otherwise 1.
 */
int bthread_check_zombie(bthread_t bthread, void **retval)
{
	//get the scheduler
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();
	__bthread_private* current_thread = scheduler->current_item->thread;
	if (current_thread->tid == bthread && current_thread->state == __BTHREAD_ZOMBIE)
	{
		if (retval != NULL)
		{
			*retval = current_thread->retval;
		}
		current_thread->state = __BTHREAD_EXITED;
		return 1;
	}
	return 0;
}

/**
 * return the current time in milliseconds
 */
double get_current_time_millis()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

/*
 * When calling bthread_sleep the state of the thread is set to __BTHREAD_SLEEPING and then the thread
 * must yield to the scheduler.
 */
void bthread_sleep(double ms)
{
	//get the scheduler
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();

	//get the current running thread
	TQueue *current_thread = scheduler->current_item;

	//set thread state to sleeping
	current_thread->thread->state = __BTHREAD_SLEEPING;

	bthread_yield();
}

/*
 * As we saw in the class, a thread can also request cancellation of another thread: cancellation happens as
 * soon as the thread receiving the request calls testcancel. To implement cancellation points in our library we
 * need to keep track of cancellation requests by adding a flag to the thread structure which will be initialized to
 * 0 and set to 1 when someone ask for cancellation:
 * */
int bthread_cancel(bthread_t bthread)
{

	//get the scheduler
	__bthread_scheduler_private* scheduler = bthread_get_scheduler();

	TQueue *queue = tqueue_at(&(scheduler->queue), bthread);

	if(queue == NULL)
	return -1;

	queue->thread->cancel_req = -1;
	return 0;
}

void bthread_testcancel()
{

	__bthread_scheduler_private* scheduler = bthread_get_scheduler();
	if(scheduler->current_item->thread->cancel_req == -1)
	bthread_exit((void *)-1);

}

/*
 * Set the scheduling algorithm
 * */
void set_scheduler_algorithm(scheduling_algorithm algorithm)
{

	__bthread_scheduler_private* scheduler = bthread_get_scheduler();

	switch (algorithm)
	{
		case __RANDOM:
		scheduler->scheduling_routine = &scheduler_random;
		bthread_printf("RANDOM scheduling\n");
		break;

		case __ROUND_ROBIN:
		bthread_printf("ROUND ROBIN scheduling\n");
		scheduler->scheduling_routine = &scheduler_round_robin;
		break;

		case __ROUND_ROBIN_PRIORITY:
		bthread_printf("ROUND ROBIN PRIORITY scheduling\n");
		scheduler->scheduling_routine = &scheduler_round_robin_with_priority;
		break;

	}

}

/*
 * random priority
 * */
void scheduler_random(__bthread_scheduler_private *scheduler)
{

	int count = tqueue_size(&(scheduler->queue));
	int r = rand() % count;
	scheduler->current_item = tqueue_at(&(scheduler->queue), r);

}

/*
 * Round robin algorithm
 * */
void scheduler_round_robin(__bthread_scheduler_private *scheduler)
{

	double current_time = get_current_time_millis();

	//controlla se il thread ha ancora tempo per eseguire
	if (scheduler->current_item->thread->stop_time < current_time /*|| !scheduler->current_item->thread->stop_time*/)
	{
		scheduler->current_item = scheduler->current_item->next;
		int priority = scheduler->current_item->thread->priority;
		scheduler->current_item->thread->stop_time = current_time + QUANTUM_MSEC;
	}
}

/*
 * Concerning priority scheduling a simple scheme can be implemented, with higher priority threads
 * receiving a longer quantum or more than one consecutive quantum.
 * */
void scheduler_round_robin_with_priority(__bthread_scheduler_private *scheduler)
{

	double current_time = get_current_time_millis();

	//controlla se il thread ha ancora tempo per eseguire
	if (scheduler->current_item->thread->stop_time < current_time /*|| !scheduler->current_item->thread->stop_time*/)
	{
		scheduler->current_item = scheduler->current_item->next;
		int priority = scheduler->current_item->thread->priority;
		scheduler->current_item->thread->stop_time = current_time + QUANTUM_MSEC + QUANTUM_MSEC * priority;
	}
}

/*
 * Preemption prevents a thread from never releasing control to the scheduler (through bthread_yield), and
 * can be implemented by means of a timer signal that periodically interrupts the executing thread and returns
 * control to the scheduler. To set such a timer we employ setitimer:
 */
void bthread_setup_timer()
{
	static unsigned char initialized = 0;

	if (!initialized)
	{
		signal(SIGVTALRM, (void (*)()) bthread_yield);

		struct itimerval time;
		time.it_interval.tv_sec = 0;
		time.it_interval.tv_usec = QUANTUM_MSEC;
		time.it_value.tv_sec = 0;
		time.it_value.tv_usec = QUANTUM_MSEC;
		initialized = 1;
		setitimer(ITIMER_VIRTUAL, &time, NULL);
	}
}

/*The length of the quantum (in microseconds) is defined by the macro QUANTUM_USEC. Each time the alarm
 * fires a SIGVTALRM is sent to the process and the bthread_yield procedure is called. The signal handler
 * executes on the current thread's stack, hence bthread__yield will save the correct execution context.
 * Preemption can also cause some issues in our library. For example, if the scheduler is preempted we might
 * generate a race condition. To avoid this problem we to define two “public” procedures to temporarily
 * block/unblock the timer signal (using sigprocmask):
 */
void bthread_block_timer_signal()
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGVTALRM);
	sigprocmask(SIG_BLOCK, &mask, NULL);
}

void bthread_unblock_timer_signal()
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGVTALRM);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}
