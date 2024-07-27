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
// Global time keepers
time_t current_time, previous_time;
// Queues array
queue** qs;
// Cover message
int *covert_message, cm_size;
// Message control
int indexId, indexMsg, statePacket[2], seq[2], index_verdict;
// Mode selector
int mode = 1 ? MODE_STREAM : MODE_TIMED;


// Main -----------------------------------------------------------------------------------------------

int main() 
{   
    // Declarations
    pthread_t verdict_tid, tcpc1_tid, tcpc2_tid;
    Thread_Data *data1, *data2;
    char buf[4096] __attribute__((aligned));
    Conf_Data conf_data;
    int n, fd;
    int connected_to_host = false; //temp??
    running = true;

    // Loading modules
    if(LoadNFLinkModules()) {
        exit(EXIT_FAILURE);
    }

    // Synchronization parameters initialization
    statePacket[0] = statePacket[1] = 0;
    seq[0] = seq[1] = 0;
    indexId = indexMsg = 0;
    index_verdict = 1;
    InitChannelsMutexes();


    // NFQUEUE configurations
    qh = (struct nfq_q_handle**)malloc(sizeof(struct nfq_q_handle*) * 2);
    qs = createQueueArr(2);
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

    // Start program run
    while (running) {
        
        if (!connected_to_host) //temp//
        {
            // Try connecting to the host
            if (Establish_connection(&conf_data, LOCAL_HOST)) {
                connected_to_host = true;
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

            // Verdict thread initialization
            int is_http = configure_qhandlers(conf_data.proto);
            void* verdict = is_http ? http_verdict_thread : verdict_thread;
            if (pthread_create(&verdict_tid, NULL, verdict, NULL) != 0 ) {
                fprintf(stderr, "Error creating thread\n");
                return 1;
            }
            
            // Channel threads initialization
            if (is_http) {
                //data1 = Create_Data(0, LOCAL_HOST, SERVER_HTTP_PORT, send_count, &cond_channels_signal, &cond_channels);
                //pthread_create(&tcpc1_tid, NULL, RunTestHTTP_Channel, data1);

                data1 = Create_Data(0, LOCAL_HOST, SERVER_HTTP_PORT, send_count / 2 - 1, &cond_channels_signal, &cond_channels);
                data2 = Create_Data(1, LOCAL_HOST, SERVER_HTTP_PORT, send_count / 2 - 1, &cond_channels_signal, &cond_channels);
                pthread_create(&tcpc1_tid, NULL, RunTestHTTP_Channel, data1);
                pthread_create(&tcpc2_tid, NULL, RunTestHTTP_Channel, data2);
            } else {
                data1 = Create_Data(0, LOCAL_HOST, 54321, send_count / 2 - 1, &cond_channels_signal, &cond_channels);
                data2 = Create_Data(1, LOCAL_HOST, 54322, send_count / 2 - 1, &cond_channels_signal, &cond_channels);
                pthread_create(&tcpc1_tid, NULL, RunTCP_Channel, data1);
                pthread_create(&tcpc2_tid, NULL, RunTCP_Channel, data2);
            }

            sending = true;
        }
        
        // Start a message sending session
        while (sending) {
            if ((n = recv(fd, buf, sizeof(buf), 0)) >= 0) {
                nfq_handle_packet(h, buf, n);
            } else {
                perror("recv");
                exit(EXIT_FAILURE);
            }
        }
        

        //if (data1 != NULL) free(data1);
        //if (data1 != NULL && is_http) {) free(data2);

        // NFQUEUE cleanup
        //if(qh[0] != NULL) nfq_destroy_queue(qh[0]);
        //if(qh[1] != NULL) nfq_destroy_queue(qh[1]);
        //free(qh);
        //nfq_close(h);
        //deleteQueueArr(qs, 2);
    }

    printf(ANSI_COLOR_GREEN "Closing the program...\n" ANSI_COLOR_RESET);

    // Threads closing
    //pthread_join(tcpc1_tid, NULL);
    //pthread_join(tcpc2_tid, NULL);
    //pthread_join(verdict_tid, NULL);

    // Cleanup
    //free(covert_message);
    //Free_Conf_Data(&conf_data);

    // Mutexes destruction
    /*
    pthread_mutex_destroy(&mutex_recv);
    pthread_mutex_destroy(&mutex_fin);
    pthread_mutex_destroy(&mutex_callback1);
    pthread_mutex_destroy(&mutex_callback2);
    DestroyChannelsMutexes();
    */

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

    while (qs[1]->counter <= 0);
    id = pop(qs[1]);
    printf("id = %d\n", id);
    while (qs[1]->counter <= 0);
    id = pop(qs[1]);
    printf("id = %d\n", id);
    while(id != 0) {
        nfq_set_verdict(qh[0], id, NF_ACCEPT, 0, NULL);
         while (qs[1]->counter <= 0);
         id = pop(qs[1]);
         printf("id = %d\n", id);
    }

    while (qs[0]->counter <= 0);
    id = pop(qs[0]);
    printf("id = %d\n", id);
    while (qs[0]->counter <= 0);
    id = pop(qs[0]);
    printf("id = %d\n", id);
    while(id != 0) {
        nfq_set_verdict(qh[0], id, NF_ACCEPT, 0, NULL);
         while (qs[0]->counter <= 0);
         id = pop(qs[0]);
         printf("id = %d\n", id);
    }


    
}


/*
void* http_verdict_thread(void* args) 
{
    int id, bit = 0, i;
    int channel = 0;
    if (cm_size > 0) {
        bit = covert_message[0];
    }

    previous_time = time(NULL);
    cond_channels_signal = 0;
    pthread_cond_signal(&cond_channels);

    while (true) {
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
            running_verdict = false;
        }
        
        struct timespec ts = {0, 100000000};
        nanosleep(&ts, NULL);
    }
}
*/

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
            running_verdict = false;
        }
        
        struct timespec ts = {0, 100000000};
        nanosleep(&ts, NULL);
    }
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

