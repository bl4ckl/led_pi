/*  INCLUDES MAIN */
#include "udpclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

static int sockfd;
static struct sockaddr_in broadcastAddress, serverAddress;
static char buffer[1024];
static char *broadcastMessage = "I bims, eins LED!";
static char desiredAnswer[] = "I bims, eins PC!";
static bool init = false;

unsigned int ipv4AddressToInt(char* __ip_address)
{
    int a, b, c, d;
    char arr[4];

    sscanf(__ip_address, "%d.%d.%d.%d", &a, &b, &c, &d);
    arr[0] = a;
    arr[1] = b;
    arr[2] = c;
    arr[3] = d;

    return *(unsigned int*)arr;
}

int udp_client_init(int __server_port) {
	if(init) {
		close(sockfd);
	}

	//Creating socket file descriptor
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("udp_client_init socket");
		return -1;
	}

    	//give socket broadcast permissions
	int broadcastEnable = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

	memset(&broadcastAddress, 0, sizeof(broadcastAddress));

    	//Filling server information
    	broadcastAddress.sin_family = AF_INET;
    	broadcastAddress.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	broadcastAddress.sin_port = htons(__server_port);

	init = true;
	return 0;
}

int udp_client_send_broadcast(void) {
	if(!init) {
		perror("udp_client_receive init false");
		return -1;
	}

	if(sendto(sockfd, (const char *)broadcastMessage, strlen(broadcastMessage), MSG_DONTWAIT, (const struct sockaddr *)&broadcastAddress, sizeof(broadcastAddress))<0) {
		perror("udp_client_send_brodacast sendto");
		return -1;
	}

	return 0;
}

int udp_client_receive(unsigned int* __server_ip) {
	if(!init) {
		perror("udp_client_receive init false");
		return -1;
	}

	memset(&serverAddress, 0, sizeof(serverAddress));
	socklen_t serverAddressLength = sizeof(serverAddressLength);

	if(usleep(250000)<0) {
		perror("udp_client_receive usleep");
		return -1;
	}
	int message = 0;
        if((message = recvfrom(sockfd, (char *)buffer, 1024, MSG_DONTWAIT, (struct sockaddr*)&serverAddress, &serverAddressLength))<0) {
		perror("udp_client_receive recvfrom");
		return -1;
	} else {
	        buffer[message] = '\0';
	        if(message >= sizeof(desiredAnswer)) {
			int length = strlen(desiredAnswer);
		        char bufferAnswer[length+1];
        		memcpy((void*)&bufferAnswer[0], (void*)&buffer[0], length);
			bufferAnswer[length]='\0';
			printf("stripped answer: %s\n", bufferAnswer);

            		if(strcmp(desiredAnswer, bufferAnswer) == 0) {
                		char ip[message-(sizeof(bufferAnswer)-1)];
		                memcpy((void*)&ip, (void*)&buffer[sizeof(desiredAnswer)-1], message - (sizeof(bufferAnswer)-1));
                		ip[sizeof(ip)] = '\0';
		                printf("server found at %s.\n", &ip[0]);
			   	*__server_ip = ipv4AddressToInt((char *)&ip);
                		return 0;
		        } else {
				perror("udp_client_receive strcmp");
				return -1;
			}
        	} else {
			perror("udp_client_receive message to small");
			return -1;
		}
	}

	close(sockfd);
	init = false;

	return 0;
}
