#include "main.h"
#include "networking/tcpclient.h"
#include "networking/udpclient.h"

#include "entity/entity.h"
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

//Timer
static timer_t volatile* it_id;
static struct itimerspec volatile* it_spec;
static long volatile current_frame;

//DATA
static entity_t volatile* entity;

//Test
#define LEDS 77
static unsigned char volatile data[LEDS*4+6];

void fire_new_frame(long __current_frame, entity_t* __entity) {
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

	printf("%d, %d\n", (int)__current_frame, (int)__current_frame%LEDS);

	spi_write(CHANNEL, (unsigned char*)&data[0], sizeof(data));
}

void playHandler(int signalType) {
	long temp = ++current_frame;
	fire_new_frame(temp, (entity_t*)entity);
}

void intHandler(int signalType) {
	cleanup();
	exit(1);
}

void cleanup() {
//	printf("Freeing entity.\n");
	entity_free((entity_t*)entity);
	free((void*)entity);
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
	entity = malloc(sizeof(entity_t));
	//current_frame = malloc(sizeof(long));

	entity->nsec=25000000;

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
//	tcpcreatesocket(&tcpsockfd);
//	tcpsetuphandler(tcpsockfd, TcpHandler);
//	tcpclientstart(tcpsockfd, server_ip, SERVER_PORT);

	return 0;
}

int main() {
	init();

	//Wait for messages
	for(;;);

	cleanup();
}
