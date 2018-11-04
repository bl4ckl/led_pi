#include "tcphandler.h"

#include <stdbool.h>
#include <stdint.h>
#include <socket.h>
#include <pthread.h>
#include <semaphore.h>

static bool init = false;

int tcp_handler_init() {
	init = true;
	return 0;
}

int tcp_handler_args_init(tcp_handler_args_t* __args, int __sockfd, void* __entity_update(long), entity_t* __entity) {
	memset(__args, 0, sizeof(tcp_handler_args_t));

	if(pthread_mutex_init(&(__args->mutex), NULL)<0) {
		perror("tcp_handler_args_init pthread_mutex_init");
		return -1;
	}

	if(sem_init(&(__args->tcp_sem), 0, 0)<0) {
		perror("tcp_handler_args_init sem_init");
		return -1;
	}

	__args->sockfd = __sockfd;
	__args->entity_update = __entity_update;
	__args->entity = __entity;

	return 0;
}

int tcp_handler_args_destroy(tcp_handler_args_t* __args) {
	if(pthread_mutex_destroy(&(__args->mutex))<0) {
		perror("tcp_handler_args_destroy pthread_mutex_destroy");
		return -1;
	}

	if(sem_destroy(&(__args->tcp_sem))<0) {
		perror("tcp_handler_args_destroy sem_destroy");
		return -1;
	}

	__args->sockfd = 0;
	__args->entity = NULL;
	return 0;
}

void* tcp_handler(void* __args) {
	if(!init) {
		perror("tcp_handler init false");
		pthread_exit(NULL);
	}

	//get refs
	tcp_handler_args* args = (tcp_handler_args*)__args;

	//build data strucutre
	tcp_handler_message_t message;
	memset(&message, 0, sizeof(tcp_handler_message_t));

	char buffer[1024];
	memset(&buffer[0], 0, sizeof(buffer));

	bool message_identified = false;

	//start main loop
	while(1) {
		//Wait for signal from main_thread
		sem_wait(__args->tcp_sem);

		//Lock arg mutex
		pthread_mutex_lock(&(args->mutex));
		//Check for exit issued
		if(__args->thread_exit_issued) {
			pthread_mutex_unlock(&(args->mutex));
			pthread_exit(NULL);
		}

		//Lock entity mutex
		pthread_mutex_lock(&(args->entity->mutex));

		//Process the message
		if(!message_identified) {
		        message.total_data_received += recv(args->sockfd, &buffer[0], sizeof(buffer), 0);
			if(identify_message(&message, &buffer[0])<0) {
				perror("tcp_handler identify_message");
				send_to_server(MESSAGE_RESEND);
			}
			message_identified = true;
		} else {
			uint32_t data_received = recv(args->sockfd, &buffer[0], sizeof(buffer), 0);
			memcpy(&message.data[message.total_data_received], &buffer[0], data_received);
			memset(&buffer[0], 0, sizeof(buffer));
		        message.total_data_received += data_received;
		}

		if((message.id != MESSAGE_NULL) && (message.total_data_length == message.total_data_received)) {
			handle_message(&message, args->entity);
			reset_message(&message);
			send_to_server(MESSAGE_READY);
		} else if(message.total_data_length < message.total_data_received) {
			reset_message(&message);
			send_to_server(MESSAGE_RESEND);
		}

		pthread_mutex_unlock(&(args->entity->mutex));
		pthread_mutex_unlock(&(args->mutex));
	}
}

