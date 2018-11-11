#include "entity_handler.h"
#include "entity.h"
#include "../spi.h"
#include "../utility.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

static void handle_new_frame(entity_handler_args_t* __args);
static void write_spi(entity_handler_args_t* __args);

static bool init = false;

static sem_t volatile spi_write_sem;

int entity_handler_init() {
	if(!init) {
		if(sem_init((sem_t*)&spi_write_sem, 0, 1)<0) {
			perror("entity_handler_init sem_init");
			return -1;
		}
		init = true;
	}
	return 0;
}

int entity_handler_args_init(entity_handler_args_t* __args, entity_bus_t* __bus, uint8_t __fps) {
	memset(__args, 0, sizeof(entity_handler_args_t));

	if(pthread_mutex_init(&__args->mutex, NULL)<0) {
		perror("entity_handler_args_init pthread_mutex_init");
		return -1;
	}

	if(sem_init(&__args->entity_sem, 0, 0)<0) {
		perror("entity_handler_args_init sem_init");
		return -1;
	}

	__args->frame_update_completed = true;
	__args->bus = __bus;
	__args->fps = __fps;

	return 0;
}

int entity_handler_args_destroy(entity_handler_args_t* __args) {
	if(pthread_mutex_destroy(&(__args->mutex))<0) {
		perror("entity_handler_args_destroy pthread_mutex_destroy");
		return -1;
	}
	if(sem_destroy(&(__args->entity_sem))<0) {
		perror("entity_handler_args_destroy sem_destroy");
		return -1;
	}

	__args->bus = NULL;
	return 0;
}

void* entity_handler(void* __args) {
	if(!init) {
		perror("entity_handler init false");
		pthread_exit(NULL);
	}

	//get refs
	entity_handler_args_t* args = (entity_handler_args_t*)__args;

	//start the main loop
	while(1) {
	//Wait for signal from main_thread
		sem_wait(&args->entity_sem);

		//Lock arg mutex
		pthread_mutex_lock(&args->mutex);

		//Check for exit issued
		if(args->thread_exit_issued) {
			pthread_mutex_unlock(&args->mutex);
			pthread_exit(NULL);
		}

		//Lock bus mutex
		pthread_mutex_lock(&args->bus->mutex);

		//Start writing the updated byte array
		handle_new_frame(args);

		//Write it out to the spi_bus
		write_spi(args);

		pthread_mutex_unlock(&args->bus->mutex);

		args->frame_update_completed = true;
		pthread_mutex_unlock(&args->mutex);
	}
}

static void handle_new_frame(entity_handler_args_t* __args) {
	//When we need to update more than one frame
	//Check if we need to load a full second or not
	if(__args->current_frame != (__args->last_frame+1)) {
		uint16_t last_second = __args->last_frame / __args->fps;
		uint16_t current_second = __args->current_frame / __args->fps;

//		printf("Maybe nee to load a full image. From: %d To: %d\n", last_second, current_second);
		//When we are in another second load the image
		//Set last_frame to the frame of the loaded second
		if(current_second != last_second) {
			printf("Loaded full image\n");
			memcpy(&__args->bus->intern_spi_write_out[0], &__args->bus->second[current_second].spi_write_out[0], __args->bus->size_spi_write_out);
			__args->last_frame = current_second * __args->fps;
		}
	}

	//Update spi_write_out to the current_frame
	printf("Updating from: %d\t to: %d\n", __args->last_frame, __args->current_frame);
	for(uint16_t i=__args->last_frame+1; i<=__args->current_frame; i++) {
//		printf("\tChecking frame: %d for changes\n", i);
		if(__args->bus->frame[i].num_change > 0) {
			for(uint16_t j=0; j<__args->bus->frame[i].num_change; j++) {
				uint16_t offset = __args->bus->frame[i].change[j].led_offset;
				uint8_t* color = &(__args->bus->frame[i].change[j].color[0]);
//				printf("Frame: %d\tChange:\tOffset: %d\tA: %d\tB: %d\t G: %d\t R: %d\n", i, offset, color[0], color[1], color[2], color[3]);
				fflush(stdout);
				memcpy(&__args->bus->intern_spi_write_out[offset], color, 4);
			}
		}
	}

	memcpy(&__args->bus->spi_write_out[0], &__args->bus->intern_spi_write_out[0], __args->bus->size_spi_write_out);
	__args->last_frame = __args->current_frame;
}

static void write_spi(entity_handler_args_t* __args) {
	//Wait for the semaphore
	//This is to ensure only one thread calls spi_write at a time
	sem_wait((sem_t*)&spi_write_sem);

	//Write out to the spi_bus
	spi_write(__args->bus->bus_id, __args->bus->spi_write_out, __args->bus->size_spi_write_out);

	//Give permission for another thread to proceed with spi_write
	sem_post((sem_t*)&spi_write_sem);
}
