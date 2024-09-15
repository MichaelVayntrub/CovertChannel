#include <sys/socket.h>
#include "monitor.h"

pthread_mutex_t mutex_channel[2];
pthread_mutex_t mutex_channel0 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_channel1 = PTHREAD_MUTEX_INITIALIZER;
static int rest;

//test//
pthread_mutex_t mutex_http = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_http = PTHREAD_COND_INITIALIZER;
int turn = 0; // 0 for thread 0, 1 for thread 1


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

Thread_Data* Create_Data(int id, char* ip, int port, int send_count, int* cond_channels_signal, pthread_cond_t* cond_channels)
{
    Thread_Data* data = (Thread_Data*)malloc(sizeof(Thread_Data));
    data->id = id;
    data->ip = ip;
    data->port = port;
    data->send_count = send_count;
    data->cond_channels_signal = cond_channels_signal;
    data->cond_channels = cond_channels;
    return data;
}

/*
void* RunTestHTTP_Channel(void* arg) 
{
    Thread_Data* data = (Thread_Data*)arg;
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int sending = true;
    int index = 0;
    char* ip = "10.0.2.2";

    while (sending)
    {   
        //test//
        //pthread_mutex_lock(&mutex_http);
        //while (turn != data->id) {
        //    pthread_cond_wait(&cond_http, &mutex_http);
        //}
        //sleep(2); //temp
        //if (data->id == 1) sleep(2); //temp
        char* page = data->id == 0  ? "/a" : "/b";

        // Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("ERROR opening socket");
            exit(EXIT_FAILURE);
        }
        
        // Set server address
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_HTTP_PORT);
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
            perror("ERROR invalid IP address");
            exit(EXIT_FAILURE);
        }

        // Connect to server
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR connecting");
            exit(EXIT_FAILURE);
        }
        if (index == 0) {
            printf(ANSI_COLOR_CYAN "Connected to the HTTP server %s on port %d\n" ANSI_COLOR_RESET, ip, SERVER_HTTP_PORT);
        }

        // Formulate HTTP GET request
        //char* page = (index + 1) % 2 ? "/a" : "/b";
        char* page = data->id == 0  ? "/a" : "/b";
        char request[1024];
        snprintf(request, sizeof(request),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n", page, ip);

        // Send HTTP request
        if (write(sockfd, request, strlen(request)) < 0) {
            perror("ERROR writing to socket");
            exit(EXIT_FAILURE);
        }
        printf(ANSI_COLOR_YELLOW "Request sent for: " ANSI_COLOR_RESET);
        printf("'%s'\n", page);

        // Receive HTTP response
        char response[4096];
        int bytes_received = read(sockfd, response, sizeof(response) - 1);
        if (bytes_received < 0) {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        response[bytes_received] = '\0';
        //printf(ANSI_COLOR_YELLOW "Received response for: " ANSI_COLOR_RESET);
        //printf("'%s'\n", page);

        close(sockfd);
        //test//
        //turn = 1 - turn;
        //pthread_cond_signal(&cond_http);
        //pthread_mutex_unlock(&mutex_http);

        sleep(2); //temp
        index++;
    }

    // Close the socket
    printf(ANSI_COLOR_CYAN  "HTTP Channel closed\n" ANSI_COLOR_RESET);
}
*/

