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

int udpclientstart(unsigned int* __server_ip, int __server_port) {
    int socketFileDescriptor;
    char buffer[1024];
    char *broadcastMessage = "I bims, eins LED!";
    char desiredAnswer[] = "I bims, eins PC!";
    struct sockaddr_in broadcastAddress, serverAddress;

    //Creating socket file descriptor
    if((socketFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&broadcastAddress, 0, sizeof(broadcastAddress));
    memset(&serverAddress, 0, sizeof(serverAddress));

    //Filling server information
    broadcastAddress.sin_family = AF_INET;
    broadcastAddress.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    broadcastAddress.sin_port = htons(__server_port);

    //give socket broadcast permissions
    int broadcastEnable = 1;
    setsockopt(socketFileDescriptor, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

	while(1)
	{
    		sendto(socketFileDescriptor, (const char *)broadcastMessage, strlen(broadcastMessage), MSG_CONFIRM, (const struct sockaddr *)&broadcastAddress, sizeof(broadcastAddress));
		printf("broadcast sent\n");

	        int message;
        	socklen_t serverAddressLength = sizeof(serverAddressLength);
		printf("waiting for answer\n");
	        message = recvfrom(socketFileDescriptor, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr*)&serverAddress, &serverAddressLength);
	        buffer[message] = '\0';
		printf("answer received: %s\n", buffer);
	        if(message >= sizeof(desiredAnswer))
	        {
			int length = strlen(desiredAnswer);
		        char bufferAnswer[length+1];
        		memcpy((void*)&bufferAnswer[0], (void*)&buffer[0], length);
			bufferAnswer[length]='\0';
			printf("stripped answer: %s\n", bufferAnswer);

            		if(strcmp(desiredAnswer, bufferAnswer) == 0)
            		{
                		char ip[message-(sizeof(bufferAnswer)-1)];
		                memcpy((void*)&ip, (void*)&buffer[sizeof(desiredAnswer)-1], message - (sizeof(bufferAnswer)-1));
                		ip[sizeof(ip)] = '\0';
		                printf("server found at %s.\n", &ip[0]);
			   	*__server_ip = ipv4AddressToInt((char *)&ip);
                		break;
		        }
        	}
    	}

    	close(socketFileDescriptor);
    	return 0;
}
