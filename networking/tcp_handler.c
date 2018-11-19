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

static int identify_message(tcp_handler_message_t* __message, char __buffer[1024], int* __buffer_received);
static void reset_message(tcp_handler_message_t* __message);
static void handle_message(tcp_handler_args_t * __args, tcp_handler_message_t* __message);
static void send_to_server(tcp_handler_args_t* __args, uint16_t __message_id, char* data);


static bool init = true;

int tcp_handler_init() {
	init = true;
	return 0;
}

int tcp_handler_args_init(tcp_handler_args_t* __args, int __sockfd,
	void (*__entity_timesync)(uint32_t),
        void (*__entity_config_effects_pre)(void),
        void (*__entity_effects_post)(void),
	void (*__entity_play)(void),
	void (*__entity_pause)(void),
	void (*__entity_show)(void),
	void (*__entity_color)(uint8_t*), entity_t* __entity) {

	memset(__args, 0, sizeof(tcp_handler_args_t));

	if(pthread_mutex_init(&(__args->mutex), NULL)<0) {
		perror("tcp_handler_args_init pthread_mutex_init");
		return -1;
	}
	if(pthread_mutex_init(&(__args->exit_mutex), NULL)<0) {
		perror("tcp_handler_args_init pthread_mutex_init");
		return -1;
	}
	if(pthread_mutex_init(&(__args->heartbeat_mutex), NULL)<0) {
		perror("tcp_handler_args_init pthread_mutex_init");
		return -1;
	}

	if(sem_init(&(__args->tcp_sem), 0, 0)<0) {
		perror("tcp_handler_args_init sem_init");
		return -1;
	}

	__args->sockfd = __sockfd;
	__args->entity_timesync = __entity_timesync;
	__args->entity_config_effects_pre = __entity_config_effects_pre;
	__args->entity_effects_post = __entity_effects_post;
	__args->entity_play = __entity_play;
	__args->entity_pause = __entity_pause;
	__args->entity_show = __entity_show;
	__args->entity_color = __entity_color;
	__args->entity = __entity;
	__args->heartbeat_received = true;

	return 0;
}