void* RunTestHTTP_Channel(void* arg) 
{
    Thread_Data* data = (Thread_Data*)arg;
    int sockfd;
    struct sockaddr_in serv_addr, src_addr;
    int sending = true;
    int index = 0;
    char* ip = "10.0.2.2";
    int src_port = data->id == 0  ? 50000 : 50001;
    int find_port;
    
    while (sending)
    {   
        find_port = true;
        while (find_port)
        {   
            // Create socket
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                perror("ERROR opening socket");
                exit(EXIT_FAILURE);
            }

            // Set SO_LINGER option immediately after creating the socket
            //struct linger sl;
            //sl.l_onoff = 1;   // Enable linger option
            //sl.l_linger = 1;  // Close socket immediately
            //setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));

            int opt = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

            // Bind socket to a specific source port
            memset(&src_addr, 0, sizeof(src_addr));
            src_addr.sin_family = AF_INET;
            src_addr.sin_addr.s_addr = INADDR_ANY; // Use any available interface
            src_addr.sin_port = htons(src_port);

            if (bind(sockfd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
                perror("ERROR binding socket to source port");
                close(sockfd);
                exit(EXIT_FAILURE);
            } 
        
            // Set server address
            memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(SERVER_HTTP_PORT);
            if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
                perror("ERROR invalid IP address");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            // Connect to server
            if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("ERROR connecting");
                close(sockfd);
                src_port+=100;
                continue;
            } else {
                find_port = false;
            }
            if (index == 0) {
                printf(ANSI_COLOR_CYAN "Connected to the HTTP server %s on port %d (Source Port: %d)\n" ANSI_COLOR_RESET, ip, SERVER_HTTP_PORT, src_port);
            }
        }

        // Formulate HTTP GET request
        char* page = data->id == 0  ? "/a" : "/b";
        char request[1024];
        snprintf(request, sizeof(request),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n", page, ip);

        // Send HTTP request
        if (write(sockfd, request, strlen(request)) < 0) {
            perror("ERROR writing to socket");
            exit(EXIT_FAILURE);
        }
        printf(ANSI_COLOR_YELLOW "Request sent for: " ANSI_COLOR_RESET "'%s' from source port: %d\n", page, src_port);

        // Receive HTTP response
        char response[4096];
        int bytes_received = read(sockfd, response, sizeof(response) - 1);
        if (bytes_received < 0) {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        response[bytes_received] = '\0';
        
        printf(ANSI_COLOR_YELLOW "Received response for: " ANSI_COLOR_RESET "'%s' on source port: %d\n", page, src_port);

        close(sockfd);
        sleep(1);
        if (index++ > data->send_count) sending = false;
        src_port+=2;
    }

    printf(ANSI_COLOR_CYAN "HTTP Channel closed (Source Port: %d)\n" ANSI_COLOR_RESET, src_port);
    return 0;
}

void* RunHTTP_Channel(void* arg) {
    Thread_Data* data = (Thread_Data*)arg;
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int sending = true;
    int index = 0;
    int dport = data->id == 0 ? SERVER_HTTPA_PORT : SERVER_HTTPB_PORT;

    while (sending)
    {
        // Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("ERROR opening socket");
            exit(EXIT_FAILURE);
        }
        
        // Set socket options to reuse the address
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Set server address
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(dport);
        //if (inet_pton(AF_INET, data->ip, &serv_addr.sin_addr) <= 0) {
        //    perror("ERROR invalid IP address");
        //    exit(EXIT_FAILURE);
        //}

        // Bind the socket to the local address
        if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Bind failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        

        struct sockaddr_in remote_addr;
        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(SERVER_HTTP_PORT); // Remote server port
        if (inet_pton(AF_INET, LOCAL_HOST, &remote_addr.sin_addr) <= 0) { // Remote server IP
            perror("Invalid address");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Connect to server
        if (connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
            perror("ERROR connecting");
            exit(EXIT_FAILURE);
        }
        if (index == 0) {
            printf(ANSI_COLOR_CYAN "Connected to the HTTP server %s on port %d\n" ANSI_COLOR_RESET, data->ip, SERVER_HTTP_PORT);
        }

        // Formulate HTTP GET request
        char* page = data->id == 0  ? "/a" : "/b";
        char request[1024];
        snprintf(request, sizeof(request),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n", page, data->ip);

        // Send HTTP request
        if (write(sockfd, request, strlen(request)) < 0) {
            perror("ERROR writing to socket");
            exit(EXIT_FAILURE);
        }
        printf(ANSI_COLOR_YELLOW "Request sent for: " ANSI_COLOR_RESET);
        printf("'%s'\n", page);

        // Receive HTTP response
        char response[4096];
        int bytes_received = read(sockfd, response, sizeof(response) - 1);
        if (bytes_received < 0) {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        response[bytes_received] = '\0';
        printf(ANSI_COLOR_YELLOW "Received response for: " ANSI_COLOR_RESET);
        printf("'%s'\n", page);

        sleep(1); //temp
        index++;
        close(sockfd);
    }
    // Close the socket
    close(sockfd);
    printf(ANSI_COLOR_CYAN  "HTTP Channel closed\n" ANSI_COLOR_RESET);
}

void* RunTCP_Channel(void* arg) {
    Thread_Data* data = (Thread_Data*)arg;
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
    printf(ANSI_COLOR_CYAN "Connected to the TCP server %s on port %d\n" ANSI_COLOR_RESET, data->ip, data->port);

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
    // Close the socket
    close(sockfd);
    printf(ANSI_COLOR_CYAN  "TCP Channel No.%d closed\n" ANSI_COLOR_RESET, data->id);
    return 0;
}

void WaitForStart()
{
    int BUFFER_SIZE = 1024, sockfd, flag_listen = true;
    char buffer[BUFFER_SIZE], response[BUFFER_SIZE];
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

    while (flag_listen) {
        client_len = sizeof(client_addr);
        recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }
        buffer[recv_len] = '\0'; 

        if (strcmp("GO", buffer) == 0) {
            flag_listen = false;
        }
    }
    close(sockfd);
}

int Establish_connection(Conf_Data* data, char* ip)
{
    if (Connect_To_Host(data)) {
        Send_UDP_Response(ip, "OK"); 
        sleep(2);
        printf(ANSI_COLOR_CYAN "Connected to the host on %s\n" ANSI_COLOR_RESET, data->ip);
        return true;
    }
    Send_UDP_Response(ip, "FAIL");
    sleep(2);
    printf(ANSI_COLOR_RED "Couldn't connect to the host\n" ANSI_COLOR_RESET);
    return false;       
}

int Connect_To_Host(Conf_Data* data)
{
    int BUFFER_SIZE = 1024, sockfd, flag_listen = true;
    char *host_ip, buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    ssize_t recv_len;
    int items_req = 4, items_count = 0;


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

    printf(ANSI_COLOR_YELLOW "Waiting for the host to connect...\n" ANSI_COLOR_RESET);

    while (flag_listen) {
        
        client_len = sizeof(client_addr);
        recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        char* temp = (char*)malloc(sizeof(char) * (int)recv_len + 1);
        sprintf(temp, "%.*s\n", (int)recv_len, buffer);

        char *token = strtok(temp, ";");
        while (token != NULL) {
            if (items_count >= items_req) return false;
            if (items_count == 0) {
                data->ip = (char*)malloc(strlen(token) + 1);
                strcpy(data->ip, token);
            } else if (items_count == 1){
                data->proto = (char*)malloc(strlen(token) + 1);
                strcpy(data->proto, token);
            } else if (items_count == 2){
                data->file = (char*)malloc(strlen(token) + 1);
                strcpy(data->file, token);
            } else if (items_count == 3){
                data->crypt = (char*)malloc(strlen(token) + 1);
                strcpy(data->crypt, token);
            }
            token = strtok(NULL, ";");
            items_count++;
        }

        flag_listen = false;
        free(temp);
    }

    close(sockfd);
    return items_count == items_req;
}

void Send_UDP_Response(char* ip, char* message)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = INADDR_ANY;
    dest_addr.sin_port = htons(UDP_RES_PORT);
    inet_pton(AF_INET, ip, &dest_addr.sin_addr);

    ssize_t sent_len = sendto(sockfd, message, strlen(message), 0,
                             (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (sent_len < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);
}

void Free_Conf_Data(Conf_Data* data) 
{
    free(data->ip);
    free(data->crypt);
    free(data->file);
    free(data->proto);
}