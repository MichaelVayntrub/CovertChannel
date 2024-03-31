#include <sys/socket.h>
#include "monitor.h"

pthread_mutex_t mutex_channel[2];
pthread_mutex_t mutex_channel0 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_channel1 = PTHREAD_MUTEX_INITIALIZER;
static int rest;

void InitChannelsMutexes() 
{
    mutex_channel[0] = mutex_channel0;
    mutex_channel[1] = mutex_channel1;
}

void DestroyChannelsMutexes() 
{
    pthread_mutex_destroy(&mutex_channel0);
    pthread_mutex_destroy(&mutex_channel1);
}

ThreadTCP_Data* CreateTCP_Data(int id, char* ip, int port, int send_count, int* cond_channels_signal, pthread_cond_t* cond_channels)
{
    ThreadTCP_Data* data = (ThreadTCP_Data*)malloc(sizeof(ThreadTCP_Data));
    data->id = id;
    data->ip = ip;
    data->port = port;
    data->send_count = send_count;
    data->cond_channels_signal = cond_channels_signal;
    data->cond_channels = cond_channels;
    return data;
}

void* RunTCP_Channel(void* arg) {
    ThreadTCP_Data* data = (ThreadTCP_Data*)arg;
    int running = true;
    int sockfd;
    struct sockaddr_in server_addr;
    int index = 0;
    char msg[30];

    int* cond_channels_signal = data->cond_channels_signal;
    pthread_cond_t* cond_channels = data->cond_channels;


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    server_addr.sin_addr.s_addr = inet_addr(data->ip);

    while (*cond_channels_signal != data->id) {
        pthread_cond_wait(cond_channels, &(mutex_channel[data->id]));
    }
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    printf(ANSI_COLOR_CYAN "Connected to the server %s on port %d\n" ANSI_COLOR_RESET, data->ip, data->port);

    int other_channel = 1 - data->id;
    
    while (running) {
        
        while (*cond_channels_signal != data->id) {
            pthread_cond_wait(cond_channels, &(mutex_channel[data->id]));
        }

        sprintf(msg, "channel-No.%d[%d]", data->id, index++);

        if (send(sockfd, msg, strlen(msg), 0) < 0) {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }
        printf(ANSI_COLOR_YELLOW "Message sent: " ANSI_COLOR_RESET);
        printf("%s\n", msg);
        
        if (index > data->send_count) running = false;
        *cond_channels_signal = -1;
    }
    
    close(sockfd);
    printf(ANSI_COLOR_CYAN  "TCP Channel No.%d closed\n" ANSI_COLOR_RESET, data->id);
    return 0;
}

char* WaitForHostIp()
{
    int BUFFER_SIZE = 1024, sockfd, flag_listen = true;
    char *host_ip, buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    ssize_t recv_len;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(UDP_PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf(ANSI_COLOR_YELLOW "Waiting for host's server ip...\n" ANSI_COLOR_RESET);

    while (flag_listen) {
        
        client_len = sizeof(client_addr);
        recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        host_ip = (char*)malloc(sizeof(char) * (int)recv_len + 1);
        sprintf(host_ip, "%.*s\n", (int)recv_len, buffer);
        flag_listen = false;
    }
    return host_ip;
}
