#include "threads.h"
#include "entity/entity_handler.h"
#include "entity/entity.h"

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

static bool init = false;
static bool entity_threads_started = false;
static bool tcp_thread_started = false;

static int num_entity_threads = 0;

static sem_t tcp_sem;

static pthread_t* entity_threads;
static pthread_t tcp_thread;

static entity_handler_args_t* entity_handler_args;

static pthread_mutex tcp_exit_mutex;
static bool tcp_exit_issued = false;

int threads_init((void*) __clean_add) {
	if(__clean_add(threads_cleanup)<0) {
		perror("threads_init __clean_add");
		return -1;
	}

	init = true;
	return 0;
}

int threads_execute_entity(long __current_frame) {
	for(int i=0; i<num_entity_threads; i++) {
		if(pthread_mutex_lock(&(entity_handler_args[i].mutex))<0) {
			perror("threads_execute_entity pthread_mutex_lock");
			return -1;
		}

		entity_handler_args[i].current_frame = __current_frame;

		if(pthread_mutex_unlock(&(entity_handler_args[i].mutex))<0) {
			perror("threads_execute_entity pthread_mutex_unlock");
			return -1;
		}

		if(sem_post(&(entity_handler_args[i].entity_sem))<0) {
			perror("threads_execute_entity sem_post");
			return -1;
		}
	}

	return 0;
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
	entity_exit_issued = false;
	num_entity_threads = __num_bus;

	//Acquire heap space
	if((entity_handler_args=(entity_handler_args_t*)malloc(sizeof(entity_handler_args_t)*__num_bus))<0) {
		perror("threads_start_entity entity_handler_args malloc");
		return -1;
	}
	if((entity_threads=(pthread_t*)malloc(sizeof(pthread_t)*__num_bus))<0) {
		perror("threads_start_entity entity_threads malloc");
		return -1;
	}

	//Initialize args and start threads
	for(int i=0; i<__num_bus; i++) {
		if(entity_handler_args_init(&entity_handler_args[i], &(__entity->bus[i]))<0) {
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
	if(!entity_threads_started) {
		perror("threads_exit_entity entity_threads_started false");
		return -1;
	}

	//Tell all threads to exit
	for(int i=0; i<num_entity_threads; i++) {
		if(pthread_mutex_lock(&(entity_handler_args[i].mutex))<0) {
			perror("threads_exit_entity pthread_mutex_lock");
			return -1;
		}
		entity_handler_args[i].thread_exit_issued = true;
		if(pthread_mutex_unlock(&(entity_handler_args[i].mutex))<0) {
			perror("threads_exit_entity pthread_mutex_unlock");
			return -1;
		}
	}

	//Execute all threads so they can exit
	threads_execute_entity(0);

	//Wait for all threads to exit
	for(int i=0; i<num_entity_threads; i++) {
		if(pthread_join(&entity_threads[i], NULL)<0) {
			perror("threads_exit_entity pthread_join");
			return -1;
		}
	}

	//Clear all remaining stuff
	threads_cleanup_entity();
	return 0;
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
}

int threads_execute_tcp() {
	if(sem_post(&tcp_sems)<0) {
		perror("threads_execute_tcp sem_post");
		return -1;
	}
}

int threads_start_tcp() {
	//Some checks to ensure we can proceed
	if(!init) {
		perror("threads_start_tcp init false");
		return -1;
	}
	if(tcp_thread_started) {
		perror("threads_start_tcp tcp_thread_started true");
		return -1;
	}

	//Setting default values
	tcp_exit_issued = false;

	//Init semaphore and start thread
	if(sem_init(&tcp_sem, 0, 0)<0) {
		perror("threads_start_tcp sem_init tcp_sem");
		return -1;
	}
	//TODO give thread entity_sem on creation to wait on
	if(pthread_create(&tcp_thread, NULL, tcp_handler, NULL)<0) {
		perror("threads_start_tcp pthread_create");
		return -1;
	}

	tcp_thread_started = true;
	return 0;
}

int threads_exit_tcp() {
	if(!tcp_threads_started) {
		perror("threads_exit_tcp tcp_threads_started false");
		return -1;
	}

	pthread_mutex_lock(&tcp_exit_mutex);
	tcp_exit_issued = true;
	pthread_mutex_unlock(&tcp_exit_mutex);
}
