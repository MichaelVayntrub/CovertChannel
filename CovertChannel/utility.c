#include "utility.h"

static const char DEFAULT_BYTE =  0b01010101;
static const char REVERSE_BYTE =  0b10101010;

int* ReadTextFile(int *size, char* file_name)
{
    FILE *fptr;
    int* message;
    int i = 0;
    char c;

    fptr = fopen(file_name, "r");
    if (fptr == NULL) {
        printf("Error: couldn't open the file.\n");
        return NULL;
    }
    *size = 0;
    while (fread(&c, sizeof(char), 1, fptr)) (*size)++;
    message = (int*)malloc(sizeof(int) * ((*size) *= CHAR_SIZE_XOR_EXPAND));
    rewind(fptr);

    while (fread(&c, sizeof(char), 1, fptr)){
        CharToBinary(i, message, c);
        i += CHAR_SIZE_XOR_EXPAND;
    }
    fclose(fptr);
    return message;
}

void CharToBinary(int iStart, int *message, char c) 
{
    char left, right;
    int end_left = iStart + 7;
    int end_right = iStart + 15;
    MirroredXor(c, &left, &right);

    for (int i = 7; i >= 0; i--) {
        message[end_left - i] = (left >> i) & 1;
        message[end_right - i] = (right >> i) & 1;
    }
}

void MirroredXor(char c, char *left_res, char *right_res)
{
    c = c ^ DEFAULT_BYTE;
    char left_part = c & REVERSE_BYTE;
    char right_part = c & DEFAULT_BYTE;
    *left_res = (((~left_part >> 1) & DEFAULT_BYTE)) | left_part;
    *right_res = (((~right_part << 1) & REVERSE_BYTE)) | right_part;
}

void PrintTxtInput(int *covert_message, int cm_size)
{
    printf(ANSI_COLOR_YELLOW "Sending order: " ANSI_COLOR_RESET);
    if (cm_size == 0) {
        printf("default\n");
        return;
    }
    for (int i = 0; i < cm_size; i++) {
        if(i % 8 == 0) printf(" ");
        printf("%d", covert_message[i]);
    }
    printf("\n");
}

struct tcphdr* get_tcphdr_and_dport(struct nfq_data *nfa, int* port, int proto)
{
    unsigned char *pkt_data;
    int data_len = nfq_get_payload(nfa, &pkt_data);
    struct iphdr *ip_header = (struct iphdr*)pkt_data;
    struct tcphdr *tcp_header = (struct tcphdr *)(pkt_data + sizeof(struct iphdr));
    if (proto = TCP) *port = ntohs(tcp_header->dest);
    else *port = ntohs(tcp_header->source);
    return tcp_header;
}

int UpdateIptables(char* proto)
{
    char output1[150], output2[150];
    int flag = system("sudo -S iptables -F");
    flag |= system("sudo -S iptables -X");
    flag |= system("sudo -S iptables -Z");
    if (!strcmp(proto, "http")) {
        sprintf(output1, "sudo -S iptables -A OUTPUT -p tcp --dport %d -j NFQUEUE --queue-num %d", SERVER_HTTP_PORT, 0);
        flag |= system(output1);
    } else if (!strcmp(proto, "tcp")) {
        sprintf(output1, "sudo -S iptables -A OUTPUT -p tcp --dport %d -j NFQUEUE --queue-num %d", SERVER_TCPA_PORT, 0);
        sprintf(output2, "sudo -S iptables -A OUTPUT -p tcp --dport %d -j NFQUEUE --queue-num %d", SERVER_TCPB_PORT, 1);
        flag |= system(output1);
        flag |= system(output2);
    }
    
    if(flag) {
        printf(ANSI_COLOR_RED "Failed to update iptables\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_GREEN "Iptables updated\n" ANSI_COLOR_RESET);
    }
    return flag;
}

int LoadNFLinkModules()
{
    if (system("lsmod | grep -q '^nfnetlink'") == 0) {
        printf(ANSI_COLOR_GREEN "nfnetlink module already loaded\n" ANSI_COLOR_RESET);
    } else {
        if (system("sudo modprobe nfnetlink") == 0) {
            printf(ANSI_COLOR_GREEN "nfnetlink module loaded successfully\n" ANSI_COLOR_RESET);
        } else {
            printf(ANSI_COLOR_RED "Failed to load nfnetlink_queue module\n" ANSI_COLOR_RESET);
            return -1;
        }
    }
    if (system("lsmod | grep -q '^nfnetlink_queue '") == 0) {
        printf(ANSI_COLOR_GREEN "nfnetlink_queue module already loaded\n" ANSI_COLOR_RESET);
    } else {
        if (system("sudo modprobe nfnetlink_queue") == 0) {
            printf(ANSI_COLOR_GREEN "nfnetlink_queue module loaded successfully\n" ANSI_COLOR_RESET);
        } else {
            printf(ANSI_COLOR_RED "Failed to load nfnetlink_queue module\n" ANSI_COLOR_RESET);
            return -1;
        }
    }
    return 0;
}