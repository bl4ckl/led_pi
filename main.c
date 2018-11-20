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
#include <unistd.h>

#include <signal.h>
#include <time.h>

void entity_black(void);
void entity_show(void);
void entity_play(void);
void entity_pause(void);
void get_server_ip();
int init_networking();

//SPI
#define CHANNEL 0
#define SPI_SPEED 12000000
static int spifd;

//DATA
static entity_t volatile* entity;
static unsigned int server_ip;
static int tcpsockfd;
static pthread_mutex_t current_frame_mutex;
static uint32_t current_frame = 0;
static bool playing = false;

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
	threads_exit_entity();

	threads_exit_tcp();

	entity_black();
	entity_free((entity_t*)entity);

	free((void*)entity);
}

void timer_handler(int __signal, siginfo_t* __sig_info, void* __dummy) {
	timer_t* current_timer = __sig_info->si_value.sival_ptr;

	if(*current_timer == timer_play.timer_id) {
//		printf("Trying to update frame\n");
		pthread_mutex_lock(&current_frame_mutex);
//		printf("\tinside lock\n");
		current_frame++;
		threads_execute_entity(current_frame);
		pthread_mutex_unlock(&current_frame_mutex);
//		printf("\toutside lock\n");
	} else if(*current_timer == timer_heartbeat.timer_id) {
		if(threads_execute_tcp(true)<0) {
			timers_stop(&timer_heartbeat);
			close(tcpsockfd);

			//Create TCP Socket
			if(tcp_client_init(&tcpsockfd)<0) {
				perror("init tcp_client_init");
			} else {
				timers_start(&timer_heartbeat);
			}
		}
	} else if(*current_timer == timer_show.timer_id) {
		if(!playing) {
			entity_black();
		}
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
	memset((entity_t*)entity, 0, sizeof(entity_t));
	entity->nsec=25000000;

	//Setup Timers
	memset(&timer_broadcast, 0, sizeof(timer_broadcast));
	timer_broadcast.timer_spec.it_value.tv_nsec = 1;
	timer_broadcast.timer_spec.it_interval.tv_sec = TIME_BROADCAST_SEC;
	timers_init(&timer_broadcast, TIME_SIGNAL_UNIMPORTANT, timer_handler);

	memset(&timer_heartbeat, 0, sizeof(timer_heartbeat));
	timer_heartbeat.timer_spec.it_value.tv_nsec = TIME_HEARTBEAT_NSEC;
	timer_heartbeat.timer_spec.it_interval = timer_heartbeat.timer_spec.it_value;
	timers_init(&timer_heartbeat, TIME_SIGNAL_HIGH , timer_handler);

	memset(&timer_show, 0, sizeof(timer_show));
	timer_show.timer_spec.it_value.tv_sec = TIME_SHOW_SEC;
	timers_init(&timer_show, TIME_SIGNAL_UNIMPORTANT, timer_handler);

	memset(&timer_play, 0, sizeof(timer_play));
	timers_init(&timer_play, TIME_SIGNAL_REALTIME, timer_handler);


	//Setup SPI
	if(spi_setup(&spifd, CHANNEL, SPI_SPEED)<0) {
		perror("init spi_setup");
		res = -1;
	}

	//Setup threading
	if(threads_init()<0) {
		perror("init threads_init");
		return -1;
	}

	return res;
}

int init_networking() {
	//Create UDP Socket
	if(udp_client_init(SERVER_PORT)<0) {
		perror("init udp_client_init");
		return -1;
	}

	//Create TCP Socket
	if(tcp_client_init(&tcpsockfd)<0) {
		perror("init tcp_client_init");
		return -1;
	}

	return 0;
}

void get_server_ip() {
	timers_start(&timer_broadcast);
	while(udp_client_receive(&server_ip)<0);
	timers_stop(&timer_broadcast);
}

void entity_timestamp(uint32_t __frame) {
	pthread_mutex_lock(&current_frame_mutex);
	current_frame = __frame;
	threads_execute_entity(current_frame);
	pthread_mutex_unlock(&current_frame_mutex);
}

void entity_config_effects_pre(void) {
	if(playing) {
		entity_pause();
	}
	threads_exit_entity();
}

void entity_effects_post(void) {
	threads_start_entity((entity_t*)entity);
}

void entity_play(void) {
	if(!playing) {
		timer_play.timer_spec.it_value.tv_nsec = entity->nsec;
		timer_play.timer_spec.it_interval = timer_play.timer_spec.it_value;
		if(timers_start(&timer_play)>0) {
			playing = true;
		}
	} else {
		perror("main entity_play already playing");
	}
}


void entity_pause(void) {
	timers_stop(&timer_play);
	playing = false;
}

void entity_show(void) {
	if(!playing) {
		unsigned char color[4];
	        memset(&color[0], 0xFF, 4);
       		entity_full(NULL, &color[0]);

		timers_start(&timer_show);
	} else {
		perror("main entity_show show while playing");
	}
}

void entity_color(uint8_t* __color) {
	entity_full(NULL, &__color[0]);
}

void entity_black(void) {
	if(!playing) {
		unsigned char color[4];
        	memset(&color[0], 0, 4);
        	entity_full(NULL, &color[0]);
	} else {
		perror("main entity_black black while playing");
	}
}

int main() {
	if(init()<0) {
		perror("main init");
		printf("Initiating reboot in 30 seconds.");
		fflush(stdout);
		system("sleep 30s; shutdown -r");
	}

	while (init_networking()<0);

	//At first get the mac id
	get_mac("wlan0", (unsigned char*)((entity_t*)entity)->id);
	printf("ID: %s.\n", ((entity_t*)entity)->id);

	//Get the server ip
	get_server_ip();

	//start tcp handling
	threads_start_tcp(tcpsockfd, (entity_t*)entity, entity_timestamp, entity_config_effects_pre, entity_effects_post, entity_play, entity_pause, entity_show, entity_color);
	tcp_client_start(tcpsockfd, server_ip, SERVER_PORT);
//	timers_start(&timer_heartbeat);

	//Wait for messages
	for(;;) {
		sleep(1);
	}

	cleanup();
}
