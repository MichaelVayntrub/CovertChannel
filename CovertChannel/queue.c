#include "queue.h"

pthread_mutex_t mutexQ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexP = PTHREAD_MUTEX_INITIALIZER;

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

Cache** createCacheArr(int size)
{
    Cache** cArr = malloc(size * sizeof(Cache));
    if (cArr != NULL) {
        for (int i = 0; i < size; i++) {
            cArr[i] = malloc(sizeof(Cache));
            cArr[i]->first = cArr[i]->last = NULL;
            cArr[i]->counter = 0;
        }
    }
    return cArr;
}

void deleteCacheArr(Cache** cArr, int size) 
{
    if (cArr != NULL) {
        for (int i = 0; i < size; i++) {
            Cache* cachePtr = cArr[i];
            while (cachePtr != NULL) {
                removePortsCache(cachePtr);
            }
            free(cArr[i]);
        }
        free(cArr);
    }
    pthread_mutex_destroy(&mutexP);
}

void addPortsCache(int port, Cache* cache) {
    portsCache* cachePtr = (portsCache*)malloc(sizeof(portsCache));
    cachePtr->next = NULL;
    cachePtr->q = (queue*)malloc(sizeof(queue));
    cachePtr->q->first = cachePtr->q->last = NULL;
    cachePtr->q->counter = 0;
    cachePtr->isFin = 0;
    cachePtr->port = port;

    pthread_mutex_lock(&mutexP);
    if (cache->first == NULL) {
        cache->first = cachePtr;
        cache->last = cachePtr;
    } else {
        cache->last->next = cachePtr;
        cache->last = cachePtr;
    }
    cache->counter++;
    pthread_mutex_unlock(&mutexP);
}


portsCache* findPortsCache(int port, portsCache* cache) {
    if (cache == NULL) return NULL;
    portsCache* temp = cache;

    while (temp != NULL) {
        if (temp->port == port) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

void removePortsCache(Cache* cache) {
    pthread_mutex_lock(&mutexP);
    portsCache* cachePtr = cache->first;
    if (cachePtr == NULL) {
        pthread_mutex_unlock(&mutexP);
        return;
    }
    cache->first = cachePtr->next;
    if (cache->first == NULL || cache->first->next == NULL ) {
        cache->last = cache->first;
    }
    cache->counter--;
    pthread_mutex_unlock(&mutexP);

    clearQueue(cachePtr->q);
    free(cachePtr);
}



