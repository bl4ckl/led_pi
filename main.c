#include "main.h"
#include "tcpclient.h"
#include "udpclient.h"

#include "entity.h"
#include "spi.h"

#include "utility.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>

#include <signal.h>
#include <time.h>

//MAC ID
static unsigned char id[13];

//SPI
#define CHANNEL 0
#define SPI_SPEED 12000000
static int spifd;

//Tcp connection
static unsigned int server_ip;
static int tcpsockfd;
static char volatile buffer[1024];
static struct ledmessage volatile message;

//Timer
static timer_t volatile* it_id;
static struct itimerspec volatile* it_spec;
static long volatile current_frame;

//DATA
static entity volatile *_entity;

//Test
#define LEDS 77
static unsigned char volatile data[LEDS*4+6];

void TcpHandler(int signalType)
{
	memset((void*)&message, 0, sizeof(message));
	memset((void*)&buffer, 0, sizeof(buffer));

	//printf("receiving data\n");
	message.dataReceived = recv(tcpsockfd, (char*)buffer, 1024, 0);
	//printf("ending data\n");
	buffer[message.dataReceived] = '\0';
	//printf("get message id\n");
	message.messageID  = *buffer;
	message.dataLength = buffer[1];
	message.data = (char*)&buffer[3];

	printf("tcpclient received message with id %d\n", message.messageID);

	switch(message.messageID) {
		case MESSAGE_ID:
	        	char sendBuf[15];
        		memset(&sendBuf, 0, sizeof(sendBuf));
		       	uint16_t sizeOfBuf = 12;
		        memcpy(&sendBuf[1], &sizeOfBuf, sizeof(sizeOfBuf));
		        memcpy(&sendBuf[3], &id, sizeof(id)-1);
		        send(tcpsockfd, sendBuf, sizeof(sendBuf), 0);

			// TESTING PURPOSE
			/*
			char test[16];
			memcpy(&test[0], &sendBuf[0], 15);
			test[15] = '\0';
        		printf("Send ID: %s\n", &test[2]);
			printf("Bits: \n");
			print_bits(sizeof(sendBuf), &sendBuf[0]);
			*/
			break;
		case MESSAGE_CONFIG:
			if (entity_write_config((entity*)_entity, message.data)<0) {
				printf("entity_write_config() failed.");
			} else {
				printf("entity_write_config() succesfull.");
			}
			break;
		case MESSAGE_EFFECTS:
			if (entity_write_effects((entity*)_entity, message.data)<0) {
				printf("entity_write_effect() failed.");
			} else {
				printf("entity_write_effect() succesfull.");
			}
			break;
		case MESSAGE_TIME:
			break;
		case MESSAGE_PLAY:
			entity_play((entity*)_entity, (timer_t*)it_id, (struct itimerspec*)it_spec);
			break;
		case MESSAGE_PAUSE:
			entity_stop((timer_t*)it_id);
			unsigned char color[4];
			memset(&color[0], 0, 4);
			entity_full((entity*)_entity, color);
			break;
		case MESSAGE_PREVIEW:
			break;
		case MESSAGE_SHOW:
			unsigned char color[4];
			memset(&color[0], 0xFF, 4);
			entity_full((entity*)_entity, color);
			break;
		case MESSAGE_COLOR:
			break;
}

void fire_new_frame(long __current_frame, entity* __entity) {
//	for(int i = 0; i<__entity->num_bus; i++) {
//		spi_write(CHANNEL, __entity->bus[i].spi_write_out, __entity->bus[i].size_spi_write_out);
//	}

	memset((void*)&data[0], 0, sizeof(data));

	unsigned char a, b, g, r;
	a = 0xFF;
	b = 0xFF;
	g = 0xFF;
	r = 0xFF;

	for(int i = 0; i < LEDS; i++) {
		if(i == __current_frame%LEDS)
		{
			data[4+i*4] = a;
			data[5+i*4] = b;
			data[6+i*4] = g;
			data[7+i*4] = r;
		}
		else
		{
			data[4+i*4] = 0b11100000;
		}
	}

	printf("%d, %d\n", __current_frame, __current_frame%LEDS);

	spi_write(CHANNEL, (unsigned char*)&data[0], sizeof(data));
}

void playHandler(int signalType) {
	long temp = ++current_frame;
	fire_new_frame(temp, (entity*)_entity);
}

void intHandler(int signalType) {
	cleanup();
	exit(1);
}

void cleanup() {
//	printf("Freeing entity.\n");
	entity_free((entity*)_entity);
	free((void*)_entity);
//	printf("Freeing it_id.\n");
	free((void*)it_id);
//	printf("Freeing it_spec.\n");
	free((void*)it_spec);
}

int init() {
	//Setup ctrl+c handler
	struct sigaction act;
	memset(&act, 0, sizeof(act));

	act.sa_handler = intHandler;
	if (sigaction(SIGINT, &act, NULL)<0) {
		perror("init sigaction");
		return -1;
	}

	//Setup various datas
	_entity = malloc(sizeof(entity));
	//current_frame = malloc(sizeof(long));

	_entity->nsec=25000000;

	it_id = malloc(sizeof(timer_t));
	it_spec = malloc(sizeof(struct itimerspec));

	//Setup Timer
	entity_setup_timer((timer_t*)it_id);
	entity_setup_play_handler(playHandler);

	//Setup SPI
	spi_setup(&spifd, CHANNEL, SPI_SPEED);

	//At first get the ID
	get_mac("wlan0", id);
	printf("ID: %s.\n", id);

	//Test LED
//	ledtest();

	//Get the server ip
	udpclientstart(&server_ip, SERVER_PORT);

	//Connect to the server
	tcpcreatesocket(&tcpsockfd);
	tcpsetuphandler(tcpsockfd, TcpHandler);
	tcpclientstart(tcpsockfd, server_ip, SERVER_PORT);

	return 0;
}

int main() {
	init();

	//Wait for messages
	for(;;);

	cleanup();
}
