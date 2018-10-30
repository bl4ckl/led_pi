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

int tcpcreatesocket(int* __tcpsockfd)
{
    //Creating socket file descriptor
    int sockfd;
    if((sockfd  = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        return -1;
    }

    *__tcpsockfd = sockfd;
    return 0;
}

int tcpsetuphandler(int __tcpsockfd, void* __handler_function)
{
    struct sigaction handler;
    handler.sa_handler = __handler_function;

    if (sigfillset(&handler.sa_mask) < 0)
    {
	perror("sigfillset() failed.");
	return -1;
    }
    handler.sa_flags = 0;

    if (sigaction(SIGIO, &handler, 0) < 0)
    {
	perror("sigaction() failed.");
	return -1;
    }

    if (fcntl(__tcpsockfd, F_SETOWN, getpid()) < 0)
    {
	perror("fcntl() 1 failed.");
	return -1;
    }

    if (fcntl(__tcpsockfd, F_SETFL, O_NONBLOCK | FASYNC) <0)
    {
	perror("fcntl() 2 failed.");
	return -1;
    }

    return 0;
}

int tcpclientstart(int __tcpsockfd, int __server_ip, int __server_port) {
    struct sockaddr_in serverAddress;

    memset(&serverAddress, 0, sizeof(serverAddress));

    //Filling server information
    //IPv4
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = __server_ip;
    serverAddress.sin_port = htons(__server_port);

    if(connect(__tcpsockfd, (const struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("connect() failed.");
        //exit(EXIT_FAILURE);
    }

    return 0;
}

int tcpclientstop(int __tcpsockfd) {
    return close(__tcpsockfd);
}
