#include <stdio.h>

#include <pthread.h>
#include <stdlib.h>
#include "hp.h"

#define TIMES 500000

int *data;

void *reader(void *arg)
{
    void **argv = (void **) arg;
    hp_protect_t *pr = argv[0];
    for (int i = 0; i < TIMES; i++) {
        int *ptr = hp_protect_load(pr, &data);
        // do something
        printf("%d\n", *ptr);
        hp_protect_release(pr, ptr);
    }
}

void printf_free(void *ptr)
{
    printf("free %d\n", *(int *) ptr);
    free(ptr);
}

void *writer(void *arg)
{
    void **argv = (void **) arg;
    hp_protect_t *pr = argv[0];
    int id = (long) argv[1];
    hp_t *hp = hp_init(pr, printf_free);
    for (int i = 0; i < TIMES; i++) {
        int *tmp = malloc(sizeof(*tmp));
        *tmp = i + id * TIMES;

        int *res = atomic_exchange(&data, tmp);
        hp_retired(hp, res);
    }
    while (hp_scan(hp))
        ;
    free(hp);
}

#define NUM_THREAD 4

int main()
{
    hp_protect_t *pr = hp_protect_init();

    void *argv[NUM_THREAD][2];

    data = malloc(sizeof(*data));
    *data = 0;
    pthread_t t[NUM_THREAD];
    for (int i = 0; i < NUM_THREAD; i++) {
        argv[i][0] = pr;
        argv[i][1] = (void *) (long) i;
        pthread_create(&t[i], NULL, i % 2 ? reader : writer, &argv[i]);
    }
    for (int i = 0; i < NUM_THREAD; i++) {
        pthread_join(t[i], NULL);
    }
    hp_pr_destroy(pr);
    printf("free %d\n", *data);
    free(data);
}