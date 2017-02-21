/*
 * Sistemi operativi
 *
 * Gruppo 3:
 * - Bovino Cristian
 * - Ceni Kevin
 * - Esteves Jorge
 * - Storni Niko
 */

//memory leak checker
//valgrind --leak-check=full --track-origins=yes -v ./threading_library
#include <setjmp.h>
#include <stdio.h>
#include "tqueue.h"
#include "bthread.h"

int g(void * i)
{
	int j = -214700000;
	for (int i = 0; i < 5; i++){
		j++;
	bthread_printf("(g) -> %d\n", i);
	for (int i = 0; i < 5000000; i++)
			j++;
	}


	return *((int *) i);
}

int h(void * i)
{
	int j = -214700000;
	for (int i = 0; i < 5; i++){
			j++;
			bthread_printf("(h) -> %d\n", i);
			for (int i = 0; i < 5000000; i++)
						j++;
	}
	return *((int *) i);
}

int l(void * i)
{
	int j = -214700000;
	for (int h = 0; h < 5; h++){
			j++;
			bthread_printf("(l) -> %d\n",h);
			for (int i = 0; i < 5000000; i++)
						j++;
	}

	return *((int *) i);
}

//@TODO: comment the code ASAP (as soon as possible)
int main()
{


	int arg1 = 4;
	int arg2 = 5;
	int arg3 = 6;
	bthread_t id1, id2, id3;

	// Create a new thread and insert to the queue

	bthread_create(&id1, NULL, (void *) g, &arg1);
	bthread_create(&id2, NULL, (void *) h, &arg2);
	bthread_create(&id3, NULL, (void *) l, &arg3);
	printf("All thread created\n");


	set_scheduler_algorithm(__RANDOM);

	// Starting our scheduler
	int ret1,ret2,ret3;
	bthread_join(id1, (void*) (&ret1));
	bthread_join(id2, (void*) (&ret2));
	bthread_join(id3, (void*) (&ret3));

	printf("\nReturn join = %d \n", ret1);
	printf("Return join = %d \n", ret2);
	printf("Return join = %d \n", ret3);

	bthread_cleanup();
	printf("Application terminated\n");
	return 0;
}

