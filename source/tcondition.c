#include "tcondition.h"

// attr is ignored
int bthread_cond_init(bthread_cond_t* c, const bthread_condattr_t *attr)
{

	c = (bthread_cond_t*) malloc (sizeof(bthread_cond_t));

	c->value = 0;
	c->waiting_list = (TQueue*) malloc(sizeof(TQueue));

	return 0;
}

int bthread_cond_destroy(bthread_cond_t* c)
{
	free(c->waiting_list);
	free(c);
	return 0;
}

int bthread_cond_wait(bthread_cond_t* c, bthread_mutex_t* mutex)
{


	return 0;
}

int bthread_cond_signal(bthread_cond_t* c)
{


	return 0;
}

int bthread_cond_broadcast(bthread_cond_t* c)
{


	return 0;
}
