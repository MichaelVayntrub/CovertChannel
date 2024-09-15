#include "monitor.h"
const int MIN_SEND_VAL = MIN_SEND_COUNT * CHAR_SIZE_XOR_EXPAND;

// Global mutexes
pthread_mutex_t mutex_fin = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_recv = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_callback1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_callback2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_channels = PTHREAD_COND_INITIALIZER;
static int cond_channels_signal = -1;
int cond_signal = 0;
// NFQUEUE handlers
struct nfq_handle *h;
struct nfq_q_handle **qh;
struct nfnl_handle *nh;
// Loop flags
int running = false;
int sending = false;
int running_verdict = true;
int send_on = true;
// Global time keepers
time_t current_time, previous_time;
// Queues array
queue** qs;
Cache** caches;
int http_port_index;
// Cover message
int *covert_message, cm_size, index_cm;
// Message control
int indexId, indexMsg, statePacket[2], seq[2], index_verdict;
// Mode selector
int mode = 1 ? MODE_STREAM : MODE_TIMED;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int condition = 0; // Shared variable to signal the condition

volatile sig_atomic_t keep_running = 1;
// Signal handler to set keep_running to 0
void handle_signal(int sig) {
    keep_running = 0;
}
void send_interrupt_signal(pid_t pid) {
    if (kill(pid, SIGUSR1) == -1) {
        perror("kill");
        exit(EXIT_FAILURE);
    }
}


// Main -----------------------------------------------------------------------------------------------

int main() 
{   
    // Declarations
    pthread_t verdict_tid, channel1_tid, channel2_tid;
    Thread_Data *data1, *data2;
    char buf[4096] __attribute__((aligned));
    Conf_Data conf_data;
    int n, fd;
    running = true, sending = false;

    // Loading modules
    if(LoadNFLinkModules()) {
        exit(EXIT_FAILURE);
    }

    // Start program run
    while (running) {
        // Reset Variables
        cond_channels_signal = -1;
        cond_signal = 0;
        running_verdict = true;
        // Set up signal handler for SIGUSR1
        keep_running = true;
        struct sigaction sa;
        sa.sa_handler = handle_signal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGUSR1, &sa, NULL) == -1) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
        // Try connecting to the host
        if (Establish_connection(&conf_data, LOCAL_HOST)) {
            WaitForStart();
        } else {
            continue;
        }
        // Update iptables rules according to the protocol
        if (UpdateIptables(conf_data.proto)) {
            exit(EXIT_FAILURE);
        }
        // Read and convert to bit order from a text file
        if ((covert_message = ReadTextFile(&cm_size, conf_data.file)) == NULL) {
            exit(EXIT_FAILURE);
        }
        PrintTxtInput(covert_message, cm_size);  // option - print input(after cenversion)
        int send_count = fmax(cm_size, MIN_SEND_VAL);
        // Synchronization parameters initialization
        statePacket[0] = statePacket[1] = 0;
        seq[0] = seq[1] = 0;
        indexId = indexMsg = index_cm = 0;
        http_port_index = 0;
        InitChannelsMutexes();
        
        // NFQUEUE configurations
        qh = (struct nfq_q_handle**)malloc(sizeof(struct nfq_q_handle*) * 2);
        qs = createQueueArr(2);
        caches = createCacheArr(2);
        h = nfq_open();
        if (!h) {
            fprintf(stderr, "Error: nfq_open()\n");
            exit(EXIT_FAILURE);
        }
        if (nfq_unbind_pf(h, AF_INET) < 0) {
            fprintf(stderr, "Error: nfq_unbind_pf()\n");
            exit(EXIT_FAILURE);
        }
        if (nfq_bind_pf(h, AF_INET) < 0) {
            fprintf(stderr, "Error binding nfq\n");
            exit(EXIT_FAILURE);
        }
        nh = nfq_nfnlh(h);
        fd = nfnl_fd(nh);
        // Verdict thread initialization
        int is_http = configure_qhandlers(conf_data.proto);
        void* verdict = is_http ? http_verdict_thread : verdict_thread;
        if (pthread_create(&verdict_tid, NULL, verdict, NULL) != 0 ) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
        // Channel threads initialization
        if (is_http) {
            data1 = Create_Data(0, LOCAL_HOST, SERVER_HTTP_PORT, send_count / 2 - 1, &cond_channels_signal, &cond_channels);
            data2 = Create_Data(1, LOCAL_HOST, SERVER_HTTP_PORT, send_count / 2 - 1, &cond_channels_signal, &cond_channels);
            pthread_create(&channel1_tid, NULL, RunTestHTTP_Channel, data1);
            pthread_create(&channel2_tid, NULL, RunTestHTTP_Channel, data2);
        } else {
            data1 = Create_Data(0, LOCAL_HOST, 54321, send_count / 2 - 1, &cond_channels_signal, &cond_channels);
            data2 = Create_Data(1, LOCAL_HOST, 54322, send_count / 2 - 1, &cond_channels_signal, &cond_channels);
            pthread_create(&channel1_tid, NULL, RunTCP_Channel, data1);
            pthread_create(&channel2_tid, NULL, RunTCP_Channel, data2);
        }
        sending = send_on = true;
        
        // Start a message sending session
        while (keep_running) {
            if ((n = recv(fd, buf, sizeof(buf), 0)) >= 0) {
                nfq_handle_packet(h, buf, n);
            } else if (errno != EINTR) {
                perror("recv");
                exit(EXIT_FAILURE);
            }
        }
        printf(ANSI_COLOR_GREEN "Finished sending proccess\n\n" ANSI_COLOR_RESET);

        // NFQUEUE cleanup
        if(qh[0] != NULL) nfq_destroy_queue(qh[0]);
        if(qh[1] != NULL) nfq_destroy_queue(qh[1]);
        free(qh);
        nfq_close(h);
        deleteQueueArr(qs, 2);
        //deleteCacheArr(caches, 2);

        // Threads closing
        pthread_join(channel1_tid, NULL);
        pthread_join(channel2_tid, NULL);
        pthread_join(verdict_tid, NULL);

        // Cleanup data
        if (data1 != NULL) free(data1);
        if (data2 != NULL) free(data2);
        free(covert_message);
        Free_Conf_Data(&conf_data);
    }
    // Mutexes destruction
    pthread_mutex_destroy(&mutex_recv);
    pthread_mutex_destroy(&mutex_fin);
    pthread_mutex_destroy(&mutex_callback1);
    pthread_mutex_destroy(&mutex_callback2);
    DestroyChannelsMutexes();

    return 0;
}