static int identify_message(tcp_handler_message_t* __message, char* __buffer[1024]) {
	//A message needs to be atleast 5 bytes
	if(__message->total_data_received >= 5) {
		//First byte represents message_id
		__message->id = (uint8_t)__buffer[0];
		//Next 4 bytes is message length
		__message->total_data_length = (uint32_t)__buffer[1];
		//Substract the 5 bytes we already read
		__message->total_data_received -= 5;
		//Malloc heap space for the rest of the data
		if((__message->data=(char*)malloc(__message->total_data_length))==NULL) {
			memset(__mesage, 0, sizeof(tcp_handler_message_t));
			memset(&__buffer[0], 0, sizeof(__buffer));
			perror("tcp_handler identify_message malloc");
			return -1;
		}
		//Copy the received data from buffer
		memcpy(__message->data, &__buffer[0], total_data_received);
		//Reset the buffer again
		memset(&__buffer[0], 0, sizeof(__buffer));
	} else {
		memset(__mesage, 0, sizeof(tcp_handler_message_t));
		memset(&__buffer[0], 0, sizeof(__buffer));
		perror("tcp_handler identify_message total_data_received < 5");
		return -1;
	}
}

static void handle_message(tcp_handler_message* __message, entity* __entity) {
	switch(__message->id) {
		case MESSAGE_ID:
			break;
		case MESSAGE_CONFIG:
			break;
		case MESSAGE_EFFECTS:
			break;
		case MESSAGE_TIME:
			break;
		case MESSAGE_PLAY:
			break;
		case MESSAGE_PAUSE:
			break;
		case MESSAGE_PREVIEW:
			break;
		case MESSAGE_SHOW:
			break;
		case MESSAGE_COLOR:
			break;
	}
}

static void send_to_server(tcp_handler_args* __args, uint8_t __message_id) {
	char send_buffer[5];
	send_buffer[0] = __message_id;
	memset(&send_buffer[0], 0, 4);
	send(__args->sockfd, &send_buffer[0], sizeof(send_buffer), 0);
}

static void reset_message(tcp_handler_message_t* __message) {
	memset(__mesage, 0, sizeof(tcp_handler_message_t));
	memset(&__buffer[0], 0, sizeof(__buffer));
	message_identified = false;
}

void TcpHandler(int signalType)
{
        memset((void*)&message, 0, sizeof(message));
        memset((void*)&buffer, 0, sizeof(buffer));

        //printf("receiving data\n");
        //printf("ending data\n");
        buffer[message.dataReceived] = '\0';
        //printf("get message id\n");
        message.messageID  = *buffer;
        message.dataLength = buffer[1];
        message.data = (char*)&buffer[3];

        printf("tcpclient received message with id %d\n", message.messageID);

        switch(message.messageID) {
                case MESSAGE_ID: {
                        char sendBuf[15];
                        memset(&sendBuf, 0, sizeof(sendBuf));
                        uint16_t sizeOfBuf = 12;
                        memcpy(&sendBuf[1], &sizeOfBuf, sizeof(sizeOfBuf));
                        memcpy(&sendBuf[3], &id, sizeof(id)-1);
                        send(tcpsockfd, sendBuf, sizeof(sendBuf), 0);
                        break; }
                case MESSAGE_CONFIG:
                        if (entity_write_config((entity_t*)entity, message.data)<0) {
                                printf("entity_write_config() failed.");
                        } else {
                                printf("entity_write_config() succesfull.");
                        }
                        break;
                case MESSAGE_EFFECTS:
                        if (entity_write_effects((entity_t*)entity, message.data)<0) {
                                printf("entity_write_effect() failed.");
                        } else {
                                printf("entity_write_effect() succesfull.");
                        }
                        break;
                case MESSAGE_TIME:
                        break;
                case MESSAGE_PLAY:
                        entity_play((entity_t*)entity, (timer_t*)it_id, (struct itimerspec*)it_spec);
                        break;
                case MESSAGE_PAUSE: {
                        entity_stop((timer_t*)it_id);
                        unsigned char color[4];
                        memset(&color[0], 0, 4);
                        entity_full((entity_t*)entity, color);
                        break; }
                case MESSAGE_PREVIEW:
                        break;
                case MESSAGE_SHOW: {
                        unsigned char color[4];
                        memset(&color[0], 0xFF, 4);
                        entity_full((entity_t*)entity, color);
                        break; }
                case MESSAGE_COLOR:
                        break;
        }
}

