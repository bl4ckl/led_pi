#include "tcp_handler.h"
#include "../utility.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>

static int identify_message(tcp_handler_message_t* __message, char __buffer[1024]);
static void reset_message(tcp_handler_message_t* __message, char __buffer[1024]);
static void handle_message(tcp_handler_args_t * __args, tcp_handler_message_t* __message);
static void send_to_server(tcp_handler_args_t* __args, uint16_t __message_id, char* data);


static bool init = true;

int tcp_handler_init() {
	init = true;
	return 0;
}

int tcp_handler_args_init(tcp_handler_args_t* __args, int __sockfd,
	void (*__entity_timesync)(uint32_t),
	void (*__entity_play)(void),
	void (*__entity_pause)(void),
	void (*__entity_show)(void),  entity_t* __entity) {

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
	__args->entity_timesync = __entity_timesync;
	__args->entity_play = __entity_play;
	__args->entity_pause = __entity_pause;
	__args->entity_show = __entity_show;
	__args->entity = __entity;
	__args->heartbeat_received = true;

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
	tcp_handler_args_t* args = (tcp_handler_args_t*)__args;

	//build data strucutre
	tcp_handler_message_t message;
	memset(&message, 0, sizeof(tcp_handler_message_t));

	char buffer[1024*8];
	memset(&buffer[0], 0, sizeof(buffer));

	bool message_identified = false;

	//start main loop
	while(1) {
		//Wait for signal from main_thread
		sem_wait(&args->tcp_sem);

		//Lock arg mutex
		pthread_mutex_lock(&(args->mutex));
		//Check for exit issued
		if(args->thread_exit_issued) {
			pthread_mutex_unlock(&(args->mutex));
			pthread_exit(NULL);
		}

/*		//Check for heartbeat issued
		pthread_mutex_lock(&args->heartbeat_mutex);
		if(args->heartbeat_send_issued) {
			send_to_server(args, MESSAGE_HEARTBEAT, NULL);
		}
		pthread_mutex_unlock(&args->heartbeat_mutex);
*/
		//Process the message
		if(!message_identified) {
		        message.total_data_received = recv(args->sockfd, &buffer[0], sizeof(buffer), 0);
			if(message.total_data_received > 0) {
				if(identify_message(&message, &buffer[0])<0) {
					perror("tcp_handler identify_message");
				} else {
					if(message.id != MESSAGE_HEARTBEAT) {
						printf("tcp_handler:\tmessage identified:\n\t\tid: %d\n\t\ttotal_data_length: %d\n\t\ttotal_data_received: %d\n", message.id, message.total_data_length, message.total_data_received);
						fflush(stdout);
						message_identified = true;
					}
				}
			} else if(message.total_data_received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {

			} else {
				perror("tcp_handler message recv < 0");
			}
		} else {
			do {
				int32_t data_received = recv(args->sockfd, &buffer[0], sizeof(buffer), 0);
				if(data_received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {

				}
				else if(data_received < 0) {
					perror("tcp_handler recv -1");
					//reset_message(&message, &buffer[0]);
					//send_to_server(args, MESSAGE_RESEND, NULL);
				} else {
					memcpy(&message.data[message.total_data_received], &buffer[0], data_received);
					memset(&buffer[0], 0, sizeof(buffer));
		       		 	message.total_data_received += data_received;
					printf("\t\ttotal_data_received: %d\n", message.total_data_received);
					fflush(stdout);
				}
			} while(message.total_data_received < message.total_data_length);
		}

		if((message.id != MESSAGE_NULL) && (message.total_data_length == message.total_data_received)) {
			handle_message(args, &message);
			reset_message(&message, &buffer[0]);
			message_identified = false;
			send_to_server(args, MESSAGE_READY, NULL);
		} else if(message.total_data_length < message.total_data_received) {
			reset_message(&message, &buffer[0]);
			message_identified = false;
			send_to_server(args, MESSAGE_RESEND, NULL);
		} else if(message.id == MESSAGE_NULL) {
			reset_message(&message, &buffer[0]);
			message_identified = false;
		}

		pthread_mutex_unlock(&(args->mutex));
	}
}

static int identify_message(tcp_handler_message_t* __message, char __buffer[1024]) {
	//A message needs to be atleast 5 bytes
	if(__message->total_data_received >= MESSAGE_HEADER_LENGTH) {
		if((uint16_t)42 != ntohs(*(uint16_t*)&__buffer[MESSAGE_HEADER_SECRET_OFFSET])) {
			//print_bits(2, &__buffer[0]);
			reset_message(__message, &__buffer[0]);
			perror("tcp_handler identify_message secret");
			return -1;
		}
		//First 2 bytes represents message_id
		__message->id = ntohs(*(uint16_t*)&__buffer[MESSAGE_HEADER_ID_OFFSET]);
		//Next 4 bytes is message length
		__message->total_data_length = ntohl(*(int32_t*)&__buffer[MESSAGE_HEADER_DATA_OFFSET]);
		//print_bits(4, &__buffer[MESSAGE_HEADER_DATA_OFFSET]);
		//Substract the 5 bytes we already read
		__message->total_data_received -= MESSAGE_HEADER_LENGTH;
		//Malloc heap space for the rest of the data
		if((__message->data=(char*)malloc(__message->total_data_length))==NULL) {
			reset_message(__message, &__buffer[0]);
			perror("tcp_handler identify_message malloc");
			return -1;
		}
		//Copy the received data from buffer
		memcpy(__message->data, &__buffer[MESSAGE_HEADER_LENGTH], __message->total_data_received);
		//Reset the buffer again
		memset(&__buffer[0], 0, sizeof(*__buffer));
	} else {
		reset_message(__message, &__buffer[0]);
		perror("tcp_handler identify_message total_data_received < MESSAGE_HEADER_LENGTH");
		return -1;
	}

	return 0;
}

static void handle_message(tcp_handler_args_t * __args, tcp_handler_message_t* __message) {
	switch(__message->id) {
		case MESSAGE_ID:
			if(pthread_mutex_lock(&(__args->entity->mutex))<0) {
				perror("tcp_handler handle_message pthread_mutex_lock");
				send_to_server(__args, MESSAGE_RESEND, NULL);
				break;
			}
			send_to_server(__args, MESSAGE_ID, __args->entity->id);
			if(pthread_mutex_unlock(&(__args->entity->mutex))<0) {
				perror("tcp_handler handle_message pthread_mutex_unlock");
				break;
			}
			break;
		case MESSAGE_CONFIG:
			if(entity_write_config(__args->entity, __message->data)<0) {
				perror("tcp_handler handle_message MESSAGE_CONFIG");
			}
			break;
		case MESSAGE_EFFECTS:
			if(entity_write_effects( __args->entity, __message->data)<0) {
				perror("tcp_handler handle_message MESSAGE_EFFECTS");
			}
			break;
		case MESSAGE_TIME:
			__args->entity_timesync(ntohl(*(uint32_t*)&__message->data));
			break;
		case MESSAGE_PLAY:
			__args->entity_play();
			break;
		case MESSAGE_PAUSE:
			__args->entity_pause();
			break;
		case MESSAGE_PREVIEW:
			break;
		case MESSAGE_SHOW:
			__args->entity_show();
			break;
		case MESSAGE_COLOR:
			break;
		case MESSAGE_HEARTBEAT:
			pthread_mutex_lock(&__args->heartbeat_mutex);
			__args->heartbeat_received = true;
			pthread_mutex_unlock(&__args->heartbeat_mutex);
			break;
	}
}

static void send_to_server(tcp_handler_args_t* __args, uint16_t __message_id, char* data) {
	uint32_t send_buffer_length = 8;
	uint32_t data_length = 0;
	if(&data[0] != NULL) {
		send_buffer_length += (uint32_t)strlen(data);
		data_length = htonl((uint32_t)strlen(data));
	}
	char send_buffer[send_buffer_length];

	uint16_t secret = htons(42);
	memcpy(&send_buffer[0], &secret, sizeof(uint16_t));

	uint16_t message_id  = htons(__message_id);
	memcpy(&send_buffer[2], &message_id, sizeof(uint16_t));

	memcpy(&send_buffer[4], &data_length, sizeof(uint32_t));

	if(&data[0] != NULL) {
		memcpy(&send_buffer[8], &data[0], strlen(data));
	}
	send(__args->sockfd, &send_buffer[0], sizeof(send_buffer), 0);
}

static void reset_message(tcp_handler_message_t* __message, char __buffer[1024]) {
	memset(__message, 0, sizeof(tcp_handler_message_t));
	memset(&__buffer[0], 0, sizeof(*__buffer));
}

/*
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
*/