// Functions ------------------------------------------------------------------------------------------

int configure_qhandlers(char* proto)
{   
    int is_http = !strcmp(proto, "http");
    void* func_callback1 = is_http ? callbackHTTP : callback1;
    
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            qh[i] = nfq_create_queue(h, i, func_callback1, NULL);
        }
        else if (i == 1 && !is_http) {
            qh[i] = nfq_create_queue(h, i, callback2, NULL);
        }
        else {
            qh[i] = NULL;
            continue;
        }
        if (!qh[i]) {
            fprintf(stderr, "Error: nfq_create_queue()\n");
            exit(EXIT_FAILURE);
        }
        if (nfq_set_mode(qh[i], NFQNL_COPY_PACKET, 0xffff) < 0) {
            fprintf(stderr, "Error: nfq_set_mode() failed\n");
            exit(EXIT_FAILURE);
        }
        if (nfq_set_queue_maxlen(qh[i], 200) == -1) {
            fprintf(stderr, "Error: nfq_set_queue_maxlen()\n");
            exit(EXIT_FAILURE);
        }
    }
    return is_http;
}

void* http_verdict_thread(void* args)
{
    int id, bit = 0, i;
    int channel = 0;
    if (cm_size > 0) {
        bit = covert_message[0];
    }  

    previous_time = time(NULL);
    //cond_channels_signal = 0;
    //pthread_cond_signal(&cond_channels);

    while (running_verdict) {
        
        /*
        current_time = time(NULL);
        if (current_time != previous_time || cond_channels_signal * mode) {
            channel = 1 - channel;
            cond_channels_signal = channel;
            previous_time = current_time;
            pthread_cond_signal(&cond_channels);
        }
        */
        
        if (running || indexId < cm_size) {
            
            if (caches[bit]->counter > 0) 
            {
                i = bit;
                pop_ports_cache(caches[i]->first->q);
                
                if (caches[i]->first->isFin) {
                    removePortsCache(caches[i]);
                    bit = get_next_bit();
                }
            }
            
        } 
        if (!send_on) {
            while (caches[0]->counter > 0) {
                pop_ports_cache(caches[0]->first->q);
                if (caches[0]->first->isFin) {
                    removePortsCache(caches[0]);
                }
            }
            while (caches[1]->counter > 0) {
                pop_ports_cache(caches[1]->first->q);
                if (caches[1]->first->isFin) {
                    removePortsCache(caches[1]);
                }
            }
            running_verdict = sending = false;
        }
        
        struct timespec ts = {0, 100000000};
        nanosleep(&ts, NULL);
    }
    pid_t my_pid = getpid(); // Get the PID of the running process
    send_interrupt_signal(my_pid); // Send the interrupt signal
}

void* verdict_thread(void* args) 
{
    int id, bit = 0, i;
    int channel = 0;
    if (cm_size > 0) {
        bit = covert_message[0];
    }

    previous_time = time(NULL);
    cond_channels_signal = 0;
    pthread_cond_signal(&cond_channels);

    while (running_verdict) {
        current_time = time(NULL);
        if (current_time != previous_time || cond_channels_signal * mode) {
            channel = 1 - channel;
            cond_channels_signal = channel;
            previous_time = current_time;
            pthread_cond_signal(&cond_channels);
        }

        if (running || indexId < cm_size) {
            if (qs[bit]->counter > 0) 
            {
                i = bit;
                id = pop(qs[bit]);
                bit = get_next_bit();
                nfq_set_verdict(qh[i], id, NF_ACCEPT, 0, NULL);
            }
        } else {
            while (qs[0]->counter > 0 || qs[1]->counter > 0) {

                if (qs[0]->counter > 0 && qs[1]->counter > 0) {
                    bit = indexId++ % 2;
                }
                else if (qs[1]->counter > 0) bit = 1;
                else bit = 0;

                id = pop(qs[bit]);
                nfq_set_verdict(qh[bit], id, NF_ACCEPT, 0, NULL);
            }
        }
        if (!send_on && qs[0]->counter == 0 && qs[1]->counter == 0)
            running_verdict = sending = false;
        
        struct timespec ts = {0, 100000000};
        nanosleep(&ts, NULL);
    }
    pid_t my_pid = getpid(); // Get the PID of the running process
    send_interrupt_signal(my_pid); // Send the interrupt signal
}