int callbackHTTP(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data) {
    struct nfqnl_msg_packet_hdr *ph;
    int srcport = 0;
    struct tcphdr *tcp_header = get_tcphdr_and_dport(nfa, &srcport, HTTP);
    ph = nfq_get_msg_packet_hdr(nfa);

    if (ph) {
        int id = ntohl(ph->packet_id);
        
        
        if (tcp_header->syn) {
            pthread_mutex_lock(&mutex_callback1);
            index_verdict = 1 - index_verdict;
            seq[index_verdict] = srcport;
            push(qs[index_verdict], 0);
            printf("id = %d, push = %d\n", index_verdict, 0);
            pthread_mutex_unlock(&mutex_callback1);
        }

        if (seq[0] == srcport) {
            //push(qs[0], id);
            printf("id = %d, push = %d\n", 0, id);
        } else if (seq[1] == srcport) {
            //push(qs[1], id);
            printf("id = %d, push = %d\n", 1, id);
        } else {
            printf("ERRORITA\n");
        }
        
        return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
    }
    return 0;
}


int callbackHTTP1(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data) {
    struct nfqnl_msg_packet_hdr *ph;
    int srcport = 0;
    struct tcphdr *tcp_header = get_tcphdr_and_dport(nfa, &srcport, HTTP);
    ph = nfq_get_msg_packet_hdr(nfa);

    if (ph) {
        int id = ntohl(ph->packet_id);
        
        if (tcp_header->syn) {
        printf("my name is %d: port = %d\n", 1, srcport);
            //push(qs[0], 0);
        }
        //push(qs[0], id);
        return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
    }
    return 0;
}

int callbackHTTP2(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data) {
    struct nfqnl_msg_packet_hdr *ph;
    int srcport = 0;
    struct tcphdr *tcp_header = get_tcphdr_and_dport(nfa, &srcport, HTTP);
    ph = nfq_get_msg_packet_hdr(nfa);
    printf("YO YO\n");

    if (ph) {
        int id = ntohl(ph->packet_id);
        
        //if (tcp_header->syn) {
        printf("my name is %d: port = %d\n", 2, srcport);
            //push(qs[0], 0);
        //}
        //push(qs[0], id);
        return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
    }
    return 0;
}

/*
int callbackHTTP(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                  struct nfq_data *nfa, void *data) 
{
    struct nfqnl_msg_packet_hdr *ph;
    struct iphdr *ip_header;
    struct tcphdr *tcp_header;
    int id = 0, path = 0;
    unsigned char *packet_data;
    int payload_len;
    int path2 = 0;

    ph = nfq_get_msg_packet_hdr(nfa);
    if (ph) {
        id = ntohl(ph->packet_id);
        push(qs[0], id);
    }

    payload_len = nfq_get_payload(nfa, &packet_data);
    if (payload_len >= 0) {
        ip_header = (struct iphdr *) packet_data;

        // Check if the packet is a TCP packet
        if (ip_header->protocol == IPPROTO_TCP) {

            tcp_header = (struct tcphdr *) (packet_data + ip_header->ihl * 4);

            // Check if the TCP destination port is HTTP_PORT
            if (ntohs(tcp_header->dest) == SERVER_HTTP_PORT) {
                unsigned char *http_data = packet_data + ip_header->ihl * 4 + tcp_header->doff * 4;
                int http_data_len = payload_len - (ip_header->ihl * 4 + tcp_header->doff * 4);

                if (http_data_len > 0) {
                    path = check_http_path(http_data, http_data_len);
                    if (path != -1) {
                        push(qs[path], id);
                        return 0;
                        //uint32_t seq = ntohl(tcp_header->seq);
                        //printf("Here2 -- ack = %d, syn = %d, seq = %u\n", tcp_header->ack, tcp_header->syn, seq);
                        //return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
                    }
                }
            }
            //printf("id = %d\n", id);
            //uint32_t seq2 = ntohl(tcp_header->seq);
            //printf("ack = %d, syn = %d, seq = %u\n", tcp_header->ack, tcp_header->syn, seq2);
        }
    }
    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}
*/

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
            running = STATE_FIN - statePacket[1];
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
            running = STATE_FIN - statePacket[0];
            pthread_mutex_unlock(&mutex_fin);
        }
    }
    return 0;
}

// Function to find the path from HTTP request
int check_http_path(unsigned char *data, int len) 
{
    char *http_data = (char *)data;
    char *method, *path, *version;

    http_data[len] = '\0';

    // Find the HTTP request line
    method = strtok(http_data, " ");
    if (method) {
        path = strtok(NULL, " ");
        if (path) {
            version = strtok(NULL, "\r\n");
            if (version) {
                if (strcmp(path, "/a") == 0) return 0;
                if (strcmp(path, "/b") == 0) return 1;
                return -1;
            }
        }
    }
    return -1;
}