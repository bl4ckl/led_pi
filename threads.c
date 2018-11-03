#include <pthread.h>

static bool init = false;
static bool entity_threads_started = false;
static bool tcp_thread_started = false;

static int num_entity_threads = 0;

static sem_t* entity_sems;
static sem_t tcp_sem;

static pthread_t* entity_threads;
static pthread_t tcp_thread;

static pthread_mutex_t entity_exit_mutex;
static bool entity_exit_issued = false;
static pthread_mutex_tcp_exit_mutex;
static bool tcp_exit_issued = false;

int threads_init((void*) __clean_array) {
	init = true;
	return 0;
}

int threads_execute_entity() {
	for(int i=0; i<num_entity_threads; i++) {
		if(sem_post(&entity_sems[i])<0) {
			perror("threads_execute_entity sem_post");
			return -1;
		}
	}

	return 0;
}

int threads_start_entity(uint16_t __num_bus) {
	if(!init) {
		perror("threads_start_entity init false");
		return -1;
	}
	if(entity_threads_started) {
		perror("threads_start_entity entity_threads_started true");
		return -1;
	}

	entity_exit_issued = false;
	num_entity_threads = __num_bus;

	if((entity_sems=(sem_t*)malloc(sizeof(sem_t)*__num_bus))<0) {
		perror("threads_start_entity entity_sems malloc");
		return -1;
	}

	if((entity_threads=(pthread_t*)malloc(sizeof(pthread_t)*__num_bus))<0) {
		perror("threads_start_entity entity_threads malloc");
		return -1;
	}

	for(int i=0; i<__num_bus; i++) {
		if(sem_init(&entity_sems[i], 0, -1)<0) {
			perror("threads_start_entity sem_init");
			return -1;
		}
		//TODO give thread entity_sem on creation to wait on
		if(pthread_create(&entity_threads[i], NULL, entity_handler, NULL)<0) {
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

	pthread_mutex_lock(&entity_exit_mutex);
	entity_exit_issued = true;
	pthread_mutex_unlock(&entity_exit_mutex);

	free((void*)entity_sems);
	free((void*)entity_threads);
	entity_threads_started = false;

	return 0;
}

int threads_execute_tcp() {
	if(sem_post(&tcp_sems)<0) {
		perror("threads_execute_tcp sem_post");
		return -1;
	}
}

int threads_start_tcp() {
	if(!init) {
		perror("threads_start_tcp init false");
		return -1;
	}
	if(tcp_thread_started) {
		perror("threads_start_tcp tcp_thread_started true");
		return -1;
	}

	tcp_exit_issued = false;

	if(sem_init(&tcp_sem, 0, -1)<0) {
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