int tcp_handler_args_destroy(tcp_handler_args_t* __args) {
	if(pthread_mutex_destroy(&(__args->mutex))<0) {
		perror("tcp_handler_args_destroy pthread_mutex_destroy");
		return -1;
	}
	if(pthread_mutex_destroy(&(__args->exit_mutex))<0) {
		perror("tcp_handler_args_destroy pthread_mutex_destroy");
		return -1;
	}
	if(pthread_mutex_destroy(&(__args->heartbeat_mutex))<0) {
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

int check_exit_issued(tcp_handler_args_t* __args) {
	//Check for exit issued
	pthread_mutex_lock(&(__args->exit_mutex));
	if(__args->thread_exit_issued) {
		pthread_mutex_unlock(&(__args->exit_mutex));
		return -1;
	} pthread_mutex_unlock(&(__args->exit_mutex));
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

	int buffer_received = 0;
	int recv_result = 0;
	char buffer[BUFFER_SIZE];
	memset(&buffer[0], 0, BUFFER_SIZE);

	int inner_loop_count = 0;

	//start main loop
	while(1) {
		//Wait for signal from main_thread
		sem_wait(&args->tcp_sem);

		//Lock arg mutex
		pthread_mutex_lock(&(args->mutex));

		if(check_exit_issued(args)<0) {
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


		//Process incoming data
		//Read everything from the socket and store it in buffer
		//If there is previous data in the buffer, the new data gets added at the end of the old data
	        recv_result = recv(args->sockfd, &buffer[buffer_received], sizeof(buffer)-buffer_received, 0);

		//When we get -1 and it was a "real" error print it out
		if(recv_result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
			perror("tcp_handler_recv < 0");
		} else if(recv_result > 0) {
//			printf("Received %d bytes\n", recv_result);
			buffer_received += recv_result;
		}
		recv_result = 0;

		//Check if we got new data or have data left from the previous iteration
		if(buffer_received > 0) {

//			printf("Received %d bytes\n", buffer_received);
			//Identify the message
			if(identify_message(&message, &buffer[0], &buffer_received)<0) {
				reset_message(&message);
				buffer_received = 0;
				memset(&buffer[0], 0, BUFFER_SIZE);
				perror("tcp_handler identify_message");
			}
			else {
				printf("tcp_handler:\tmessage identified:\n\t\tid: %d\n\t\ttotal_data_length: %d\n", message.id, message.total_data_length);
				fflush(stdout);

				//When we didn't receive all the data we need recv till we got it
				while(message.total_data_received < message.total_data_length) {
					if(inner_loop_count >= 100) {
						inner_loop_count = 0;

						if(check_exit_issued(args)<0) {
							pthread_mutex_unlock(&(args->mutex));
							pthread_exit(NULL);
						}
					} else {
						inner_loop_count++;
					}

					recv_result = recv(args->sockfd, &buffer[0], BUFFER_SIZE, 0);
					if(recv_result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
						perror("tcp_handler_recv < 0");
					} else if(recv_result > 0) {
						buffer_received += recv_result;
					}
					recv_result = 0;


					if(buffer_received > 0) {
						int data_needed = message.total_data_length - message.total_data_received;
						if(buffer_received < data_needed) {
							memcpy(&message.data[message.total_data_received], &buffer[0], buffer_received);
							message.total_data_received += buffer_received;
							buffer_received = 0;
							memset(&buffer[0], 0, buffer_received);
						} else {
							memcpy(&message.data[message.total_data_received], &buffer[0], data_needed);
							message.total_data_received += data_needed;
							buffer_received -= data_needed;
							memcpy(&buffer[0], &buffer[data_needed], buffer_received);
							memset(&buffer[buffer_received], 0, data_needed + buffer_received);
						}
					}
				}
				if(message.total_data_received >= message.total_data_length) {
					printf("\t\ttotal_data_received: %d\n", message.total_data_received);
					fflush(stdout);

					handle_message(args, &message);
					reset_message(&message);
					//send_to_server(args, MESSAGE_READY, NULL);
					//printf("sent ready");
					//fflush(stdout);
				}
				else {
					perror("tcp_handler something horrible happened");
				}
			}
		}

		pthread_mutex_unlock(&(args->mutex));
	}
}

static int identify_message(tcp_handler_message_t* __message, char __buffer[1024], int* __buffer_received) {
	//A message needs to be atleast 8 bytes
	if(*__buffer_received >= MESSAGE_HEADER_LENGTH) {
		if((uint16_t)42 != ntohs(*(uint16_t*)&__buffer[MESSAGE_HEADER_SECRET_OFFSET])) {
			perror("tcp_handler identify_message secret");
			return -1;
		}
		//First 2 bytes represents message_id
		__message->id = ntohs(*(uint16_t*)&__buffer[MESSAGE_HEADER_ID_OFFSET]);
		//Next 4 bytes is message length
		__message->total_data_length = ntohl(*(int32_t*)&__buffer[MESSAGE_HEADER_DATA_OFFSET]);
		//print_bits(4, &__buffer[MESSAGE_HEADER_DATA_OFFSET]);
		if(*__buffer_received-MESSAGE_HEADER_LENGTH > __message->total_data_length) {
			__message->total_data_received = __message->total_data_length;
			*__buffer_received -= __message->total_data_length+MESSAGE_HEADER_LENGTH;
		} else {
			__message->total_data_received = *__buffer_received-MESSAGE_HEADER_LENGTH;
			*__buffer_received = 0;
		}
		//Malloc heap space for the data
		if((__message->data=(char*)malloc(__message->total_data_length))==NULL) {
			perror("tcp_handler identify_message malloc");
			return -1;
		}
		//Copy the received data from buffer
		memcpy(__message->data, &__buffer[MESSAGE_HEADER_LENGTH], __message->total_data_received);

		//Delete read data from buffer
		if(*__buffer_received > 0) {
			memcpy(&__buffer[0], &__buffer[__message->total_data_received+MESSAGE_HEADER_LENGTH], *__buffer_received);
			memset(&__buffer[*__buffer_received], 0, BUFFER_SIZE-(*__buffer_received));
			fflush(stdout);
		} else {
			memset(&__buffer[0], 0, BUFFER_SIZE);
		}
	} else {
		perror("tcp_handler identify_message total_data_received < MESSAGE_HEADER_LENGTH");
		return -1;
	}

	return 0;
}

static void handle_message(tcp_handler_args_t * __args, tcp_handler_message_t* __message) {
	switch(__message->id) {
		case MESSAGE_ID:
			send_to_server(__args, MESSAGE_ID, __args->entity->id);
			break;
		case MESSAGE_CONFIG:
			__args->entity_config_effects_pre();
			if(entity_write_config(__args->entity, __message->data)<0) {
				perror("tcp_handler handle_message MESSAGE_CONFIG");
			}
			break;
		case MESSAGE_EFFECTS:
//			printf("handle_message\tentity_config_effects_pre\n");
			__args->entity_config_effects_pre();
//			printf("handle_message\tentity_write\n");
			if(entity_write_effects( __args->entity, __message->data)<0) {
				perror("tcp_handler handle_message MESSAGE_EFFECTS");
			}
//			printf("handle_message\tentity_effects_post\n");
			__args->entity_effects_post();
			printf("handle_message\tdone\n");
			break;
		case MESSAGE_TIME:
			__args->entity_timesync(ntohl(*(uint32_t*)&__message->data[0]));
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
		case MESSAGE_COLOR: {
			uint8_t color[4];
			color[0] = __message->data[0];
			color[1] = __message->data[1];
			color[2] = __message->data[2];
			color[3] = __message->data[3];
			__args->entity_color(color);
			break;
}
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
	send(__args->sockfd, &send_buffer[0], send_buffer_length, 0);
}

static void reset_message(tcp_handler_message_t* __message) {
	memset(__message, 0, sizeof(tcp_handler_message_t));
}
