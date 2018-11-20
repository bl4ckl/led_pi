/*  INCLUDES MAIN */
#include "tcpclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>
#include <net/if.h>

int tcp_client_init(int* __tcpsockfd) {
    //Creating socket file descriptor
    int sockfd;
    if((sockfd  = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("tcp_client_init socket");
        return -1;
    }

    *__tcpsockfd = sockfd;
    return 0;
}

int tcp_client_start(int __tcpsockfd, int __server_ip, int __server_port) {
    struct sockaddr_in serverAddress;

    memset(&serverAddress, 0, sizeof(serverAddress));

    //Filling server information
    //IPv4
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = __server_ip;
    serverAddress.sin_port = htons(__server_port);

    if(connect(__tcpsockfd, (const struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("tcp_client_start connect");
        return -1;
    }

    return 0;
}

int tcp_client_stop(int __tcpsockfd) {
    return close(__tcpsockfd);
}
