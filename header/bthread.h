#ifndef BTHREAD_H
#define BTHREAD_H

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errno.h"
#include "tqueue.h"
#include <signal.h>
#include <sys/time.h>


#define CUSHION_SIZE 5000
#define QUANTUM_MSEC 1

unsigned char sig_old_init;
sigset_t sig_old;
#define bthread_printf(...) \
		bthread_block_timer_signal(); \
		printf(__VA_ARGS__); \
		bthread_unblock_timer_signal();

//thread id
typedef unsigned long int bthread_t;

typedef enum
{
	__BTHREAD_EXITED = 0,
	__BTHREAD_ZOMBIE,
	__BTHREAD_UNINITIALIZED,
	__BTHREAD_READY,
	__BTHREAD_BLOCKED,
	__BTHREAD_SLEEPING
} bthread_state;

typedef struct
{
} bthread_attr_t;

typedef void* (*bthread_routine)(void *);
typedef void (*bthread_scheduling_routine)();

typedef enum
{
	__RANDOM = 0,
	__ROUND_ROBIN,
	__ROUND_ROBIN_PRIORITY
} scheduling_algorithm;


typedef struct __bthread_private
{
	bthread_t tid;
	bthread_routine body;
	double wake_up_time;
	void* arg;
	bthread_state state;
	bthread_attr_t attr;
	jmp_buf context;
	int cancel_req;
	void* retval;
    int priority;
    //for check quantum
    double stop_time;
} __bthread_private;

typedef struct
{
	TQueue* queue;
	TQueue* current_item;
	jmp_buf context;
	sigset_t *sig_set;
	bthread_scheduling_routine scheduling_routine;
} __bthread_scheduler_private;

int bthread_join(bthread_t bthread, void **retval);
double get_current_time_millis();
void bthread_yield();
void bthread_exit(void *retval);
void bthread_cleanup();

__bthread_scheduler_private* bthread_get_scheduler();
int bthread_create(bthread_t *bthread, const bthread_attr_t *attr, void *(*start_routine)(void *), void *arg);
int bthread_set_priority(bthread_t *bthread,int priority);
void bthread_create_cushion(__bthread_private *t_data);
void bthread_initialize_next();
void bthread_sleep(double ms);
int bthread_check_zombie(bthread_t bthread, void **retval);


void set_scheduler_algorithm(scheduling_algorithm algorithm);
void scheduler_random(__bthread_scheduler_private *scheduler);
void scheduler_round_robin(__bthread_scheduler_private *scheduler);
void scheduler_round_robin_with_priority(__bthread_scheduler_private *scheduler);

int bthread_cancel(bthread_t bthread);
void bthread_testcancel(void);

void bthread_setup_timer();
void bthread_block_timer_signal();
void bthread_unblock_timer_signal();

#endif /* BTHREAD_H */
