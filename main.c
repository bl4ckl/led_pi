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
static unsigned int server_ip;
static int tcpsockfd;

//Timer
static entity_timer_t timer_broadcast;
static entity_timer_t timer_heartbeat;
static entity_timer_t timer_play;
static entity_timer_t timer_show;

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

	} else if(*current_timer == timer_heartbeat.timer_id) {

	} else if(*current_timer == timer_show.timer_id) {
		entity_black();
	} else if(*current_timer == timer_broadcast.timer_id) {
		printf("Sending UDP broadcast.\n");
		udp_client_send_broadcast();
	}
}

int init_sigint() {
	//Setup ctrl+c handler
	struct sigaction act;
	memset(&act, 0, sizeof(act));

	act.sa_handler = intHandler;
	if (sigaction(SIGINT, &act, NULL)<0) {
		perror("init_sigin sigaction");
		return -1;
	}

	return 0;
}

int init() {
	int res = 0;
	init_sigint();

	//Setup entity
	entity = malloc(sizeof(entity_t));
	entity->nsec=25000000;

	//Setup Timers
	memset(&timer_broadcast, 0, sizeof(timer_broadcast));
	timer_broadcast.timer_spec.it_value.tv_sec = TIME_BROADCAST_SEC;
	timer_broadcast.timer_spec.it_interval = timer_broadcast.timer_spec.it_value;
	timers_init(&timer_broadcast, TIME_SIGNAL_UNIMPORTANT, timer_handler);

	memset(&timer_heartbeat, 0, sizeof(timer_heartbeat));
	timer_heartbeat.timer_spec.it_value.tv_sec = TIME_HEARTBEAT_NSEC;
	timer_heartbeat.timer_spec.it_interval = timer_heartbeat.timer_spec.it_value;
	timers_init(&timer_heartbeat, TIME_SIGNAL_HIGH , timer_handler);

	memset(&timer_show, 0, sizeof(timer_show));
	timer_show.timer_spec.it_value.tv_sec = TIME_SHOW_SEC;
	timers_init(&timer_show, TIME_SIGNAL_UNIMPORTANT, timer_handler);

	memset(&timer_play, 0, sizeof(timer_play));
	timers_init(&timer_play, TIME_SIGNAL_HIGHEST, timer_handler);


	//Setup SPI
	if(spi_setup(&spifd, CHANNEL, SPI_SPEED)<0) {
		perror("init spi_setup");
		res = -1;
	}

	//Create UDP Socket
	if(udp_client_init(SERVER_PORT)<0) {
		perror("init udp_client_init");
		res = -1;
	}

	//Create TCP Socket
	if(tcp_client_init(&tcpsockfd, tcp_handler_main)<0) {
		perror("init tcp_client_init");
		res = -1;
	}

	//Setup threading
	if(threads_init()<0) {
		perror("init threads_init");
		return -1;
	}

	return res;
}

void get_server_ip() {
	timers_start(&timer_broadcast);
	while(udp_client_receive(&server_ip)<0);
	timers_stop(&timer_broadcast);
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
	if(init()<0) {
		perror("main init");
		system("sleep 30s; shutdown -r now");
	}

	//At first get the mac id
	get_mac("wlan0", (unsigned char*)((entity_t*)entity)->id);
	printf("ID: %s.\n", ((entity_t*)entity)->id);

	//Get the server ip
	get_server_ip();

	//start tcp handling
	threads_start_tcp(tcpsockfd, (entity_t*)entity, NULL, NULL, NULL, entity_show);
	tcp_client_start(tcpsockfd, server_ip, SERVER_PORT);
	//timers_start(&timer_heartbeat);

	//Wait for messages
	for(;;);

	cleanup();
}