int get_next_bit()
{
    int res;
    indexId++;
    if (indexId < cm_size) {
        res =  covert_message[indexId];
    } else {
        res = indexId % 2;
    }
    return res;
}

void pop_ports_cache(queue* q) 
{
    while(q->counter > 0) {
        int id = pop(q);
        nfq_set_verdict(qh[0], id, NF_ACCEPT, 0, NULL);
    }
}

int callbackHTTP(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data) {
    struct nfqnl_msg_packet_hdr *ph;
    int srcport = 0;
    struct tcphdr *tcp_header = get_tcphdr_and_dport(nfa, &srcport, HTTP);
    ph = nfq_get_msg_packet_hdr(nfa);
    http_port_index = srcport % 2 == 0 ? 0 : 1;

    if (ph) {
        int id = ntohl(ph->packet_id);
        portsCache* cachePtr;
        
        pthread_mutex_lock(&mutex_callback1);
        if ((cachePtr = findPortsCache(srcport, caches[0]->first)) != NULL) { 
            push(cachePtr->q, id);
            //printf("1-%d\n",srcport);
        } 
        else if ((cachePtr = findPortsCache(srcport, caches[1]->first)) != NULL) {
            push(cachePtr->q, id);
            //printf("2-%d\n",srcport);
        }
        else {
            if (tcp_header->syn) {
                addPortsCache(srcport, caches[http_port_index]);
                push(caches[http_port_index]->first->q, id);
                pthread_mutex_lock(&mutex_fin);
                index_cm++;
                pthread_mutex_unlock(&mutex_fin);
                //printf("3-%d\n",srcport);
            } else {
                nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
            }
        }

        if (cachePtr != NULL && tcp_header->fin) {
            cachePtr->isFin = true;
            //printf("FIN-%d\n",srcport);
        }
        if (cm_size == index_cm && caches[0]->counter + caches[1]->counter <= 1) {
            //send_on = false;
        }
        pthread_mutex_unlock(&mutex_callback1);
    }
    return 0;
}

int callback1(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                  struct nfq_data *nfa, void *data) 
{
    int dport, id;
    struct nfqnl_msg_packet_hdr *hdr;
    struct tcphdr *tcp_header = get_tcphdr_and_dport(nfa, &dport, TCP);

    hdr = nfq_get_msg_packet_hdr(nfa);
    if (hdr) {
        id = ntohl(hdr->packet_id);
        
        if (statePacket[0] > 2 ) {
            if (seq[0] == tcp_header->seq) {
                nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
            } else { push(qs[0], id); }
        }
        else {
            (statePacket[0])++;
            if (statePacket[0] == 2) {
                push(qs[0], id);
            }
            else {
                nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
            }
        }
        pthread_mutex_lock(&mutex_callback1);
        seq[0] = tcp_header->seq;
        pthread_mutex_unlock(&mutex_callback1);

        if (tcp_header->fin) {
            pthread_mutex_lock(&mutex_fin);
            statePacket[0] = STATE_FIN;
            send_on = STATE_FIN - statePacket[1];
            pthread_mutex_unlock(&mutex_fin);
        }
    }
    return 0;
}

int callback2(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                  struct nfq_data *nfa, void *data) 
{
    int dport, id;
    struct nfqnl_msg_packet_hdr *hdr;
    struct tcphdr *tcp_header = get_tcphdr_and_dport(nfa, &dport, TCP);

    hdr = nfq_get_msg_packet_hdr(nfa);
    if (hdr) {
        id = ntohl(hdr->packet_id);
        
        if (statePacket[1] > 2) {
            if (seq[1] == tcp_header->seq) {
                nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
            } else { push(qs[1], id); }
        }
        else {
            (statePacket[1])++;
            if (statePacket[1] == 2) {
                push(qs[1], id);
            }
            else {
                nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
            }
        }
        pthread_mutex_lock(&mutex_callback1);
        seq[1] = tcp_header->seq;
        pthread_mutex_unlock(&mutex_callback1);

        if (tcp_header->fin) {
            pthread_mutex_lock(&mutex_fin);
            statePacket[1] = STATE_FIN;
            send_on = STATE_FIN - statePacket[0];
            pthread_mutex_unlock(&mutex_fin);
        }
    }
    return 0;
}
