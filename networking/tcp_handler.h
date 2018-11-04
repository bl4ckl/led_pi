#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#include <pthread.h>
#include <semaphore.h>

#define MESSAGE_NULL		0
#define MESSAGE_ID              1
#define MESSAGE_CONFIG          2
#define MESSAGE_EFFECTS         3
#define MESSAGE_TIME            4
#define MESSAGE_PLAY            5
#define MESSAGE_PAUSE           6
#define MESSAGE_PREVIEW         7
#define MESSAGE_SHOW            8
#define MESSAGE_COLOR           9
#define MESSAGE_RESEND		10
#define MESSAGE_READY		11


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

	//Ref to function calls
	void*		entity_timesync(uint32_t);
	void*		entity_play();
	void*		entity_pause();
	void*		entity_show();

	//Ref to entity
	entity_t*	entity;

	char[13]	id;
}tcp_handler_args_t;

int tcp_handler_init();
int tcp_handler_args_init(tcp_handler_args_t* __args, int __sockfd, entity_t* __entity);
int tcp_handler_args_destroy(tcp_handler_args_t* __args);
void* tcp_handler(void* __args);

static int identify_message(tcp_handler_message_t* __message, char* __buffer[1024]);

static void reset_message(tcp_handler_message_t* __message);
static void send_to_server(tcp_handler_args* __args, uint8_t __message_id);

#endif /* TCP_HANDLER_H */
