#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define STATE_NONE 0
#define STATE_SYN 1
#define STATE_ACK 2
#define STATE_DONE 3
#define STATE_FIN 4

typedef struct _packet {
    struct _packet* next;
    int id;
} packet;

typedef struct _queue {
    packet* first;
    packet* last;
    int counter;
} queue;

queue** createQueueArr(int size);
void deleteQueueArr(queue** q, int size);
void push(queue* q, int id);
int pop(queue* q);
void clearQueue(queue* q);