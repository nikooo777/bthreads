#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bthread.h"
#include "tmutex.h"

#define N 5
#define LEFT(i) (i+N-1) % N
#define RIGHT(i) (i+1) % N

bthread_mutex_t forks[N];

void* philosopher (void* arg);
void think(int i);
void eat(int i);

void think(int i)
{
    bthread_printf("Philosopher %d is thinking...\n", i);
    bthread_sleep(200);
}

void eat(int i)
{
    bthread_printf("Philosopher %d is eating...\n", i);
    bthread_sleep(300);
}

void* philosopher (void* arg)
{
    int i;
    i = (intptr_t) arg;
    while(1) {
        think(i);
        bthread_mutex_lock(&forks[LEFT(i)]);
        if (bthread_mutex_trylock(&forks[RIGHT(i)]) < 0) {
            bthread_mutex_unlock(&forks[LEFT(i)]);
            continue;
        }
        eat(i);
        bthread_mutex_unlock(&forks[LEFT(i)]);
        bthread_mutex_unlock(&forks[RIGHT(i)]);
    }
    bthread_printf("\tPhilosopher %d dead\n", i);
}

int main(int argc, char *argv[])
{
    int j;
    for (j=0; j<N; j++) {
        bthread_mutex_init(&forks[j], NULL);
    }

    bthread_t philosophers[N];
    int i;
    for (i=0; i<N; i++) {
        bthread_create(&philosophers[i], NULL, philosopher, (void*) (intptr_t) i);
    }

    for (i=0; i<N; i++) {
        bthread_join(philosophers[i], NULL);
    }

    for (j=0; j<N; j++) {
        bthread_mutex_destroy(&forks[j]);
    }

    printf("Exiting main\n");
    return 0;
}
