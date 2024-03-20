#include "monitor.h"

const int MIN_SEND_VAL = MIN_SEND_COUNT * CHAR_SIZE_XOR_EXPAND;
pthread_mutex_t mutex_fin = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_recv = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_callback1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_callback2 = PTHREAD_MUTEX_INITIALIZER;
int cond_signal = 0;
struct nfq_handle *h;
struct nfq_q_handle **qh;
struct nfnl_handle *nh;
int running = false;

int *covert_message, cm_size;
char *server_ip;

queue** qs;
int indexId, indexMsg, statePacket[2], seq[2];

void* verdict_thread(void* args) 
{
    int id, bit = 0, i;
    if (cm_size > 0) {
        bit = covert_message[0];
    }

    while (running) {

        pthread_mutex_lock(&mutex_recv);
        while (!cond_signal) {
            pthread_cond_wait(&cond, &mutex_recv);
        }
        pthread_mutex_unlock(&mutex_recv);

        if (qs[bit]->counter > 0) 
        {
            i = bit;
            id = pop(qs[bit]);
            bit = get_next_bit();
            nfq_set_verdict(qh[i], id, NF_ACCEPT, 0, NULL);
        }
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

int callback1(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                  struct nfq_data *nfa, void *data) 
{
    int dport, id;
    struct nfqnl_msg_packet_hdr *hdr;
    struct tcphdr *tcp_header = get_tcphdr_and_dport(nfa, &dport);

    hdr = nfq_get_msg_packet_hdr(nfa);
    if (hdr) {
        id = ntohl(hdr->packet_id);
        
        if (statePacket[0] == 2) {
            (statePacket[0])++;
            push(qs[0], id);
        }
        else if(statePacket[0] > 2) {
            if (seq[0] == tcp_header->seq) {
                nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
            } else { push(qs[0], id); }
        }
        else {
            (statePacket[0])++;
            nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
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
    struct tcphdr *tcp_header = get_tcphdr_and_dport(nfa, &dport);

    hdr = nfq_get_msg_packet_hdr(nfa);
    if (hdr) {
        id = ntohl(hdr->packet_id);
        
        if (statePacket[1] == 2) {
            (statePacket[1])++;
            push(qs[1], id);
        }
        else if(statePacket[1] > 2) {
            if (seq[1] == tcp_header->seq) {
                nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
            } else { push(qs[1], id); }
        }
        else {
            (statePacket[1])++;
            nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
        }
        pthread_mutex_lock(&mutex_callback2);
        seq[1] = tcp_header->seq;
        pthread_mutex_unlock(&mutex_callback2);

        if (tcp_header->fin) {
            pthread_mutex_lock(&mutex_fin);
            statePacket[1] = STATE_FIN;
            running = STATE_FIN - statePacket[0];
            pthread_mutex_unlock(&mutex_fin);
        }
    }
    return 0;
}

int main() 
{
    pthread_t verdict_tid, tcpc1_tid, tcpc2_tid;
    char buf[4096] __attribute__((aligned));
    int n, fd;
    running = true;

    qh = (struct nfq_q_handle**)malloc(sizeof(struct nfq_q_handle*) * 2);
    statePacket[0] = statePacket[1] = 0;
    seq[0] = seq[1] = 0;
    indexId = indexMsg = 0;
    qs = createQueueArr(2);

    if(LoadNFLinkModules()) {
        exit(EXIT_FAILURE);
    }

    if(UpdateIptables()) {
       exit(EXIT_FAILURE);
    }

    if ((server_ip = WaitForHostIp()) == NULL) {
        exit(EXIT_FAILURE);
    }
    //printf("host_ip: %s", server_ip);  // test - host ip

    if ((covert_message = ReadTextFile(&cm_size)) == NULL) {
        exit(EXIT_FAILURE);
    }
    PrintTxtInput(covert_message, cm_size);  // test - print input(after cenversion)
    
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
    qh[0] = nfq_create_queue(h, 0, &callback1, NULL);
    if (!qh[0]) {
        fprintf(stderr, "Error: nfq_create_queue()\n");
        exit(EXIT_FAILURE);
    }

    if (nfq_set_mode(qh[0], NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "Error: nfq_set_mode() failed\n");
        exit(EXIT_FAILURE);
    }

    if (nfq_set_queue_maxlen(qh[0], 200) == -1) {
        fprintf(stderr, "Error: nfq_set_queue_maxlen()\n");
        exit(EXIT_FAILURE);
    }

    qh[1] = nfq_create_queue(h, 1, &callback2, NULL);
    if (!qh[1]) {
        fprintf(stderr, "Error: nfq_create_queue()\n");
        exit(EXIT_FAILURE);
    }

    if (nfq_set_mode(qh[1], NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "Error: nfq_set_mode() failed\n");
        exit(EXIT_FAILURE);
    }

    if (nfq_set_queue_maxlen(qh[1], 200) == -1) {
        fprintf(stderr, "Error: nfq_set_queue_maxlen()\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&verdict_tid, NULL, verdict_thread, NULL) != 0 ) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    int send_count = fmax(cm_size, MIN_SEND_VAL);
    ThreadTCP_Data* data1 = CreateTCP_Data(0, server_ip, SERVER_A_PORT, send_count / 2 - 1);
    ThreadTCP_Data* data2 = CreateTCP_Data(1, server_ip, SERVER_B_PORT, send_count / 2 - 1);
    pthread_create(&tcpc1_tid, NULL, RunTCP_Channel, data1);
    sleep(1);
    pthread_create(&tcpc2_tid, NULL, RunTCP_Channel, data2);

    while (running) {
        if ((n = recv(fd, buf, sizeof(buf), 0)) >= 0) {
            pthread_mutex_lock(&mutex_recv);
            nfq_handle_packet(h, buf, n);
            cond_signal = 1;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex_recv);
        } else {
            perror("recv");
            exit(EXIT_FAILURE);
        }
    }
    
    pthread_join(tcpc1_tid, NULL);
    pthread_join(tcpc2_tid, NULL);
    pthread_join(verdict_tid, NULL);
    printf(ANSI_COLOR_GREEN "Closing the program...\n" ANSI_COLOR_RESET);
    nfq_destroy_queue(qh[0]);
    nfq_destroy_queue(qh[1]);
    free(qh);
    nfq_close(h);
    free(data1);
    free(data2);
    free(server_ip);
    free(covert_message);
    deleteQueueArr(qs, 2);
    pthread_mutex_destroy(&mutex_recv);
    pthread_mutex_destroy(&mutex_fin);
    pthread_mutex_destroy(&mutex_callback1);
    pthread_mutex_destroy(&mutex_callback2);
    return 0;
}