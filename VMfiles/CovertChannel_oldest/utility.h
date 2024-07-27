#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#define SERVER_IP "192.168.1.25"
#define SERVER_A_PORT 54321 
#define SERVER_B_PORT 54322
#define UDP_PORT 54323
#define MIN_SEND_COUNT 1
#define CHAR_SIZE_XOR_EXPAND 16  // 8(bits) * 2(expanding)
#define MODE_TIMED 0
#define MODE_STREAM 1

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define true 1
#define false 0

#define idDone(status, x, y) (status == HS_ACK && !(x - y) ? false : true)
int* ReadTextFile(int *size);
void CharToBinary(int iStart, int *message, char c);
void XorAndExpand(char c, char *left_res, char *right_res);
void MirroredXor(char c, char *left_res, char *right_res);
void PrintTxtInput(int *covert_message, int cm_size);
struct tcphdr* get_tcphdr_and_dport(struct nfq_data *nfa, int* dport);
int UpdateIptables();
int LoadNFLinkModules();