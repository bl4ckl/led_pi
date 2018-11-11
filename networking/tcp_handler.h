#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#include "../entity/entity.h"
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE	8*1024

#define MESSAGE_HEADER_LENGTH		8
#define MESSAGE_HEADER_SECRET_OFFSET	0
#define MESSAGE_HEADER_ID_OFFSET	2
#define MESSAGE_HEADER_DATA_OFFSET	4

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
#define MESSAGE_HEARTBEAT	12

typedef struct {
        uint16_t         id;
        int32_t		total_data_length;
        int32_t         total_data_received;
        char*           data;
}tcp_handler_message_t;

typedef struct {
	//Mutex for struct access
	pthread_mutex_t	mutex;

	//The thread waits on this sem
	sem_t		tcp_sem;

	//If we want to exit, this becomes true
	pthread_mutex_t	exit_mutex;
	bool		thread_exit_issued;

	//Socket file descriptor
	int		sockfd;

	//Ref to function calls
	void		(*entity_timesync)(uint32_t);
	void		(*entity_config_effects_pre)(void);
	void		(*entity_effects_post)(void);
	void		(*entity_play)(void);
	void		(*entity_pause)(void);
	void		(*entity_show)(void);

	//Ref to entity
	entity_t*	entity;

	//Heartbeat bool
	pthread_mutex_t	heartbeat_mutex;
	bool		heartbeat_send_issued;
	bool		heartbeat_received;
}tcp_handler_args_t;

int tcp_handler_init();
int tcp_handler_args_init(tcp_handler_args_t* __args, int __sockfd,
        void (*__entity_timesync)(uint32_t),
	void (*__entity_config_effects_pre)(void),
	void (*__entity_effects_post)(void),
        void (*__entity_play)(void),
        void (*__entity_pause)(void),
        void (*__entity_show)(void),  entity_t* __entity);
int tcp_handler_args_destroy(tcp_handler_args_t* __args);
void* tcp_handler(void* __args);

#endif /* TCP_HANDLER_H */
