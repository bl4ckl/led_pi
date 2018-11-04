#include "main.h"
#include "networking/tcpclient.h"
#include "networking/udpclient.h"

#include "entity/entity.h"
#include "spi.h"

#include "utility.h"

#include "threads.h"

#include "timers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>

#include <signal.h>
#include <time.h>

void entity_black(void);

//SPI
#define CHANNEL 0
#define SPI_SPEED 12000000
static int spifd;

//DATA
static entity_t volatile* entity;
static int tcpsockfd;

//Tcp connection
static unsigned int server_ip;

//Timer
static entity_timer_t timer_play;
static entity_timer_t timer_show;

//Test
#define LEDS 77
static unsigned char volatile data[LEDS*4+6];

void intHandler(int signalType) {
	cleanup();
	exit(1);
}

void cleanup() {
	entity_black();
//	printf("Freeing entity.\n");
	entity_free((entity_t*)entity);
	free((void*)entity);
}

void tcp_handler_main(int dummy) {
	threads_execute_tcp();
}

void timer_handler(int __signal, siginfo_t* __sig_info, void* __dummy) {
	timer_t* current_timer = __sig_info->si_value.sival_ptr;

	if(*current_timer == timer_play.timer_id) {

	} else if(*current_timer == timer_show.timer_id) {
		entity_black();
	}
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

	//Setup Timers
	memset(&timer_show, 0, sizeof(timer_show));
	timer_show.timer_spec.it_value.tv_sec = 2;
	timers_init(&timer_show, timer_handler);

	//Setup SPI
	if(spi_setup(&spifd, CHANNEL, SPI_SPEED)<0)
		printf("spi_setup failed");

	//At first get the ID
	get_mac("wlan0", (unsigned char*)((entity_t*)entity)->id);
	printf("ID: %s.\n", ((entity_t*)entity)->id);

	//Test LED
//	ledtest();

	//Get the server ip
	udpclientstart(&server_ip, SERVER_PORT);

	//Connect to the server
	tcpcreatesocket(&tcpsockfd);
	tcpsetuphandler(tcpsockfd, tcp_handler_main);

	return 0;
}

void entity_show(void) {
	 unsigned char color[4];
         memset(&color[0], 0xFF, 4);
         entity_full(NULL, color);
	timers_start(&timer_show);
}

void entity_black(void) {
	 unsigned char color[4];
         memset(&color[0], 0, 4);
         entity_full(NULL, color);
}

int main() {
	init();

	//start tcp handling
	threads_init();
	threads_start_tcp(tcpsockfd, (entity_t*)entity, NULL, NULL, NULL, entity_show);
	tcpclientstart(tcpsockfd, server_ip, SERVER_PORT);
	//threads_exit_tcp();

	//Wait for messages
	for(;;);

	cleanup();
}
