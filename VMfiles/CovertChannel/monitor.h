#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include "queue.h"
#include "utility.h"

typedef struct ThreadTCP_Data {
    int id;
    char* ip;
    int port;
    int send_count;
    int* cond_channels_signal;
    pthread_cond_t* cond_channels;
} ThreadTCP_Data;

void InitChannelsMutexes();
void DestroyChannelsMutexes();
ThreadTCP_Data* CreateTCP_Data(int id, char* ip, int port, int send_count, int* cond_channels_signal, 
                               pthread_cond_t* cond_channels);
void* RunTCP_Channel(void* arg);
char* WaitForHostIp();
void* make_verdict(void* args);
int get_next_bit();

int callback1(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data);
int callback2(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data);
