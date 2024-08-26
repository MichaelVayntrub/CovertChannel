#include "queue.h"

pthread_mutex_t mutexQ = PTHREAD_MUTEX_INITIALIZER;

queue** createQueueArr(int size)
{
    queue** qArr = malloc(size * sizeof(queue));
    if (qArr != NULL) {
        for (int i = 0; i < size; i++) {
            qArr[i] = malloc(sizeof(queue));
            qArr[i]->first = qArr[i]->last = NULL;
            qArr[i]->counter = 0;
        }
    }
    return qArr;
}

void deleteQueueArr(queue** qArr, int size) 
{
    if (qArr != NULL) {
        for (int i = 0; i < size; i++) {
            clearQueue(qArr[i]);
            free(qArr[i]);
        }
        free(qArr);
    }
    pthread_mutex_destroy(&mutexQ);
}

void push(queue* q, int id)
{
    pthread_mutex_lock(&mutexQ);
    packet* newPkt = (packet*)malloc(sizeof(packet));
    newPkt->next = NULL;
    newPkt->id = id;

    if (q->counter == 0) {
        q->last = newPkt;
        q->first = newPkt;
    } else {
        q->last->next = newPkt;
        q->last = newPkt;
    }
    q->counter++;
    pthread_mutex_unlock(&mutexQ);
}

int pop(queue* q)
{
    pthread_mutex_lock(&mutexQ);
    packet* oldPkt;
    int id;

    if (q->counter > 0) {
        oldPkt = q->first;
        q->first = q->first->next;
        id = oldPkt->id;
        free(oldPkt);
        q->counter--;
    } else {
        pthread_mutex_unlock(&mutexQ);
        return -1;
    }
    pthread_mutex_unlock(&mutexQ);
    return id;
}

void clearQueue(queue* q)
{
    pthread_mutex_lock(&mutexQ);
    while (q->counter > 0) {
        packet* delPkt = q->first;
        q->first = q->first->next;
        free(delPkt);
        q->counter--;
    }
    pthread_mutex_unlock(&mutexQ);
}
