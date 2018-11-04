#ifndef ENTITY_HANDLER_H
#define ENTITY_HANDLER_H

#include "entity.h"

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
	//Mutex for struct access
	pthread_mutex_t	mutex;

	//The thread waits on this sem
	sem_t		entity_sem;

	//If we want to exit, this becomes true
	bool		thread_exit_issued;

	//Check this to see if the thread is still running after
	//issuing a frame update
	bool		frame_update_completed;

	//Need to update this before calling sem_post
	long		last_frame;
	long		current_frame;

	//Ref to the bus struct
	entity_bus_t*	bus;
	//fps of the project
	uint8_t		fps;
}entity_handler_args_t;

int entity_handler_init();
int entity_handler_args_init(entity_handler_args_t* __args, entity_bus_t* __bus, uint8_t __fps);
int entity_handler_args_destroy(entity_handler_args_t* __args);
void* entity_handler(void* __args);

static void handle_new_frame(entity_handler_args_t* __args);
static void write_spi(entity_handler_args_t* __args);

#endif /* ENTITY_HANDLER_H */
