#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#include <pthread.h>
#include <semaphore.h>

typedef struct {
        uint8_t         id;
        uint32_t        total_data_length;
        uint32_t        total_data_received;
        char*           data;
}tcp_handler_message_t;

typedef struct {
	//Mutex for struct access
	pthread_mutex_t	mutex;

	//The thread waits on this sem
	sem_t		tcp_sem;

	//If we want to exit, this becomes true
	bool		thread_exit_issued;

	//Socket file descriptor
	int		sockfd;

	//Ref to function call to update entity
	void*		entity_update(long);

	//Ref to entity
	entity_t*	entity;
}tcp_handler_args_t;

int tcp_handler_init();
int tcp_handler_args_init(tcp_handler_args_t* __args, int __sockfd, entity_t* __entity);
int tcp_handler_args_destroy(tcp_handler_args_t* __args);
void* tcp_handler(void* __args);

static int identify_message(tcp_handler_message_t* __message, char* __buffer[1024]);

static void reset_message(tcp_handler_message_t* __message);
static void send_to_server(tcp_handler_args* __args, uint8_t __message_id);

#endif /* TCP_HANDLER_H */
