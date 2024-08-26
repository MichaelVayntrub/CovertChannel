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
#include <netdb.h>

#include "queue.h"
#include "utility.h"

typedef struct Thread_Data {
    int id;
    char* ip;
    int port;
    int send_count;
    int* cond_channels_signal;
    pthread_cond_t* cond_channels;
} Thread_Data;

typedef struct Conf_Data {
    char* ip;
    char* proto;
    char* file;
    char* crypt;
} Conf_Data;

typedef struct Port_Queue {
    queue* qport;
    int srcport;
    int isFin;
} Port_Queue;

typedef struct Ports_Cache {
    Port_Queue ports[5];
    int port_index;
} Ports_Cache;



void InitChannelsMutexes();
void DestroyChannelsMutexes();
Thread_Data* Create_Data(int id, char* ip, int port, int send_count, int* cond_channels_signal, 
                               pthread_cond_t* cond_channels);
void* RunHTTP_Channel(void* arg);
void* RunTCP_Channel(void* arg);
void WaitForStart();
void* make_verdict(void* args);
int get_next_bit();

int callback1(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data);
int callback2(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data);


int callbackHTTP(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                 struct nfq_data *nfa, void *data);

int callbackHTTP1(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                 struct nfq_data *nfa, void *data);
int callbackHTTP2(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                 struct nfq_data *nfa, void *data);


void* RunTestHTTP_Channel(void* arg); //test//
int Connect_To_Host(Conf_Data* data);
void Send_UDP_Response(char* ip, char* message);
void Free_Conf_Data(Conf_Data* data);
int configure_qhandlers(char* proto);
void* http_verdict_thread(void* args);
void* verdict_thread(void* args);
int check_http_path(unsigned char *data, int len);
int Establish_connection(Conf_Data* data, char* ip);

void Init_Ports_Cache(Ports_Cache* cache);
void Add_Port(Ports_Cache* cache, int port);
int Check_Port(int port);