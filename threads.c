#include "threads.h"
#include "entity/entity_handler.h"
#include "entity/entity.h"
#include "networking/tcp_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

static bool init = false;
static bool entity_threads_started = false;
static bool tcp_thread_started = false;

static int num_entity_threads = 0;

static pthread_t* entity_threads;
static pthread_t tcp_thread;

static entity_handler_args_t* entity_handler_args;
static tcp_handler_args_t tcp_handler_args;

int threads_init() {

	init = true;
	return 0;
}

int threads_execute_entity(long __current_frame) {
	int res = 0;
	bool threads_to_restart[num_entity_threads];
	memset(&threads_to_restart[0], 0, sizeof(threads_to_restart));

	for(int i=0; i<num_entity_threads; i++) {
		if(pthread_mutex_lock(&(entity_handler_args[i].mutex))<0) {
			perror("threads_execute_entity pthread_mutex_lock");
			res = -1;
		}

		//Check if the previous call to this function caused a thread to exit
		if(!(entity_handler_args[i].frame_update_completed)) {
			perror("threads_execute_entity frame_update_completed false");
			threads_to_restart[i] = true;
		} else {
			entity_handler_args[i].current_frame = __current_frame;
			entity_handler_args[i].frame_update_completed = false;
		}

		if(pthread_mutex_unlock(&(entity_handler_args[i].mutex))<0) {
			perror("threads_execute_entity pthread_mutex_unlock");
			res = -1;
		}

		if(sem_post(&(entity_handler_args[i].entity_sem))<0) {
			perror("threads_execute_entity sem_post");
			res = -1;
		}
	}

	/*
	for(int i=0; i<num_entity_threads; i++) {
		if(threads_to_restart[i]) {
			threads_restart_entity(i);
		}
	}
	*/

	return res;
}

int threads_start_entity(entity_t* __entity) {
	//Some checks to ensure we can proceed
	if(!init) {
		perror("threads_start_entity init false");
		return -1;
	}
	if(entity_threads_started) {
		perror("threads_start_entity entity_threads_started true");
		return -1;
	}

	//Setting default values
	num_entity_threads = __entity->num_bus;

	//Acquire heap space
	if((entity_handler_args=(entity_handler_args_t*)malloc(sizeof(entity_handler_args_t)*num_entity_threads))<0) {
		perror("threads_start_entity entity_handler_args malloc");
		return -1;
	}
	if((entity_threads=(pthread_t*)malloc(sizeof(pthread_t)*num_entity_threads))<0) {
		perror("threads_start_entity entity_threads malloc");
		return -1;
	}

	//Initialize args and start threads
	for(int i=0; i<num_entity_threads; i++) {
		if(entity_handler_args_init(&entity_handler_args[i], &(__entity->bus[i]), __entity->fps)<0) {
			perror("threads_start_entity entity_handler_args_init");
			return -1;
		}
		if(pthread_create(&entity_threads[i], NULL, entity_handler, (void*)&entity_handler_args[i])<0) {
			perror("threads_start_entity pthread_create");
			return -1;
		}
	}

	entity_threads_started = true;
	return 0;
}

int threads_exit_entity() {
	int res = 0;
	if(!entity_threads_started) {
		perror("threads_exit_entity entity_threads_started false");
		return -1;
	}

	//Tell all threads to exit
	for(int i=0; i<num_entity_threads; i++) {
		if(pthread_mutex_lock(&(entity_handler_args[i].mutex))<0) {
			perror("threads_exit_entity pthread_mutex_lock");
			res = -1;
		}
		entity_handler_args[i].thread_exit_issued = true;
		if(pthread_mutex_unlock(&(entity_handler_args[i].mutex))<0) {
			perror("threads_exit_entity pthread_mutex_unlock");
			res = -1;
		}
	}

	//Execute all threads so they can exit
	threads_execute_entity(0);

	//Wait for all threads to exit
	for(int i=0; i<num_entity_threads; i++) {
		if(pthread_join(entity_threads[i], NULL)<0) {
			perror("threads_exit_entity pthread_join");
			res = -1;
		}
	}

	//Clear all remaining stuff
	threads_cleanup_entity();
	return res;
}

int threads_cleanup_entity() {
	free((void*)entity_threads);

	for(int i=0; i<num_entity_threads; i++) {
		if(entity_handler_args_destroy(&entity_handler_args[i])<0) {
			perror("threads_cleanup_entity entity_handler_args_destroy");
		}
	}
	free((void*)entity_handler_args);

	entity_threads_started = false;
	num_entity_threads = 0;

	return 0;
}

int threads_execute_tcp() {
/*	if(__heartbeat) {
		pthread_mutex_lock(&tcp_handler_args.heartbeat_mutex);
		if(!tcp_handler_args.heartbeat_received) {
			perror("threads_execute_tcp no heartbeat");
			pthread_mutex_unlock(&tcp_handler_args.heartbeat_mutex);
			return -1;
		}
		tcp_handler_args.heartbeat_received = false;
		tcp_handler_args.heartbeat_send_issued = true;
		pthread_mutex_unlock(&tcp_handler_args.heartbeat_mutex);
	}*/
	if(sem_post(&(tcp_handler_args.tcp_sem))<0) {
		perror("threads_execute_tcp sem_post");
		return -1;
	}

	return 0;
}

int threads_start_tcp(int __sockfd, entity_t* __entity,
	void (*__entity_timesync)(uint32_t),
	void (*__entity_play)(void),
	void (*__entity_pause)(void),
	void (*__entity_show)(void)) {
	//Some checks to ensure we can proceed
	if(!init) {
		perror("threads_start_tcp init false");
		return -1;
	}
	if(tcp_thread_started) {
		perror("threads_start_tcp tcp_thread_started true");
		return -1;
	}

	//Init args ands start thread
	if(tcp_handler_args_init(&tcp_handler_args, __sockfd, __entity_timesync, __entity_play, __entity_pause, __entity_show, __entity)<0) {
		perror("threads_start_tcp tpc_handler_args_init");
		return -1;
	}
	if(pthread_create(&tcp_thread, NULL, tcp_handler, (void*)&tcp_handler_args)<0) {
		perror("threads_start_tcp pthread_create");
		return -1;
	}

	tcp_thread_started = true;
	return 0;
}

int threads_exit_tcp() {
	int res = 0;
	if(!tcp_thread_started) {
		perror("threads_exit_tcp tcp_threads_started false");
		return -1;
	}

	//Tell the thread to exit
	if(pthread_mutex_lock(&(tcp_handler_args.mutex))<0) {
		perror("threads_exit_tcp pthread_mutex_lock");
		res = -1;
	}
	tcp_handler_args.thread_exit_issued = true;
	if(pthread_mutex_unlock(&(tcp_handler_args.mutex))<0) {
		perror("threads_exit_tcp pthread_mutex_unlock");
		res = -1;
	}

	//Execute all threads so they can exit
	threads_execute_tcp(false);

	//Wait for all threads to exit
	if(pthread_join(tcp_thread, NULL)<0) {
		perror("threads_exit_tcp pthread_join");
		res = -1;
	}

	return res;
}

int threads_cleanup_tcp() {
	int res = 0;
	if(tcp_handler_args_destroy(&tcp_handler_args)<0) {
		perror("threads_cleanup_tcp tcp_handler_args_destroy");
		res = -1;
	}

	tcp_thread_started = false;

	return res;
}
