#include "main.h"
#include "tcpclient.h"
#include "udpclient.h"

#include "led.h"
#include "spi.h"

#include "utility.h"


#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>

//MAC ID
unsigned char id[13];

//SPI
#define CHANNEL 0
#define SPI_SPEED 12000000
int spifd;

//Tcp connection
unsigned int server_ip;
int tcpsockfd;

struct ledmessage {
        uint8_t         messageID;
        uint32_t        dataLength;
        uint32_t        dataReceived;
        char *          data;
};

void TcpHandler(int signalType)
{
    struct ledmessage message;
    memset(&message, 0, sizeof(message));

    char buffer[1024];
    //printf("receiving data\n");
    message.dataReceived = recv(tcpsockfd, (char *)buffer, 1024, 0);
    //printf("ending data\n");
    buffer[message.dataReceived] = '\0';
    //printf("get message id\n");
    message.messageID  = *buffer;
    message.dataLength = buffer[1];
    message.data = &buffer[3];

    printf("tcpclient received message with id %d\n", message.messageID);

    if(message.messageID == MESSAGE_ID) {
        char sendBuf[15];
        memset(&sendBuf, 0, sizeof(sendBuf));
        uint16_t sizeOfBuf = 12;
        memcpy(&sendBuf[1], &sizeOfBuf, sizeof(sizeOfBuf));
        memcpy(&sendBuf[3], &id, sizeof(id)-1);
        send(tcpsockfd, sendBuf, sizeof(sendBuf), 0);

        //printf("Send ID.\n");
    }
    else if(message.messageID == MESSAGE_CONFIG) {

    }
    else if(message.messageID == MESSAGE_EFFECTS) {

    }
    else if(message.messageID == MESSAGE_TIME) {

    }
    else if(message.messageID == MESSAGE_PLAY) {

    }
    else if(message.messageID == MESSAGE_PAUSE) {

    }
    else if(message.messageID == MESSAGE_PREVIEW) {

    }
}

void ledtest() {
	//Setup SPI
	spi_setup(&spifd, CHANNEL, SPI_SPEED);

	//Test leds
	unsigned char data[6+4*72];
	memset(&data[0], 0, 4);
//	memset(&data[sizeof(data)-5], 255, 4);

	unsigned char a, b, g, r;
	a = 0xFF;
	b = 0x00;
	g = 0x78;
	r = 0xFF;

	for(int i = 0; i < 72; i++) {
		data[4+i*4] = a;
		data[5+i*4] = b;
		data[6+i*4] = g;
		data[7+i*4] = r;
	}

	spi_write(CHANNEL, &data[0], sizeof(data));
}

int main() {
	//At first get the ID
	get_mac("wlan0", id);
	printf("ID: %s.\n", id);

	//Test LED
	ledtest();

	//Get the server ip
	udpclientstart(&server_ip, SERVER_PORT);

	//Connect to the server
	tcpcreatesocket(&tcpsockfd);
	tcpsetuphandler(tcpsockfd, TcpHandler);
	tcpclientstart(tcpsockfd, server_ip, SERVER_PORT);

	//Wait for messages
	for(;;);
}
