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
        int * cnt;
        int * cnt2;
        mtx_t * cntMtx;
        mtx_t * cntMtx2;
        
};


void apply_delay(int delay) {
    for(int i = 0; i < delay * DELAY_SCALE; i++); // waste time
}


int increment(void *args2)
{
    struct args *args = args2;
    int pos, val;
   
    while(1){
        mtx_lock(args->cntMtx);
        if(*(args->cnt)>=args->iterations){
            mtx_unlock(args->cntMtx);
            break;}
        *(args->cnt)=*(args->cnt) +1;
        mtx_unlock(args->cntMtx);
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

int increment_decrement(void *args2)
{
    struct args *args = args2;
    int pos,pos2, val,val2;
    
    while(1){
        mtx_lock(args->cntMtx2);
        if(*(args->cnt2)>=args->iterations){
            mtx_unlock(args->cntMtx2);
            break;}
        *(args->cnt2)=*(args->cnt2) +1;
        mtx_unlock(args->cntMtx2);
        pos = rand() % args->arr->size;
        pos2 = rand() % args->arr->size;

        printf("%ld increasing position %d *\n", args->id, pos);
        printf("%ld decreasing position %d *\n", args->id, pos);
        mtx_lock(&(args->mutex[pos]));
        val = args->arr->arr[pos];
        apply_delay(args->delay);

        val ++;
        apply_delay(args->delay);

        args->arr->arr[pos] = val;
        
        apply_delay(args->delay);
        mtx_unlock(&(args->mutex[pos]));

        mtx_lock(&(args->mutex[pos2]));
        val2 = args->arr->arr[pos2];
        apply_delay(args->delay);

        val2--;
        apply_delay(args->delay);

        args->arr->arr[pos2] = val2;

        apply_delay(args->delay);
        mtx_unlock(&(args->mutex[pos2]));
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
    thrd_t *thr2 = malloc(sizeof(thrd_t)*opt.num_threads);
    mtx_t * cntMtx = malloc(sizeof(mtx_t));
    mtx_t * cntMtx2 = malloc(sizeof(mtx_t));
    mtx_init(cntMtx,mtx_plain);
    mtx_init(cntMtx2,mtx_plain);
    int * cnt = malloc(sizeof(int));
    int * cnt2 = malloc(sizeof(int));
    *cnt = 0;
    *cnt2 = 0;


    arr.size = opt.size;
    arr.arr  = malloc(arr.size * sizeof(int));

    memset(arr.arr, 0, arr.size * sizeof(int));    

    for(int i = 0; i< opt.num_threads;i++){
        args[i].iterations = opt.iterations;
        args[i].delay = opt.delay;
        args[i].arr = &arr;
        args[i].id = i;
        args[i].mutex = mutex;
        args[i].cntMtx = cntMtx;
        args[i].cntMtx2 = cntMtx2;
        args[i].cnt = cnt;
        args[i].cnt2 = cnt2;

        thrd_create(&thr[i],increment,&args[i]);
        thrd_create(&thr2[i],increment_decrement,&args[i]);
    }
    for(int i = 0; i< opt.num_threads;i++){
        thrd_join(thr[i],NULL); 
        thrd_join(thr2[i],NULL);
    }
    mtx_destroy(mutex);
    mtx_destroy(cntMtx);
    mtx_destroy(cntMtx2);
    print_array(arr);
    free(args);
    free(thr);
    free(thr2);
    free(mutex);
    free(cntMtx);
    free(cntMtx2);
    free(cnt);
    free(cnt2);
    free(arr.arr);
    return 0;
}
