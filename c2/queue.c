#include <stdlib.h>
#include <threads.h>


// circular array
typedef struct _queue {
    int size;
    int used;
    int first;
    void **data;
    mtx_t mutex;
    cnd_t FULL,EMPTY;
    int finished;
} _queue;

#include "queue.h"

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));
    mtx_init(&q->mutex,mtx_plain);
    cnd_init(&q->FULL);
    cnd_init(&q->EMPTY);

    q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->finished=-1;
    q->data  = malloc(size * sizeof(void *));

    return q;
}

int q_elements(queue q) {
    return q->used;
}

int q_insert(queue q, void *elem) {
    mtx_lock(&q->mutex);
    while(q->size == q->used){
        if(q->finished==0){
        mtx_unlock(&q->mutex);
        return -1;}
    cnd_wait(&q->FULL,&q->mutex);}

    q->data[(q->first + q->used) % q->size] = elem;
    q->used++;

    if(q->used==1) cnd_broadcast(&q->EMPTY);
    mtx_unlock(&q->mutex);

    return 0;
}

void *q_remove(queue q) {
    void *res;
    mtx_lock(&q->mutex);
    while(q->used == 0){
    if(q->finished==0){
        mtx_unlock(&q->mutex);
        return NULL;}
    cnd_wait(&q->EMPTY,&q->mutex);}

    res = q->data[q->first];

    q->first = (q->first + 1) % q->size;
    q->used--;

    if(q->used == q->size -1) cnd_broadcast(&q->FULL);
    mtx_unlock(&q->mutex);

    return res;
}
void q_finish(queue q){
    mtx_lock(&q->mutex);
    q->finished=0;
    cnd_broadcast(&q->EMPTY);
    mtx_unlock(&q->mutex);
}


void q_destroy(queue q) {
    mtx_destroy(&q->mutex);
    cnd_destroy(&q->FULL);
    cnd_destroy(&q->EMPTY);
    free(q->data);
    free(q);
}
