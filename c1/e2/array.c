#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <threads.h>
#include "options.h"

#define DELAY_SCALE 1000


struct array {
    int size;
    int *arr;
};

struct args{
        thrd_t id;
        int iterations;
        int delay;
        struct array *arr;
        mtx_t * mutex;
        
};


void apply_delay(int delay) {
    for(int i = 0; i < delay * DELAY_SCALE; i++); // waste time
}


int increment(void *args2)
{
    struct args *args = args2;
    int pos, val;
    for(int i = 0; i < args->iterations; i++) {
        pos = rand() % args->arr->size;

        printf("%ld increasing position %d\n", args->id, pos);
        mtx_lock(&(args->mutex[pos]));
        val = args->arr->arr[pos];
        apply_delay(args->delay);

        val ++;
        apply_delay(args->delay);

        args->arr->arr[pos] = val;
        apply_delay(args->delay);
        mtx_unlock(&(args->mutex[pos]));
    }
    

    return 0;
}


void print_array(struct array arr) {
    int total = 0;

    for(int i = 0; i < arr.size; i++) {
        total += arr.arr[i];
        printf("%d ", arr.arr[i]);
    }

    printf("\nTotal: %d\n", total);
}


int main (int argc, char **argv)
{
    struct options       opt;
    struct array         arr;
    srand(time(NULL));
    

    // Default values for the options
    opt.num_threads  = 5;
    opt.size         = 10;
    opt.iterations   = 100;
    opt.delay        = 1000;

    read_options(argc, argv, &opt);

    struct args *args= malloc(sizeof(struct args)*opt.num_threads);
    thrd_t *thr = malloc(sizeof(thrd_t)*opt.num_threads);
    mtx_t * mutex = malloc(sizeof(mtx_t)*opt.size);
    for(int i = 0; i < opt.size; i++) {
        mtx_init(&mutex[i],mtx_plain);
    }

    arr.size = opt.size;
    arr.arr  = malloc(arr.size * sizeof(int));

    memset(arr.arr, 0, arr.size * sizeof(int));    

    for(int i = 0; i< opt.num_threads;i++){
        args[i].iterations = opt.iterations;
        args[i].delay = opt.delay;
        args[i].arr = &arr;
        args[i].id = i;
        args[i].mutex = mutex;
        thrd_create(&thr[i],increment,&args[i]);
    }
    for(int i = 0; i< opt.num_threads;i++){
        thrd_join(thr[i],NULL);
    }
    mtx_destroy(mutex);
    print_array(arr);
    free(args);
    free(thr);
    free(mutex);
    free(arr.arr);
    return 0;
}
